//
// Created by kingdo on 22-7-22.
//

#include <utility>

#include "LocalGateway.h"

LocalGatewayHandler::FDsRecord::FDsRecord(bool need_lock)
    : need_lock(need_lock)
{ }
void LocalGatewayHandler::FDsRecord::add(int read_fd, int request_fd, int response_fd)
{
    if (need_lock)
        wukong::utils::WriteLock lock(mutex);
    readFD_set.insert(read_fd);
    requestFD_set.insert(request_fd);
    requestFD_2_responseFD_map.emplace(request_fd, response_fd);
}
bool LocalGatewayHandler::FDsRecord::hasReadFD(int read_fd)
{
    if (need_lock)
        wukong::utils::ReadLock lock(mutex);
    return readFD_set.contains(read_fd);
}
bool LocalGatewayHandler::FDsRecord::hasRequestFD(int request_fd)
{
    if (need_lock)
        wukong::utils::ReadLock lock(mutex);
    return requestFD_set.contains(request_fd);
}
int LocalGatewayHandler::FDsRecord::getResponseFD(int request_fd)
{
    if (need_lock)
        wukong::utils::ReadLock lock(mutex);
    WK_CHECK_WITH_EXIT(requestFD_2_responseFD_map.contains(request_fd), "Dont find RequestFD");
    return requestFD_2_responseFD_map.at(request_fd);
}

LocalGatewayHandler::LocalGatewayHandler(LocalGateway* lg_)
    : lg(lg_)
{ }

LocalGatewayHandler::LocalGatewayHandler(const LocalGatewayHandler& handler)
    : lg(handler.lg)
{ }

void LocalGatewayHandler::onReady(const Pistache::Aio::FdSet& fds)
{
    for (auto fd : fds)
    {
        if (fdsRecord.hasReadFD((int)fd.getTag().value()))
        {
            receiveResponse_classifyHandle((int)fd.getTag().value());
        }
        else if (fdsRecord.hasRequestFD((int)fd.getTag().value()))
        {
            receiveInternalReq_ConsMsg_submit2LG((int)fd.getTag().value());
        }
        else if (fd.getTag() == requestQueue.tag())
        {
            sendReq_fromLG_toFuncInst();
        }
    }
}
void LocalGatewayHandler::registerPoller(Pistache::Polling::Epoll& poller)
{
    requestQueue.bind(poller);
}
void LocalGatewayHandler::putRequest(std::shared_ptr<RequestEntry> requestEntry, std::shared_ptr<FunctionInstanceInfo> inst)
{
    LGHandlerRequestEntry entry(std::move(requestEntry), std::move(inst));
    requestQueue.push(std::move(entry));
}
void LocalGatewayHandler::addInst(const std::string& funcInst_uuid, const std::shared_ptr<FunctionInstanceInfo>& inst)
{
    reactor()->registerFd(key(), inst->getReadFD(),
                          Pistache::Polling::NotifyOn::Read,
                          Pistache::Polling::Mode::Edge);
    //    reactor()->registerFdOneShot(key(), inst->getWriteFD(),
    //                                 Pistache::Polling::NotifyOn::Write,
    //                                 Pistache::Polling::Mode::Edge);
    if (inst->instGroup->instList->type == WorkerFunction)
    {
        reactor()->registerFd(key(), inst->getRequestFD(),
                              Pistache::Polling::NotifyOn::Read,
                              Pistache::Polling::Mode::Edge);
        //        reactor()->registerFdOneShot(key(), inst->getResponseFD(),
        //                                     Pistache::Polling::NotifyOn::Write,
        //                                     Pistache::Polling::Mode::Edge);
    }
    fdsRecord.add(inst->getReadFD(), inst->getRequestFD(), inst->getResponseFD());
}

void LocalGatewayHandler::sendReq_fromLG_toFuncInst()
{
    for (;;)
    {
        auto entry = requestQueue.popSafe();
        if (!entry)
            break;

        auto inst = entry->inst;
        /// send the req to Func-Instance
        responseRecord.emplace(entry->request->msg.id(), std::make_shared<ResponseEntry>(inst, entry->request));
        inst->sendRequest(entry->request->msg);
    }
}
void LocalGatewayHandler::receiveInternalReq_ConsMsg_submit2LG(int request_fd)
{
    for (;;)
    {
        InternalRequest request;
        READ_FROM_FD_goto(request_fd, &request);
        WK_CHECK_WITH_EXIT(MAGIC_NUMBER_CHECK(request.magic_number), "Data Wrong, Magic Number Check Failed");
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
        SPDLOG_DEBUG("receiveInternalReq_ConsMsg_submit2LG {}, msg-id = {}", request.funcname, msg.id());
        lg->internalCall(msg, fdsRecord.getResponseFD(request_fd));
    }
read_fd_EAGAIN:;
}
void LocalGatewayHandler::receiveResponse_classifyHandle(int read_fd)
{
    for (;;)
    {
        FuncResult result;
        READ_FROM_FD_goto(read_fd, &result);
        WK_CHECK_WITH_EXIT(MAGIC_NUMBER_CHECK(result.magic_number), "Data Wong, Magic Num Check is Failed");
        result.data[result.data_size] = 0;

        WK_CHECK_WITH_EXIT(responseRecord.contains(result.request_id), fmt::format("msg_id == {} is not exists in responseTypeMap", result.request_id));
        auto req = responseRecord.at(result.request_id)->request;
        switch (req->type)
        {
        case RequestType::ExternalRequest_Type: {
            handleExternalResult(result);
            break;
        }
        case RequestType::InternalRequest_Type: {
            handleInternalWorkerResult(result);
            break;
        }
        case RequestType::InternalStorageRequest_Type: {
            handleInternalStorageResult(result);
            break;
        }
        }
        responseRecord.erase(result.request_id);
    }
read_fd_EAGAIN:;
}
void LocalGatewayHandler::handleExternalResult(const FuncResult& result)
{
    std::string funcname;
    auto requestEntry = std::make_shared<ExternalRequestEntry>(reinterpret_cast<ExternalRequestEntry&&>(*(responseRecord.at(result.request_id)->request)));
    if (result.success)
    {
        std::string msg = fmt::format("{{'status':'OK','data'='{}'}}", result.data);
        requestEntry->response.send(Pistache::Http::Code::Ok, msg);
        SPDLOG_DEBUG("Write ExternalResult {}", std::string(result.data, result.data_size));
    }
    else
    {
        std::string msg = fmt::format("{{'status':'Failed','data'='{}'}}", result.data);
        requestEntry->response.send(Pistache::Http::Code::Internal_Server_Error, msg);
        SPDLOG_DEBUG("Write ExternalResult Failed Reason: {}", msg);
    }
    lg->putResult(*(responseRecord.at(result.request_id)));
}
void LocalGatewayHandler::handleInternalWorkerResult(const FuncResult& result)
{
    auto requestEntry = std::make_shared<InternalRequestEntry>(reinterpret_cast<InternalRequestEntry&&>(*(responseRecord.at(result.request_id)->request)));
    int responseFD    = requestEntry->internalResponseFD;
    WRITE_2_FD(responseFD, result);
    SPDLOG_DEBUG("Write InternalWorkerResult {}", std::string(result.data, result.data_size));
    lg->putResult(*(responseRecord.at(result.request_id)));
}
void LocalGatewayHandler::handleInternalStorageResult(const FuncResult& result)
{
    auto responseEntry = responseRecord.at(result.request_id);
    auto requestEntry  = std::make_shared<InternalRequestEntry>(reinterpret_cast<InternalRequestEntry&&>(*(responseEntry->request)));
    int responseFD     = requestEntry->internalResponseFD;
    auto funcname      = requestEntry->msg.function();

    wukong::utils::Json json(std::string(result.data, result.data_size));
    std::string uuid = json.get("uuid");
    size_t length    = json.getUInt64("length", 0);

    StorageFuncOpType type = StorageFuncOpType::Unknown;
    if (funcname == fmt::format("{}/{}", STORAGE_FUNCTION_NAME, StorageFuncOpTypeName[StorageFuncOpType::Create]))
    {
        type = StorageFuncOpType::Create;
        lg->storageFuncEphemeralDataRecord.createShmDone(responseEntry->inst, uuid, length);
    }
    else if (funcname == fmt::format("{}/{}", STORAGE_FUNCTION_NAME, StorageFuncOpTypeName[StorageFuncOpType::Delete]))
    {
        type = StorageFuncOpType::Delete;
        lg->storageFuncEphemeralDataRecord.deleteShmDone(responseEntry->inst, length);
    }
    else if (funcname == fmt::format("{}/{}", STORAGE_FUNCTION_NAME, StorageFuncOpTypeName[StorageFuncOpType::Get]))
    {
        WK_CHECK_WITH_EXIT(false, "Unreachable");
    }
    else
    {
        WK_CHECK_WITH_EXIT(false, "Unreachable");
    }
    WRITE_2_FD(responseFD, result);
    SPDLOG_DEBUG("Write InternalStorageResult {} -> {}", StorageFuncOpTypeName[type], std::string(result.data, result.data_size));
}
