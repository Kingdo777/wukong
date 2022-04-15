//
// Created by kingdo on 2022/3/23.
//

#include "LocalGatewayClientServer.h"
#include "LocalGateway.h"

void LocalGatewayClientHandler::onReady(const Pistache::Aio::FdSet& fds)
{
    for (auto fd : fds)
    {
        if (lg->getReadFDs().contains((int)fd.getTag().value()))
        {
            handleResult((int)fd.getTag().value());
        }
        else if (lg->geInternalRequestFDs().contains((int)fd.getTag().value()))
        {
            handleInternalRequest((int)fd.getTag().value());
        }
        else if (fd.getTag() == internalReadyQueue.tag())
        {
            handleInternalReadyQueue();
        }
        else if (fd.getTag() == externalReadyQueue.tag())
        {
            handleExternalReadyQueue();
        }
        else if (fd.getTag() == internalWaitQueue.tag())
        {
            handleInternalWaitQueue();
        }
        else if (fd.getTag() == externalWaitQueue.tag())
        {
            handleExternalWaitQueue();
        }
    }
    Base::onReady(fds);
}

void LocalGatewayClientHandler::registerPoller(Pistache::Polling::Epoll& poller)
{
    Base::registerPoller(poller);
    internalReadyQueue.bind(poller);
    internalWaitQueue.bind(poller);
    externalReadyQueue.bind(poller);
    externalWaitQueue.bind(poller);
}

void LocalGatewayClientHandler::handleExternalResult(const FuncResult& result)
{
    wukong::utils::UniqueLock lock_external_response(externalResponseMapMutex);
    std::string funcname;
    LocalGateway::Process* process_ptr;
    auto& responseEntry = externalResponseMap.at(result.request_id);
    funcname            = responseEntry.funcname;
    process_ptr         = static_cast<LocalGateway::Process*>(responseEntry.process);
    lg->backWorkerFuncProcess(funcname, process_ptr);
    if (result.success)
    {
        std::string msg = fmt::format("{{'status':'OK','data'='{}'}}", result.data);
        responseEntry.response.send(Pistache::Http::Code::Ok, msg);
        SPDLOG_DEBUG("Write InternalWorkerResult {}", std::string(result.data, result.data_size));
    }
    else
    {
        std::string msg = fmt::format("{{'status':'Failed','data'='{}'}}", result.data);
        responseEntry.response.send(Pistache::Http::Code::Internal_Server_Error, msg);
        SPDLOG_DEBUG("Write ExternalResult {}", msg);
    }
    externalResponseMap.erase(result.request_id);
}
void LocalGatewayClientHandler::handleInternalWorkerResult(const FuncResult& result)
{
    wukong::utils::UniqueLock lock_internal_response(internalWorkerResponseMapMutex);
    std::string funcname;
    LocalGateway::Process* process_ptr;
    auto& responseEntry = internalWorkerResponseMap.at(result.request_id);
    int responseFD      = responseEntry.responseFD;
    funcname            = responseEntry.funcname;
    process_ptr         = static_cast<LocalGateway::Process*>(responseEntry.process);
    lg->backWorkerFuncProcess(funcname, process_ptr);
    internalWorkerResponseMap.erase(result.request_id);
    wukong::utils::write_2_fd(responseFD, result);
    SPDLOG_DEBUG("Write InternalWorkerResult {}", std::string(result.data, result.data_size));
}
void LocalGatewayClientHandler::handleInternalStorageResult(const FuncResult& result)
{
    wukong::utils::UniqueLock lock_internal_response(internalStorageResponseMapMutex);
    std::string funcname;
    LocalGateway::Process* process_ptr;
    auto& responseEntry    = internalStorageResponseMap.at(result.request_id);
    int responseFD         = responseEntry.responseFD;
    funcname               = responseEntry.funcname;
    process_ptr            = static_cast<LocalGateway::Process*>(responseEntry.process);
    StorageFuncOpType type = StorageFuncOpType::Unknown;
    if (funcname == fmt::format("{}/{}", STORAGE_FUNCTION_NAME, StorageFuncOpTypeName[StorageFuncOpType::Create]))
    {
        type = StorageFuncOpType::Create;
    }
    else if (funcname == fmt::format("{}/{}", STORAGE_FUNCTION_NAME, StorageFuncOpTypeName[StorageFuncOpType::Delete]))
    {
        type = StorageFuncOpType::Delete;
    }
    else if (funcname == fmt::format("{}/{}", STORAGE_FUNCTION_NAME, StorageFuncOpTypeName[StorageFuncOpType::Get]))
    {
        type = StorageFuncOpType::Get;
    }
    wukong::utils::Json json(std::string(result.data, result.data_size));
    std::string uuid = json.get("uuid");
    size_t length    = json.getUInt64("length", 0);

    switch (type)
    {
    case Create: {
        lg->createShmDone(process_ptr, uuid, length);
        break;
    }
    case Delete: {
        lg->deleteShmDone(process_ptr, length);
        break;
    }
    case Get:
    case Unknown: {
        WK_CHECK_WITH_EXIT(false, "Unreachable");
    }
    }

    internalStorageResponseMap.erase(result.request_id);
    wukong::utils::write_2_fd(responseFD, result);
    SPDLOG_DEBUG("Write InternalStorageResult {} -> {}", StorageFuncOpTypeName[type], std::string(result.data, result.data_size));
}

void LocalGatewayClientHandler::handleResult(int fd)
{
    for (;;)
    {
        FuncResult result;
        auto ret = wukong::utils::read_from_fd(fd, &result);
        if (ret == -1)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                SPDLOG_ERROR("read fd is wrong : {}", wukong::utils::errors());
                // TODO handler error;
            }
            return;
        }
        SPDLOG_DEBUG("handlerInternalResponse {}", result.data);
        WK_CHECK(ret == sizeof(result), "read_from_fd failed");
        WK_CHECK_WITH_EXIT(MAGIC_NUMBER_CHECK(result.magic_number), "Data Wong, Magic Num Check is Failed");
        result.data[result.data_size] = 0;

        wukong::utils::UniqueLock lock(responseTypeMapMutex);
        WK_CHECK_WITH_EXIT(responseTypeMap.contains(result.request_id), fmt::format("msg_id == {} is not exists in responseTypeMap", result.request_id));
        auto type = responseTypeMap.at(result.request_id);
        lock.unlock();
        switch (type)
        {
        case ResultType::ExternalResponse: {
            handleExternalResult(result);
            break;
        }
        case ResultType::InternalWorkerResponse: {
            handleInternalWorkerResult(result);
            break;
        }
        case ResultType::InternalStorageResponse: {
            handleInternalStorageResult(result);
            break;
        }
        }
    }
}

void LocalGatewayClientHandler::handleInternalRequest(int fd)
{
    for (;;)
    {
        InternalRequest request;
        auto ret = wukong::utils::read_from_fd(fd, &request);
        if (ret == -1)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                SPDLOG_ERROR("read fd is wrong : {}", wukong::utils::errors());
                // TODO handler error;
            }
            return;
        }
        WK_CHECK_WITH_EXIT(MAGIC_NUMBER_CHECK(request.magic_number), "Data Wrong, Magic Number Check Failed");
        SPDLOG_DEBUG("handleInternalRequest {}", request.funcname);
        wukong::proto::Message msg;

        msg.set_id(request.request_id);
        msg.set_user(lg->username());
        msg.set_application(lg->appname());
        msg.set_function(request.funcname);
        msg.set_inputdata(request.args);
        msg.set_type(wukong::proto::Message_MessageType_FUNCTION);
        msg.set_resultkey(fmt::format("{}#{}#{}-{}",
                                      msg.user(),
                                      msg.application(),
                                      msg.function(),
                                      msg.id()));

        msg.set_timestamp(wukong::utils::getMillsTimestamp());
        InternalRequestEntry entry(msg, lg->getInternalResponseFD(fd));
        internalReadyQueue.push(std::move(entry));
    }
}

void LocalGatewayClientHandler::handleExternalReadyQueue()
{
    for (;;)
    {
        auto entry = externalReadyQueue.popSafe();
        if (!entry)
            break;
        const auto& msg = entry->msg;
        SPDLOG_DEBUG("Handle External Request , call Func `{}`", msg.function());
        LocalGateway::Process* process;
        auto ret = lg->takeWorkerFuncProcess(msg.function(), &process, this);
        if (ret.first)
        {
            WK_CHECK_WITH_EXIT(process != nullptr, "process == nullptr");
            uint64_t msg_id = msg.id();
            wukong::utils::UniqueLock lock(process->handler->externalResponseMapMutex);
            wukong::utils::UniqueLock lock_(process->handler->responseTypeMapMutex);
            process->handler->externalResponseMap.emplace(msg_id, ExternalResponseEntry(process, msg.function(), std::move(entry->response)));
            process->handler->responseTypeMap.emplace(msg_id, ResultType::ExternalResponse);

            std::string msg_json = wukong::proto::messageToJson(msg);
            WK_CHECK_WITH_EXIT(msg_json.size() < wukong::utils::Config::InstanceFunctionReadBufferSize(),
                               "msg is too long");
            msg_json.resize(wukong::utils::Config::InstanceFunctionReadBufferSize(), 0);
            // TODO 缺少封装
            ::write(process->sub_process->write_fd(), msg_json.data(), msg_json.size());
        }
        else
        {
            // TODO 默认创建一定是成功的，不然要加入到 wait-Queue
            WK_CHECK(false, fmt::format("We can't handle Failed Now , {}", ret.second));
        }
    }
}

void LocalGatewayClientHandler::handleStorageFunctionInternalRequest(const wukong::proto::Message& msg, int responseFD)
{
    SPDLOG_DEBUG("Handle Storage Internal Request ， call Func `{}`", msg.function());
    const std::string& funcname = msg.function();
    LocalGateway::Process* process;
    StorageFuncOpType type = StorageFuncOpType::Unknown;
    if (funcname == fmt::format("{}/{}", STORAGE_FUNCTION_NAME, StorageFuncOpTypeName[StorageFuncOpType::Create]))
    {
        type = StorageFuncOpType::Create;
    }
    else if (funcname == fmt::format("{}/{}", STORAGE_FUNCTION_NAME, StorageFuncOpTypeName[StorageFuncOpType::Delete]))
    {
        type = StorageFuncOpType::Delete;
    }
    else if (funcname == fmt::format("{}/{}", STORAGE_FUNCTION_NAME, StorageFuncOpTypeName[StorageFuncOpType::Get]))
    {
        type = StorageFuncOpType::Get;
        wukong::utils::Json json;
        const auto& uuid = msg.inputdata();
        std::string shm;
        WK_CHECK_FUNC_RET(lg->getStorageShm(uuid, shm));
        FuncResult result(true, shm, msg.id());
        wukong::utils::write_2_fd(responseFD, result);
        SPDLOG_DEBUG("Return getSHM Request : {}", shm);
        return;
    }
    auto ret = lg->takeStorageFuncProcess(type, msg, &process, this);
    if (ret.first)
    {
        WK_CHECK_WITH_EXIT(process != nullptr, "process == nullptr");
        uint64_t msg_id = msg.id();
        wukong::utils::UniqueLock lock(process->handler->internalStorageResponseMapMutex);
        wukong::utils::UniqueLock lock_(process->handler->responseTypeMapMutex);
        process->handler->internalStorageResponseMap.emplace(msg_id, InternalResponseEntry(process, funcname, responseFD));
        process->handler->responseTypeMap.emplace(msg_id, ResultType::InternalStorageResponse);
        std::string msg_json = wukong::proto::messageToJson(msg);
        WK_CHECK_WITH_EXIT(msg_json.size() < wukong::utils::Config::InstanceFunctionReadBufferSize(),
                           "msg is too long");
        msg_json.resize(wukong::utils::Config::InstanceFunctionReadBufferSize(), 0);
        // TODO 缺少封装
        ::write(process->sub_process->write_fd(), msg_json.data(), msg_json.size());
    }
    else
    {
        // TODO 默认创建一定是成功的，不然要加入到 wait-Queue
        WK_CHECK(false, fmt::format("We can't handle Failed Now , {}", ret.second));
    }
}

void LocalGatewayClientHandler::handleWorkerFunctionInternalRequest(const wukong::proto::Message& msg, int responseFD)
{
    SPDLOG_DEBUG("Handle Internal Request ， call Func `{}`", msg.function());
    const std::string& funcname = msg.function();
    LocalGateway::Process* process;
    auto ret = lg->takeWorkerFuncProcess(msg.function(), &process, this);
    if (ret.first)
    {
        WK_CHECK_WITH_EXIT(process != nullptr, "process == nullptr");
        uint64_t msg_id = msg.id();
        wukong::utils::UniqueLock lock(process->handler->internalWorkerResponseMapMutex);
        wukong::utils::UniqueLock lock_(process->handler->responseTypeMapMutex);
        process->handler->internalWorkerResponseMap.emplace(msg_id, InternalResponseEntry(process, funcname, responseFD));
        process->handler->responseTypeMap.emplace(msg_id, ResultType::InternalWorkerResponse);
        std::string msg_json = wukong::proto::messageToJson(msg);
        WK_CHECK_WITH_EXIT(msg_json.size() < wukong::utils::Config::InstanceFunctionReadBufferSize(),
                           "msg is too long");
        msg_json.resize(wukong::utils::Config::InstanceFunctionReadBufferSize(), 0);
        // TODO 缺少封装
        ::write(process->sub_process->write_fd(), msg_json.data(), msg_json.size());
    }
    else
    {
        // TODO 默认创建一定是成功的，不然要加入到 wait-Queue
        WK_CHECK(false, fmt::format("We can't handle Failed Now , {}", ret.second));
    }
}

void LocalGatewayClientHandler::handleInternalReadyQueue()
{
    for (;;)
    {
        auto entry = internalReadyQueue.popSafe();
        if (!entry)
            break;
        const auto& msg             = entry->msg;
        const std::string& funcname = msg.function();
        if (funcname.starts_with(STORAGE_FUNCTION_NAME))
        {
            handleStorageFunctionInternalRequest(msg, entry->internalResponseFD);
        }
        else
        {
            handleWorkerFunctionInternalRequest(msg, entry->internalResponseFD);
        }
    }
}
void LocalGatewayClientHandler::handleExternalWaitQueue()
{
    WK_CHECK(false, "We can't handle Failed Now");
    //    for (;;)
    //    {
    //        auto entry = externalWaitQueue.popSafe();
    //        if (!entry)
    //            break;
    //        const auto& msg = entry->msg;
    //    }
}
void LocalGatewayClientHandler::handleInternalWaitQueue()
{
    WK_CHECK(false, "We can't handle Failed Now");
    //    for (;;)
    //    {
    //        auto entry = externalWaitQueue.popSafe();
    //        if (!entry)
    //            break;
    //        const auto& msg = entry->msg;
    //    }
}
