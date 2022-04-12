//
// Created by kingdo on 2022/3/28.
//

#include "LocalGateway.h"
#include "LocalGatewayClientServer.h"

LocalGateway::LocalGateway()
    : endpoint { this }
{
    worker_func_exec_path = boost::dll::program_location().parent_path().append("instance-worker-func");

    storage_func_exec_path = boost::dll::program_location().parent_path().append("instance-storage-func");
}

void LocalGateway::start()
{
    auto opts = Pistache::Http::Client::options().threads(wukong::utils::Config::ClientNumThreads()).maxConnectionsPerHost(wukong::utils::Config::ClientMaxConnectionsPerHost());
    cs.setHandler(std::make_shared<LocalGatewayClientHandler>(&cs, this));
    SPDLOG_INFO("Startup LocalGatewayClientServer with {} threads", wukong::utils::Config::ClientNumThreads());
    cs.start(opts);

    endpoint.start();

    /// 返回local-gateway的实际port
    if (wukong::utils::getIntEnvVar("NEED_RETURN_PORT", 0) == 1)
    {
        SPDLOG_DEBUG("NEED_RETURN_PORT");
        wukong::utils::write_2_fd(subprocess_write_fd, endpoint.port());
    }
}

void LocalGateway::shutdown()
{
    endpoint.stop();
    cs.stop();
    killAllProcess();
}

bool LocalGateway::checkUser(const std::string& username)
{
    return this->username_ == username;
}

bool LocalGateway::checkApp(const std::string& appname)
{
    return this->appname_ == appname;
}

bool LocalGateway::existFunCode(const std::string& funcname)
{
    auto lib_path = boost::filesystem::path("/tmp/wukong").append("func-code").append(funcname).append("lib.so");
    return exists(lib_path);
}

boost::filesystem::path LocalGateway::getFunCodePath(const std::string& funcname)
{
    auto lib_path = boost::filesystem::path("/tmp/wukong").append("func-code").append(funcname).append("lib.so");
    return lib_path;
}

wukong::proto::Function LocalGateway::getFunction(const std::string& funcname)
{
    wukong::utils::ReadLock lock(functions_mutex);
    WK_CHECK_WITH_EXIT(functions.contains(funcname), fmt::format("func {} is not found", funcname));
    return functions.at(funcname);
}

std::pair<bool, std::string> LocalGateway::loadFuncCode(const std::string& funcname, bool update)
{
    bool success    = false;
    std::string msg = "ok";
    auto lib_path   = boost::filesystem::path("/tmp/wukong").append("func-code").append(funcname).append("lib.so");
    if (exists(lib_path) && !update)
    {
        msg = fmt::format("lib {} is exists", lib_path.string());
        return std::make_pair(success, msg);
    }
    wukong::utils::ReadLock lock(functions_mutex);
    if (!functions.contains(funcname))
    {
        msg = fmt::format("func {} is not found", funcname);
        return std::make_pair(success, msg);
    }
    if (!exists(lib_path.parent_path()))
        create_directories(lib_path.parent_path());
    const auto& func = functions.at(funcname);
    redis.get_to_file(func.storagekey(), lib_path);
    WK_CHECK_WITH_EXIT(exists(lib_path), "get code Failed");
    if (!PingCode(lib_path))
    {
        msg = "Ping Code failed";
        return std::make_pair(success, msg);
    }
    SPDLOG_DEBUG("Write lib to {} and Ping it Success", lib_path.string());
    success = true;
    return std::make_pair(success, msg);
}

std::pair<bool, std::string> LocalGateway::initApp(const std::string& username, const std::string& appname)
{
    username_ = username;
    appname_  = appname;
    SPDLOG_DEBUG(fmt::format("Init App : {}#{}", username_, appname_));
    auto funSet = redis.smembers(SET_FUNCTION_REDIS_KEY(username_, appname_));
    if (funSet.empty())
    {
        return std::make_pair(false, fmt::format("{}#{} have no Functions", username_, appname_));
    }
    wukong::utils::WriteLock functions_write_lock(functions_mutex);
    for (const auto& funcname : funSet)
    {
        auto funcHast = redis.hgetall(FUNCTION_REDIS_KEY(username_, appname_, funcname));
        functions.emplace(funcname, wukong::proto::hashToFunction(funcHast));
    }
    wukong::utils::WriteLock processes_write_lock(worker_processes_map_mutex);
    for (const auto& funcname : funSet)
    {
        worker_processes_map.emplace(funcname, std::make_shared<WorkerProcesses>());
    }
    return std::make_pair(true, "ok");
}

bool LocalGateway::PingCode(const boost::filesystem::path& lib_path)
{
    wukong::utils::Lib lib;
    if (lib.open(lib_path.c_str()))
    {
        SPDLOG_ERROR(fmt::format("pingCode Error:{}", lib.errors()));
        lib.close();
        return false;
    }
    FP_Func faas_ping;
    if (lib.sym("_Z9faas_pingB5cxx11v", (void**)&faas_ping))
    {
        SPDLOG_ERROR(fmt::format("pingCode Error:{}", lib.errors()));
        lib.close();
        return false;
    }
    bool pong = faas_ping() == "pong";
    lib.close();
    return pong;
}

/// createFuncProcess中继承了take-process中的两把锁
WK_FUNC_RETURN_TYPE LocalGateway::createWorkerFuncProcess(const wukong::proto::Function& func, Process** process, LocalGatewayClientHandler* handler)
{
    WK_FUNC_START()
    auto sub_process_ptr = std::make_shared<wukong::utils::DefaultSubProcess>();
    std::string cmd      = worker_func_exec_path.string();
    uint flags           = 0; // TODO 这里应该施加资源限制
    wukong::utils::SubProcess::Options options(cmd);
    options.Flags(flags).Env("WHO_AM_I", func.functionname());
    sub_process_ptr->setOptions(options);
    /// 启动子进程
    auto internal_request_fd_index = sub_process_ptr->createPIPE(
        wukong::utils::SubProcess::CREATE_PIPE | wukong::utils::SubProcess::WRITABLE_PIPE);
    auto internal_response_fd_index = sub_process_ptr->createPIPE(
        wukong::utils::SubProcess::CREATE_PIPE | wukong::utils::SubProcess::READABLE_PIPE);
    // TODO 默认创建一定是成功的，即暂时不考虑扩容失败的问题
    int err = sub_process_ptr->spawn();
    WK_CHECK_WITH_EXIT(0 == err, fmt::format("spawn process \"{}\" failed with errno {}", worker_func_exec_path.string(), err));
    int read_fd              = sub_process_ptr->read_fd();
    int write_fd             = sub_process_ptr->write_fd();
    int internal_request_fd  = sub_process_ptr->getPIPE_FD(internal_request_fd_index);
    int internal_response_fd = sub_process_ptr->getPIPE_FD(internal_response_fd_index);

    if (!LocalGateway::existFunCode(func.functionname()))
    {
        auto load_res = loadFuncCode(func.functionname());
        if (!load_res.first)
        {
            return load_res;
        }
    }
    auto lib_path = getFunCodePath(func.functionname());
    FunctionInfo functionInfo;
    /// TODO 应该根据资源限制以及并发数，确定一个比较合适的并发级别
    functionInfo.threads = 1;
    WK_CHECK_WITH_EXIT(lib_path.size() < 256, fmt::format("path <{}> is to long!", lib_path.string()));
    memcpy(functionInfo.lib_path, lib_path.c_str(), lib_path.size());
    wukong::utils::write_2_fd(write_fd, functionInfo);

    /// 等待function启动完成
    wukong::utils::nonblock_ioctl(read_fd, 0);
    wukong::utils::read_from_fd(read_fd, &success);
    wukong::utils::nonblock_ioctl(read_fd, 1);
    WK_FUNC_CHECK(success, fmt::format("Create Worker-Func for {} Failed", func.functionname()));

    SPDLOG_DEBUG("Create Worker-Func for {} Success", func.functionname());
    auto& func_process = worker_processes_map.at(func.functionname());
    func_process->func_slots += (int)func.concurrency();
    const auto& process_shared_ptr = std::make_shared<Process>(std::move(sub_process_ptr), handler, internal_request_fd, internal_response_fd, (int)func.concurrency() - 1);
    func_process->process_vector.emplace_back(process_shared_ptr);
    *process = process_shared_ptr.get();
    /// 将fd写入到handler中进行监听
    // TODO process结束时需要将其从handler中移除
    handler->reactor()->registerFd(handler->key(), read_fd,
                                   Pistache::Polling::NotifyOn::Read,
                                   Pistache::Polling::Mode::Edge);
    handler->reactor()->registerFd(handler->key(), internal_request_fd,
                                   Pistache::Polling::NotifyOn::Read,
                                   Pistache::Polling::Mode::Edge);
    wukong::utils::WriteLock read_fd_set_lock(read_fd_set_mutex);
    wukong::utils::WriteLock internal_request_fd_set_lock(internal_request_fd_set_mutex);
    wukong::utils::WriteLock internal_request_fd_2_response_fd_map_lock(internal_request_fd_2_response_fd_map_mutex);
    read_fd_set.insert(read_fd);
    internal_request_fd_set.insert(internal_request_fd);
    internal_request_fd_2_response_fd_map.emplace(internal_request_fd, internal_response_fd);
    WK_FUNC_END()
}

WK_FUNC_RETURN_TYPE LocalGateway::takeWorkerFuncProcess(const std::string& funcname, LocalGateway::Process** process, LocalGatewayClientHandler* handler)
{
    WK_FUNC_START()
    *process            = nullptr;
    const auto& func    = getFunction(funcname);
    int concurrency     = (int)func.concurrency();
    bool is_concurrency = concurrency > 1;
    WK_CHECK_WITH_EXIT(!is_concurrency, "Dont Support concurrency now");
    wukong::utils::ReadLock processes_read_lock(worker_processes_map_mutex);
    WK_CHECK_WITH_EXIT(worker_processes_map.contains(funcname), fmt::format("unknown function {}", funcname));
    auto& func_processes = worker_processes_map.at(funcname);
    wukong::utils::ReadLock func_processes_read_lock(func_processes->func_processes_shared_mutex);
    int left_func_slots = --(func_processes->func_slots);
    /// 当前必然存在一个空位，要做的是找出这个空位
    //    SPDLOG_DEBUG("takeProcess, func_processes->func_slots:{}", func_processes->func_slots);
    if (left_func_slots >= 0)
    {
        for (const auto& item : func_processes->process_vector)
        {
            int i = 1;
            //            SPDLOG_DEBUG("takeProcess, processes->slots:{}", item->slots);
            if (is_concurrency)
            {
                if (item->slots <= 0)
                    continue;
                int left_slots = --(item->slots);
                WK_CHECK_WITH_EXIT(left_slots >= 0, "left_slots<0");
            }
            else if (!item->slots.compare_exchange_strong(i, 0))
                continue;
            *process = item.get();
            break;
        }
        WK_CHECK_WITH_EXIT(*process != nullptr, "don't found process");
    }
    /// 当前比不存在空位
    else
    {
        /// 只有在非并发，或者刚好被concurrency整除时，才会创建新的process
        /// 如并发值为5，则在-1，-6，-11时才会创建新的process
        if (!(is_concurrency && ((left_func_slots + 1) % concurrency)))
        {
            func_processes_read_lock.unlock();
            wukong::utils::WriteLock func_processes_write_lock(func_processes->func_processes_shared_mutex);
            WK_FUNC_CHECK_RET(createWorkerFuncProcess(func, process, handler));
        }
        else /// 等待process被创建
        {
            while (*process == nullptr)
            {
                func_processes_read_lock.unlock();
                // TODO 如果创建失败，那么就会进入死循环，目前先不支持并发！
                while (func_processes->func_slots >= 0)
                    ;
                func_processes_read_lock.lock();
                for (const auto& item : func_processes->process_vector)
                {
                    if (item->slots <= 0)
                        continue;
                    int left_slots = --(item->slots);
                    WK_CHECK_WITH_EXIT(left_slots >= 0, "left_slots<0");
                    *process = item.get();
                    break;
                }
            }
        }
    }
    WK_FUNC_END()
}

WK_FUNC_RETURN_TYPE LocalGateway::backWorkerFuncProcess(const std::string& funcname, Process* process)
{
    WK_FUNC_START()
    wukong::utils::ReadLock processes_read_lock(worker_processes_map_mutex);
    WK_CHECK_WITH_EXIT(worker_processes_map.contains(funcname), fmt::format("unknown function {}", funcname));
    auto& func_processes = worker_processes_map.at(funcname);
    wukong::utils::ReadLock func_processes_read_lock(func_processes->func_processes_shared_mutex);
    process->slots++;
    func_processes->func_slots++;
    WK_FUNC_END()
}

WK_FUNC_RETURN_TYPE LocalGateway::createStorageFuncProcess(size_t length, LocalGateway::Process** process, LocalGatewayClientHandler* handler)
{
    WK_FUNC_START()
    WK_FUNC_CHECK(length <= STORAGE_FUNCTION_DEFAULT_SIZE, "length > 64MB");
    auto sub_process_ptr = std::make_shared<wukong::utils::DefaultSubProcess>();
    std::string cmd      = storage_func_exec_path.string();
    uint flags           = 0; // TODO 这里应该施加资源限制
    wukong::utils::SubProcess::Options options(cmd);
    options.Flags(flags);
    sub_process_ptr->setOptions(options);
    /// 启动子进程
    // TODO 默认创建一定是成功的，即暂时不考虑扩容失败的问题
    int err = sub_process_ptr->spawn();
    WK_CHECK_WITH_EXIT(0 == err, fmt::format("spawn process \"{}\" failed with errno {}", storage_func_exec_path.string(), err));
    int read_fd = sub_process_ptr->read_fd();

    SPDLOG_DEBUG("Create Storage-Func Success");
    const auto& process_shared_ptr = std::make_shared<Process>(std::move(sub_process_ptr), handler, STORAGE_FUNCTION_DEFAULT_SIZE - length);
    *process                       = process_shared_ptr.get();
    storageProcessList.emplace_back(process_shared_ptr);
    handler->reactor()->registerFd(handler->key(), read_fd,
                                   Pistache::Polling::NotifyOn::Read,
                                   Pistache::Polling::Mode::Edge);
    wukong::utils::WriteLock read_fd_set_lock(read_fd_set_mutex);
    read_fd_set.insert(read_fd);
    WK_FUNC_END()
}

WK_FUNC_RETURN_TYPE LocalGateway::takeStorageFuncProcess(StorageFuncOpType type, const wukong::proto::Message& message, LocalGateway::Process** process, LocalGatewayClientHandler* handler)
{
    WK_FUNC_START()
    *process = nullptr;
    wukong::utils::UniqueLock lock(storageProcessesMutex);
    switch (type)
    {
    case Create: {
        auto length = strtoul(message.inputdata().c_str(), nullptr, 10);
        for (auto& p : storageProcessList)
        {
            if (p->free_size < length)
                continue;
            *process = p.get();
            p->free_size -= length;
            break;
        }
        if (*process == nullptr)
        {
            WK_FUNC_CHECK_RET(createStorageFuncProcess(length, process, handler));
        }
        break;
    }
    case Delete: {
        const std::string& uuid = message.inputdata();
        WK_FUNC_CHECK(uuid2StorageProcessMap.contains(uuid), fmt::format("Can't find uuid {}", uuid));
        *process = uuid2StorageProcessMap.at(uuid).second;
        break;
    }
    default:
        WK_FUNC_CHECK(false, fmt::format("Unknown OP type : {}", type));
    }

    WK_FUNC_END()
}

WK_FUNC_RETURN_TYPE LocalGateway::createShmDone(Process* process, const std::string& uuid, size_t length)
{
    WK_FUNC_START()
    wukong::utils::UniqueLock lock(storageProcessesMutex);
    uuid2StorageProcessMap.emplace(uuid, std::make_pair(length, process));
    WK_FUNC_END()
}

WK_FUNC_RETURN_TYPE LocalGateway::deleteShmDone(Process* process, size_t length)
{
    WK_FUNC_START()
    wukong::utils::UniqueLock lock(storageProcessesMutex);
    process->free_size += length;
    WK_FUNC_END()
}

std::set<int> LocalGateway::getReadFDs()
{
    wukong::utils::ReadLock lock(read_fd_set_mutex);
    return read_fd_set;
}

std::set<int> LocalGateway::geInternalRequestFDs()
{
    wukong::utils::ReadLock lock(internal_request_fd_set_mutex);
    return internal_request_fd_set;
}

std::shared_ptr<LocalGatewayClientHandler> LocalGateway::pickOneHandler()
{
    return std::static_pointer_cast<LocalGatewayClientHandler>(cs.pickOneHandler());
}

void LocalGateway::killAllProcess()
{
    wukong::utils::WriteLock lock(worker_processes_map_mutex);
    for (auto& func_processes : worker_processes_map)
    {
        for (auto& process : func_processes.second->process_vector)
        {
            process->sub_process->kill();
        }
    }
    worker_processes_map.clear();
}
void LocalGateway::externalCall(const wukong::proto::Message& msg, Pistache::Http::ResponseWriter response)
{
    LocalGatewayClientHandler::ExternalRequestEntry entry(msg, std::move(response));
    pickOneHandler()->externalReadyQueue.push(std::move(entry));
}