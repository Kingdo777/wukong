//
// Created by kingdo on 2022/3/28.
//

#include "LocalGateway.h"

LocalGateway::LocalGateway() : endpoint{this} {
    worker_func_exec_path = boost::dll::program_location().parent_path().append("instance-worker-func");

    storage_func_exec_path = boost::dll::program_location().parent_path().append("instance-storage-func");
}

void LocalGateway::start() {
    auto opts = Pistache::Http::Client::options().
            threads(wukong::utils::Config::ClientNumThreads()).
            maxConnectionsPerHost(wukong::utils::Config::ClientMaxConnectionsPerHost());
    cs.setHandler(std::make_shared<LocalGatewayClientHandler>(&cs, this));
    SPDLOG_INFO("Startup LocalGatewayClientServer with {} threads", wukong::utils::Config::ClientNumThreads());
    cs.start(opts);

    endpoint.start();

    /// 返回local-gateway的实际port
    if (wukong::utils::getIntEnvVar("NEED_RETURN_PORT", 0) == 1) {
        SPDLOG_DEBUG("NEED_RETURN_PORT");
        wukong::utils::write_2_fd(subprocess_write_fd, endpoint.port());
    }
}

void LocalGateway::shutdown() {
    endpoint.stop();
    cs.shutdown();
}

bool LocalGateway::checkUser(const std::string &username_) {
    return this->username == username_;
}

bool LocalGateway::checkApp(const std::string &appname_) {
    return this->appname == appname_;
}

bool LocalGateway::existFunCode(const std::string &funcname) {
    auto lib_path = boost::filesystem::path("/tmp/wukong").
            append("func-code").
            append(funcname).
            append("lib.so");
    return exists(lib_path);
}

boost::filesystem::path LocalGateway::getFunCodePath(const std::string &funcname) {
    auto lib_path = boost::filesystem::path("/tmp/wukong").
            append("func-code").
            append(funcname).
            append("lib.so");
    return lib_path;
}

wukong::proto::Function LocalGateway::getFunction(const std::string &funcname) {
    wukong::utils::ReadLock lock(functions_mutex);
    WK_CHECK_WITH_ASSERT(functions.contains(funcname), fmt::format("func {} is not found", funcname));
    return functions.at(funcname);
}

std::pair<bool, std::string> LocalGateway::loadFuncCode(const std::string &funcname, bool update) {
    bool success = false;
    std::string msg = "ok";
    auto lib_path = boost::filesystem::path("/tmp/wukong").
            append("func-code").
            append(funcname).
            append("lib.so");
    if (exists(lib_path) && !update) {
        msg = fmt::format("lib {} is exists", lib_path.string());
        return std::make_pair(success, msg);
    }
    wukong::utils::ReadLock lock(functions_mutex);
    if (!functions.contains(funcname)) {
        msg = fmt::format("func {} is not found", funcname);
        return std::make_pair(success, msg);
    }
    if (!exists(lib_path.parent_path()))
        create_directories(lib_path.parent_path());
    const auto &func = functions.at(funcname);
    redis.get_to_file(func.storagekey(), lib_path);
    WK_CHECK_WITH_ASSERT(exists(lib_path), "get code Failed");
    if (!PingCode(lib_path)) {
        msg = "Ping Code failed";
        return std::make_pair(success, msg);
    }
    SPDLOG_DEBUG("Write lib to {} and Ping it Success", lib_path.string());
    success = true;
    return std::make_pair(success, msg);

}

std::pair<bool, std::string> LocalGateway::initApp(const std::string &username_, const std::string &appname_) {
    username = username_;
    appname = appname_;
    SPDLOG_DEBUG(fmt::format("Init App : {}#{}", username, appname));
    auto funSet = redis.smembers(SET_FUNCTION_REDIS_KEY(username, appname));
    if (funSet.empty()) {
        return std::make_pair(false, fmt::format("{}#{} have no Functions", username, appname));
    }
    wukong::utils::WriteLock lock(functions_mutex);
    for (const auto &funcname: funSet) {
        auto funcHast = redis.hgetall(FUNCTION_REDIS_KEY(username, appname, funcname));
        functions.emplace(funcname, wukong::proto::hashToFunction(funcHast));
    }
    return std::make_pair(true, "ok");

}

bool LocalGateway::PingCode(const boost::filesystem::path &lib_path) {
    wukong::utils::Lib lib;
    if (lib.open(lib_path.c_str())) {
        SPDLOG_ERROR(fmt::format("pingCode Error:{}", lib.errors()));
        lib.close();
        return false;
    }
    FP_Func faas_ping;
    if (lib.sym("_Z9faas_pingB5cxx11v", (void **) &faas_ping)) {
        SPDLOG_ERROR(fmt::format("pingCode Error:{}", lib.errors()));
        lib.close();
        return false;
    }
    bool pong = faas_ping() == "pong";
    lib.close();
    return pong;
}

std::pair<bool, std::string> LocalGateway::createFuncProcess(const wukong::proto::Function &func) {
    wukong::utils::WriteLock writeLock(process_mutex);
    auto process_ptr = std::make_shared<wukong::utils::DefaultSubProcess>();
    wukong::utils::SubProcess::Options options(worker_func_exec_path.string());
    //TODO 这里应该施加资源限制
    options.Flags(0);
    process_ptr->setOptions(options);
    /// 启动子进程
    int err = process_ptr->spawn();
    WK_CHECK_WITH_ASSERT(0 == err, fmt::format("spawn process \"{}\" failed with errno {}",
                                               worker_func_exec_path.string(), err));
    int read_fd = process_ptr->read_fd();
    int write_fd = process_ptr->write_fd();

    if (!LocalGateway::existFunCode(func.functionname())) {
        auto load_res = loadFuncCode(func.functionname());
        if (!load_res.first) {
            return load_res;
        }
    }
    auto lib_path = getFunCodePath(func.functionname());
    struct FunctionInfo {
        char lib_path[256];
        int threads;
    } functionInfo{{0}};
    /// TODO 应该根据资源限制以及并发数，确定一个比较合适的并发级别
    functionInfo.threads = 1;
    WK_CHECK_WITH_ASSERT(lib_path.size() < 256, fmt::format("path <{}> is to long!", lib_path.string()));
    memcpy(functionInfo.lib_path, lib_path.c_str(), lib_path.size());
    wukong::utils::write_2_fd(write_fd, functionInfo);
    bool success = false;
    std::string msg = "ok";
    /// 等待function启动完成
    wukong::utils::nonblock_ioctl(read_fd, 0);
    wukong::utils::read_from_fd(read_fd, &success);
    wukong::utils::nonblock_ioctl(read_fd, 1);
    if (success) {
        SPDLOG_DEBUG("Create Worker-Func for {} Success", func.functionname());
        const auto &handler = pickOneHandler();
        processes.emplace(func.functionname(), ProcessInfo{std::move(process_ptr), handler});
        /// 将fd写入到handler中进行监听
        // TODO process结束时需要将其从handler中移除
        handler->reactor()->registerFd(handler->key(), read_fd,
                                       Pistache::Polling::NotifyOn::Read,
                                       Pistache::Polling::Mode::Edge);
        wukong::utils::WriteLock read_fd_set_lock(read_fd_set_mutex);
        read_fd_set.insert(read_fd);
        return std::make_pair(success, msg);
    } else {
        msg = fmt::format("Create Worker-Func for {} Failed", func.functionname());
        SPDLOG_ERROR(msg);
        return std::make_pair(success, msg);
    }
}

void LocalGateway::callFunc(const wukong::proto::Message &msg, Pistache::Http::ResponseWriter response) {
    const auto &func = getFunction(msg.function());
    wukong::utils::ReadLock readLock(process_mutex);
    if (!processes.contains(func.functionname())) {
        readLock.unlock();
        auto create_res = createFuncProcess(func);
        if (!create_res.first) {
            response.send(Pistache::Http::Code::Internal_Server_Error,
                          fmt::format("load func code failed : {}", create_res.second));
            return;
        }
    }
    // TODO 我们应该在这里做出扩展决策！
    const auto &process_ptr = processes.at(func.functionname()).process();
    const auto &handler_ptr = processes.at(func.functionname()).handler();
    WK_CHECK_WITH_ASSERT(process_ptr->isRunning(), "function sub-process is not running");
    /// 约定第三个通道用于子进程读，父进程写; 第四个通道用于父进程读，子进程写
    int read_fd = process_ptr->read_fd();
    int write_fd = process_ptr->write_fd();
    handler_ptr->callFunc(write_fd, read_fd, msg, std::move(response));
}

std::set<int> LocalGateway::getReadFDs() {
    wukong::utils::ReadLock lock(read_fd_set_mutex);
    return read_fd_set;
}

std::shared_ptr<LocalGatewayClientHandler> LocalGateway::pickOneHandler() {
    return std::static_pointer_cast<LocalGatewayClientHandler>(cs.pickOneHandler());
}
