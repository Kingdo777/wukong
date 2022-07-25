//
// Created by kingdo on 2022/3/28.
//

#include "LocalGateway.h"

RequestEntry::RequestEntry(wukong::proto::Message msg_, RequestType type_)
    : type(type_)
    , msg(std::move(msg_))
{ }

InternalRequestEntry::InternalRequestEntry(wukong::proto::Message msg_, int internalResponseFD_)
    : RequestEntry(std::move(msg_), InternalRequest_Type)
    , internalResponseFD(internalResponseFD_)
{
    if (msg.function().starts_with(STORAGE_FUNCTION_NAME))
        type = InternalStorageRequest_Type;
}
ExternalRequestEntry::ExternalRequestEntry(wukong::proto::Message msg_, Pistache::Http::ResponseWriter response_)
    : RequestEntry(std::move(msg_), ExternalRequest_Type)
    , response(std::move(response_))
{ }

FunctionInstanceInfo::FunctionInstanceInfo(std::shared_ptr<LocalGatewayHandler> handler, std::string funcInst_uuid, int64_t slots_or_freeSize)
    : handler(std::move(handler))
    , funcInst_uuid(std::move(funcInst_uuid))
    , slots_or_freeSize(slots_or_freeSize)
{ }

void FunctionInstanceInfo::dispatch(std::shared_ptr<RequestEntry> entry, std::shared_ptr<FunctionInstanceInfo> inst, int64_t need_slots_or_freeSize)
{
    WK_CHECK_WITH_EXIT(slots_or_freeSize >= need_slots_or_freeSize, "slots_or_freeSize==0??");
    instList->actual_Slots_or_allFreeSize -= need_slots_or_freeSize;
    instList->need_Slots_or_allFreeSize -= need_slots_or_freeSize;
    slots_or_freeSize -= need_slots_or_freeSize;
    handler->putRequest(funcInst_uuid, std::move(entry), std::move(inst));
}
void FunctionInstanceInfo::sendRequest(const wukong::proto::Message& msg) const
{
    std::string msg_json = wukong::proto::messageToJson(msg);
    WK_CHECK_WITH_EXIT(msg_json.size() < wukong::utils::Config::InstanceFunctionReadBufferSize(), "msg is too long");
    msg_json.resize(wukong::utils::Config::InstanceFunctionReadBufferSize(), 0);
    WRITE_2_FD_original(getWriteFD(), msg_json.data(), msg_json.size());
}
int FunctionInstanceInfo::getReadFD() const
{
    return fds[write_readPipePath];
}
int FunctionInstanceInfo::getWriteFD() const
{
    return fds[read_writePipePath];
}
int FunctionInstanceInfo::getRequestFD() const
{
    return fds[request_responsePipePath];
}
int FunctionInstanceInfo::getResponseFD() const
{
    return fds[response_requestPipePath];
}

ResponseEntry::ResponseEntry(std::shared_ptr<FunctionInstanceInfo> inst, std::shared_ptr<RequestEntry> request)
    : inst(std::move(inst))
    , request(std::move(request))
{ }

StorageFuncEphemeralDataRecord::StorageFuncEphemeralDataRecord(bool need_lock)
    : need_lock(need_lock)
{ }
bool StorageFuncEphemeralDataRecord::contains(const std::string& SF_dada_uuid)
{
    if (need_lock)
        wukong::utils::UniqueLock l(mutex);
    return SF_DadaUUID2Inst_map.contains(SF_dada_uuid);
}
std::shared_ptr<FunctionInstanceInfo> StorageFuncEphemeralDataRecord::getInst(const std::string& SF_dada_uuid)
{
    if (need_lock)
        wukong::utils::UniqueLock l(mutex);
    return SF_DadaUUID2Inst_map.at(SF_dada_uuid)->inst;
}
void StorageFuncEphemeralDataRecord::getStorageShm(const std::string& SF_dada_uuid, std::string& result)
{
    if (need_lock)
        wukong::utils::UniqueLock l(mutex);
    // TODO 此时应该从其他节点寻找
    WK_CHECK_WITH_EXIT(SF_DadaUUID2Inst_map.contains(SF_dada_uuid), "Now don't support Across Instance！");
    auto date = SF_DadaUUID2Inst_map.at(SF_dada_uuid);
    wukong::utils::Json json;
    json.set("uuid", SF_dada_uuid);
    json.setUInt64("length", date->data_size);
    result = json.serialize();
}
void StorageFuncEphemeralDataRecord::deleteShmDone(const std::shared_ptr<FunctionInstanceInfo>& inst, size_t length)
{

    if (need_lock)
        wukong::utils::UniqueLock l(mutex);
    inst->slots_or_freeSize += (int64_t)length;
    inst->instList->actual_Slots_or_allFreeSize += (int64_t)length;
}
void StorageFuncEphemeralDataRecord::createShmDone(const std::shared_ptr<FunctionInstanceInfo>& inst, const std::string& SF_dada_uuid, size_t length)
{
    if (need_lock)
        wukong::utils::UniqueLock l(mutex);
    SF_DadaUUID2Inst_map.emplace(SF_dada_uuid, std::make_shared<StorageFuncEphemeralData>(SF_dada_uuid, length, inst));
}
StorageFuncEphemeralDataRecord::StorageFuncEphemeralData::StorageFuncEphemeralData(std::string SF_dada_uuid, size_t size, std::shared_ptr<FunctionInstanceInfo> inst)
    : SF_dada_uuid(std::move(SF_dada_uuid))
    , data_size(size)
    , inst(std::move(inst))
{ }

LocalGateway::Options::Options()
    : threads_(wukong::utils::Config::EndpointNumThreads())
{ }
LocalGateway::Options LocalGateway::Options::options()
{
    return {};
}
LocalGateway::Options& LocalGateway::Options::threads(int val)
{
    threads_ = val;
    return *this;
}

LocalGateway::LocalGateway()
    : endpoint { this }
    , func_pool_process(std::make_shared<wukong::utils::DefaultSubProcess>())
{
    func_pool_exec_path = boost::dll::program_location().parent_path().append("instance-function-pool");
}

void LocalGateway::init(LocalGateway::Options& options)
{
    /// start FunctionPool
    /// FP is responsible for create worker/storage func instance.
    /// when req arrived, and if there are enough func inst, LG will
    /// send 'create req' to FP, and add req to waitQueue, while create Success,
    /// LG will send a certain number of reqs to the new inst.
    initFuncPool();

    WK_CHECK_WITH_EXIT(func_pool_read_fd != -1, "FUNC-POOL read_fd is not SET rightly");
    poller.addFd(func_pool_read_fd,
                 Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Read),
                 Pistache::Polling::Tag(func_pool_read_fd),
                 Pistache::Polling::Mode::Edge);

    readyQueue.bind(poller);
    resultQueue.bind(poller);

    Reactor::init(options.threads_, "Work Function fp");
}

void LocalGateway::run()
{
    /// which include the tasks1 and handler thread;
    /// tasks1 is responsible for listen FP-read_fd, for knowledge inst is created completed,
    /// and then handle the waiting req.
    /// handler is bind with func-instance! each handler listen the inst's read_fd and
    /// request_fd for req-handle-result and internal-req launched by inst. the handler is
    /// also responsible for dispatching req to inst and back the internal-req's result.
    Reactor::run();

    /// task_2 is responsible for listen ReadyQueue, to accept the external-req form endpoint
    /// and internal from task_2;
    /// if there has idle instance that matches the req, then directly send it to the inst by
    /// handler thread. if not, add the req to waitingQueue, and tell the task_1 to create instance.
    ////////  #####     Abandon the dual main thread mode
    //    task_2 = std::thread([=, this] {
    //        for (;;)
    //        {
    //            std::vector<Pistache::Polling::Event> events;
    //            int ready_fds = poller_2.poll(events);
    //            WK_CHECK_WITH_EXIT(ready_fds != -1, "Pistache::Polling");
    //            for (const auto& event : events)
    //            {
    //                if (event.tag == shutdownFd_2.tag())
    //                    return;
    //                onReady2(event);
    //            }
    //        }
    //    });

    /// the endpoint is responsible for receiving req from users(external req)
    endpoint.start();

    /// 返回local-gateway的实际port
    if (wukong::utils::getIntEnvVar("NEED_RETURN_PORT", 0) == 1)
    {
        SPDLOG_DEBUG("NEED_RETURN_PORT");
        WRITE_2_FD(subprocess_write_fd, endpoint.port());
    }
}

void LocalGateway::shutdown()
{
    endpoint.stop();

    killAllProcess();
    Reactor::shutdown();

    func_pool_process->kill();
}

void LocalGateway::initFuncPool()
{
    std::string cmd = func_pool_exec_path.string();
    wukong::utils::SubProcess::Options options(cmd);
    options.Env("WHO_AM_I", "Function-Pool");
    func_pool_process->setOptions(options);
    int err = func_pool_process->spawn();
    WK_CHECK_WITH_EXIT(0 == err, fmt::format("spawn process \"{}\" failed with errno {}", func_pool_exec_path.string(), err));
    func_pool_read_fd  = func_pool_process->read_fd();
    func_pool_write_fd = func_pool_process->write_fd();

    wukong::utils::nonblock_ioctl(func_pool_write_fd, 0);
    auto msg = GreetingMsg("HELLO FP");
    WRITE_2_FD(func_pool_write_fd, msg);

    /// 等待function启动完成
    SPDLOG_DEBUG("Waiting Create Function Pool");
    bool success;
    wukong::utils::nonblock_ioctl(func_pool_read_fd, 0);
    READ_FROM_FD(func_pool_read_fd, &success);
    wukong::utils::nonblock_ioctl(func_pool_read_fd, 1);
    WK_CHECK_WITH_EXIT(success, fmt::format("Create Function Pool Failed"));

    SPDLOG_DEBUG("Create Function Pool Success");
}

bool LocalGateway::checkUser(const std::string& username)
{
    return this->username_ == username;
}

bool LocalGateway::checkApp(const std::string& appname)
{
    return this->appname_ == appname;
}

bool LocalGateway::existFunCode(const std::string& funcname, FunctionType type)
{
    return exists(getFunCodePath(funcname, type));
}

wukong::proto::Function LocalGateway::getFunction(const std::string& funcname)
{
    wukong::utils::ReadLock lock(functions_shared_mutex);
    WK_CHECK_WITH_EXIT(functions.contains(funcname), fmt::format("func {} is not found", funcname));
    return functions.at(funcname);
}

void LocalGateway::addFunction(const std::string& funcname, const wukong::proto::Function& func)
{
    wukong::utils::WriteLock lock(functions_shared_mutex);
    WK_CHECK_WITH_EXIT(!functions.contains(funcname), fmt::format("func {} is exist", funcname));
    functions.emplace(funcname, func);
}

std::pair<bool, std::string> LocalGateway::loadFuncCode(const std::string& funcname, FunctionType type, bool update)
{
    bool success    = false;
    std::string msg = "ok";
    auto func_path  = getFunCodePath(funcname, type);
    if (exists(func_path) && !update)
    {
        msg = fmt::format("lib {} is exists", func_path.string());
        return std::make_pair(success, msg);
    }
    if (!exists(func_path.parent_path()))
        create_directories(func_path.parent_path());
    const auto& func = getFunction(funcname);
    redis.get_to_file(func.storagekey(), func_path);
    WK_CHECK_WITH_EXIT(exists(func_path), "get code Failed");
    if (type == Cpp && !PingCode(func_path))
    {
        msg = "Ping Code failed";
        return std::make_pair(success, msg);
    }
    SPDLOG_DEBUG("Write lib to {} and Ping it Success", func_path.string());
    success = true;
    return std::make_pair(success, msg);
}

std::pair<bool, std::string> LocalGateway::initApp(const std::string& username, const std::string& appname)
{
    username_   = username;
    appname_    = appname;
    auto funSet = redis.smembers(SET_FUNCTION_REDIS_KEY(username_, appname_));
    if (funSet.empty())
    {
        return std::make_pair(false, fmt::format("{}#{} have no Functions", username_, appname_));
    }
    for (const auto& funcname : funSet)
    {
        auto funcHash    = redis.hgetall(FUNCTION_REDIS_KEY(username_, appname_, funcname));
        const auto& func = wukong::proto::hashToFunction(funcHash);
        addFunction(funcname, func);
        auto type = wukong::proto::toFunctionType(func.type());
        if (!LocalGateway::existFunCode(funcname, type))
        {
            auto load_res = loadFuncCode(funcname, type);
            WK_CHECK_WITH_EXIT(load_res.first, load_res.second);
        }
        funcInstanceList_map.emplace(funcname, std::make_shared<FunctionInstanceList>());
        SPDLOG_DEBUG(fmt::format("Init App : load func {} OK!", funcname));
    }
    funcInstanceList_map.emplace(STORAGE_FUNCTION_NAME, std::make_shared<FunctionInstanceList>());
    SPDLOG_DEBUG(fmt::format("Init App : {}#{} Done", username_, appname_));
    return std::make_pair(true, "ok");
}

bool LocalGateway::PingCode(const boost::filesystem::path& func_path)
{
    wukong::utils::Lib lib;
    if (lib.open(func_path.c_str()))
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

void LocalGateway::externalCall(const wukong::proto::Message& msg, Pistache::Http::ResponseWriter response)
{
    std::shared_ptr<ExternalRequestEntry> entry = std::make_shared<ExternalRequestEntry>(msg, std::move(response));
    readyQueue.push(std::move(entry));
}
void LocalGateway::internalCall(const wukong::proto::Message& msg, int responseFD)
{
    std::shared_ptr<InternalRequestEntry> entry = std::make_shared<InternalRequestEntry>(msg, responseFD);
    readyQueue.push(std::move(entry));
}
void LocalGateway::putResult(const ResponseEntry& entry)
{
    resultQueue.push(entry);
}
std::string LocalGateway::username() const
{
    return username_;
}
std::string LocalGateway::appname() const
{
    return appname_;
}

std::shared_ptr<LocalGatewayHandler> LocalGateway::pickOneHandler()
{
    return std::static_pointer_cast<LocalGatewayHandler>(Reactor::pickHandler());
}
void LocalGateway::onReady(const Pistache::Polling::Event& event)
{
    if (event.flags.hasFlag(Pistache::Polling::NotifyOn::Read))
    {
        auto fd = event.tag.value();
        /// for monitoring whether the instance is created completely!
        if (static_cast<ssize_t>(fd) == func_pool_read_fd)
        {
            try
            {
                instCreateDone();
            }
            catch (std::exception& ex)
            {
                SPDLOG_ERROR("instCreateDone error: {}", ex.what());
            }
        }
        /// internalReadyQueue and readyQueue is responsible for listening the req form user
        /// and function respectively.
        else if (event.tag == readyQueue.tag())
        {
            try
            {
                handlerRequest();
            }
            catch (std::exception& ex)
            {
                SPDLOG_ERROR("readyQueue error: {}", ex.what());
            }
        }
        else if (event.tag == resultQueue.tag())
        {
            try
            {
                handlerResult();
            }
            catch (std::exception& ex)
            {
                SPDLOG_ERROR("resultQueue error: {}", ex.what());
            }
        }
    }
}
void LocalGateway::handlerRequest()
{
    for (;;)
    {
        auto entry = readyQueue.popSafe();
        if (!entry)
            break;
        auto& entry_shared_ptr = *entry;
        switch (entry_shared_ptr->type)
        {
        case ExternalRequest_Type:
        case InternalRequest_Type:
            handlerWorkerRequest(entry_shared_ptr);
            break;
        case InternalStorageRequest_Type:
            handlerStorageRequest(std::make_shared<InternalRequestEntry>(reinterpret_cast<InternalRequestEntry&&>(*entry_shared_ptr)));
            break;
        }
    }
}
void LocalGateway::handlerWorkerRequest(const std::shared_ptr<RequestEntry>& entry)
{
    const auto& msg = entry->msg;

    SPDLOG_DEBUG("Handle External Request , call Func `{}`", msg.function());
    auto funcname = msg.function();
    /// Determine there are whether available concurrent slots to handle this req
    if (!checkInstList_and_dispatchReq(entry, funcname))
    {
        /// there are not enough slots to handler req,
        /// we should add the req to the waitQueue firstly, and then tell the FP to create new instance!
        toWaitQueue_and_CreateNewInst(entry, funcname);
    }
}
void LocalGateway::handlerStorageRequest(const std::shared_ptr<InternalRequestEntry>& entry)
{
    auto& msg      = entry->msg;
    int responseFD = entry->internalResponseFD;

    SPDLOG_DEBUG("Handle Storage Internal Request ， call Func `{}`", msg.function());
    const std::string& funcname = msg.function();
    std::string wukong_funcname = std::string { STORAGE_FUNCTION_NAME };
    if (funcname == fmt::format("{}/{}", wukong_funcname, StorageFuncOpTypeName[StorageFuncOpType::Create]))
    {
        /// 1. get request mem-space size
        auto length = (int64_t)strtoul(msg.inputdata().c_str(), nullptr, 10);
        WK_CHECK_WITH_EXIT(length <= STORAGE_FUNCTION_DEFAULT_SIZE, "length > 64MB");

        /// 2. check the list_map to determine there are enough space to hold the SF request.
        if (!checkInstList_and_dispatchReq(entry, wukong_funcname, length))
        {
            /// 3. add req to waitQueue and Create new Instance
            toWaitQueue_and_CreateNewInst(entry, wukong_funcname);
        }
    }
    else if (funcname == fmt::format("{}/{}", wukong_funcname, StorageFuncOpTypeName[StorageFuncOpType::Delete]))
    {
        const std::string& SF_dada_uuid = msg.inputdata();
        WK_CHECK_WITH_EXIT(storageFuncEphemeralDataRecord.contains(SF_dada_uuid), fmt::format("Can't find uuid {}", SF_dada_uuid));
        auto inst = storageFuncEphemeralDataRecord.getInst(SF_dada_uuid);
        inst->dispatch(entry, inst, 0);
    }
    else if (funcname == fmt::format("{}/{}", wukong_funcname, StorageFuncOpTypeName[StorageFuncOpType::Get]))
    {
        wukong::utils::Json json;
        const auto& uuid = msg.inputdata();
        std::string shm;
        storageFuncEphemeralDataRecord.getStorageShm(uuid, shm);
        FuncResult result(true, shm, msg.id());
        WRITE_2_FD(responseFD, result);
        SPDLOG_DEBUG("Return getSHM Request : {}", shm);
        return;
    }
}
void LocalGateway::toWaitQueue_and_CreateNewInst(const std::shared_ptr<RequestEntry>& entry, const std::string& funcname)
{
    if (!waitQueue.contains(funcname))
        waitQueue.emplace(funcname, std::queue<std::shared_ptr<RequestEntry>>());
    waitQueue.at(funcname).push(entry);
    if (funcInstanceList_map.at(funcname)->needCreateInst())
        CreateFuncInst(funcname);
}
void LocalGateway::CreateFuncInst(const std::string& funcname)
{
    FunctionInstanceType instType = (funcname == std::string { STORAGE_FUNCTION_NAME }) ? FunctionInstanceType::StorageFunction : FunctionInstanceType::WorkerFunction;
    FunctionType type;
    std::string wukong_funcname;
    int64_t slots_or_freeSize;
    if (instType == WorkerFunction)
    {
        const auto& func  = getFunction(funcname);
        type              = wukong::proto::toFunctionType(func.type());
        wukong_funcname   = funcname;
        slots_or_freeSize = func.concurrency();
    }
    else
    {
        type              = StorageFunc;
        wukong_funcname   = std::string { STORAGE_FUNCTION_NAME };
        slots_or_freeSize = STORAGE_FUNCTION_DEFAULT_SIZE;
    }
    FuncCreateMsg fcMsg(funcname, type, instType);
    fcMsg.magic_number = MAGIC_NUMBER_WUKONG;
    WRITE_2_FD(func_pool_write_fd, fcMsg);
    /// Important, update creating_Slots_or_allFreeSize
    WK_CHECK_WITH_EXIT(funcInstanceList_map.contains(wukong_funcname), fmt::format("can't find funcname : `{}`", wukong_funcname));
    funcInstanceList_map.at(wukong_funcname)->creating_Slots_or_allFreeSize += slots_or_freeSize;
}
void LocalGateway::instCreateDone()
{
    for (;;)
    {
        FuncCreateDoneMsg msg;
        // TODO 缺少封装
        READ_FROM_FD_goto(func_pool_read_fd, &msg);
        /// 1. check Magic-Number
        WK_CHECK_WITH_EXIT(MAGIC_NUMBER_CHECK(msg.magic_number), "Data Wrong, Magic Check Failed!");
        auto funcname = std::string { msg.funcname, msg.funcname_size };

        /// 2.  determine the Slots or free size according to instType
        auto instType             = msg.instType;
        int64_t slots_or_freeSize = STORAGE_FUNCTION_DEFAULT_SIZE;
        int pipes_count           = PipeIndex::pipeCount;
        switch (instType)
        {
        case WorkerFunction:
            slots_or_freeSize = getFunction(funcname).concurrency();
            break;
        case StorageFunction:
            pipes_count = 2;
            break;
        default:
            WK_CHECK_WITH_EXIT(false, "Unknown Inst-Type");
        }
        /// 3. update Inst-Map
        auto instList  = funcInstanceList_map.at(funcname);
        instList->type = instType;
        instList->actual_Slots_or_allFreeSize += slots_or_freeSize;
        instList->creating_Slots_or_allFreeSize -= slots_or_freeSize;
        /// specify handler
        auto handler = pickOneHandler();
        auto inst    = std::make_shared<FunctionInstanceInfo>(handler, std::string { msg.funcInst_uuid, msg.uuidSize }, slots_or_freeSize);
        /// record the List which inst belong to
        inst->instList = instList;
        /// open pipes and listen
        for (int i = 0; i < pipes_count; ++i)
        {
            auto pipe_path  = boost::filesystem::path(std::string { msg.PipeArray[i], msg.PipeSizeArray[i] });
            auto pipe_name  = pipe_path.filename().string();
            auto open_flags = (pipe_name.starts_with("read") || pipe_name.starts_with("response")) ? O_WRONLY : O_RDONLY | O_NONBLOCK;
            inst->fds[i]    = ::open(pipe_path.c_str(), open_flags);
            //            if(pipe_name.starts_with("read") || pipe_name.starts_with("response"))
            //                wukong::utils::nonblock_ioctl(inst->fds[i], 1);
        }
        /// add Instance to handler's InstMap
        handler->addInst(inst->funcInst_uuid, inst);
        instList->list.emplace_back(inst);

        /// 4. check waitQueue
        if (waitQueue.contains(funcname) && !waitQueue.at(funcname).empty())
        {
            WK_CHECK_WITH_EXIT(inst->slots_or_freeSize > 0, "inst->slots_or_freeSize<=0");
            auto& reqQueue = waitQueue.at(funcname);
            while (inst->slots_or_freeSize > 0)
            {
                if (reqQueue.empty())
                    break;
                auto req      = reqQueue.front();
                bool isWorker = msg.instType == FunctionInstanceType::WorkerFunction;
                WK_CHECK_WITH_EXIT((req->type == InternalStorageRequest_Type && !isWorker) || (req->type != InternalStorageRequest_Type && isWorker), "InstType and ReqType don't correspond");
                int64_t need_slots_or_freeSize = isWorker ? 1 : strtol(req->msg.inputdata().c_str(), nullptr, 10);
                WK_CHECK_WITH_EXIT(need_slots_or_freeSize > 0, "need_slots_or_freeSize<0");
                inst->dispatch(req, inst, need_slots_or_freeSize);
                reqQueue.pop();
            }
        }
    }
read_fd_EAGAIN:;
}
bool LocalGateway::checkInstList_and_dispatchReq(const std::shared_ptr<RequestEntry>& entry, const std::string& funcname, int64_t need_Slots_or_FreeSize)
{
    WK_CHECK_WITH_EXIT(funcInstanceList_map.contains(funcname), fmt::format("funcInstanceList_map can't find function: `{}`", funcname));
    const auto& instList = funcInstanceList_map.at(funcname);
    instList->need_Slots_or_allFreeSize += need_Slots_or_FreeSize;
    bool enough = instList->actual_Slots_or_allFreeSize >= need_Slots_or_FreeSize;
    if (!enough)
    {
        /// avoiding too many request prevent free-slots from adding , That is Important!
        handlerResult();
        enough = instList->actual_Slots_or_allFreeSize >= need_Slots_or_FreeSize;
    }
    if (enough)
    {
        auto funcInstanceList = funcInstanceList_map.at(funcname)->list;

        for (auto& inst : funcInstanceList)
        {
            WK_CHECK_WITH_EXIT(inst->slots_or_freeSize >= 0, "inst->slots_or_freeSize < 0");
            if (inst->slots_or_freeSize < need_Slots_or_FreeSize)
                continue;
            inst->dispatch(entry, inst, need_Slots_or_FreeSize);
            goto out;
        }
        WK_CHECK_WITH_EXIT(false, "Unreachable!");
    }
out:
    return enough;
}
void LocalGateway::handlerResult()
{
    for (;;)
    {
        auto entry = resultQueue.popSafe();
        if (!entry)
            break;
        auto inst = entry->inst;
        auto req  = entry->request;
        switch (req->type)
        {
        case RequestType::ExternalRequest_Type:
        case RequestType::InternalRequest_Type: {
            inst->slots_or_freeSize++;
            inst->instList->actual_Slots_or_allFreeSize++;
            break;
        }
        case RequestType::InternalStorageRequest_Type: {
            break;
        }
        }
    }
}
void LocalGateway::killAllProcess()
{
    //#TODO
}
