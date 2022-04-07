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

void LocalGatewayClientHandler::handleResult(int fd)
{
    FuncResult result;
    wukong::utils::read_from_fd(fd, &result);
    //    SPDLOG_DEBUG("Read : {} {} {}", result.success, result.msg_id, result.data);
    wukong::utils::UniqueLock lock_external_response(externalResponseMapMutex);
    wukong::utils::UniqueLock lock_internal_response(internalResponseMapMutex);
    // TODO 这里可以从msg_id上判断是否是内部请求，比如，内部请求的id是奇数而外部请求是偶数等
    bool is_external = externalResponseMap.contains(result.msg_id);
    bool is_internal = internalResponseMap.contains(result.msg_id);
    WK_CHECK_WITH_ASSERT(is_external != is_internal, fmt::format("msg_id == {}, is_external:{} = is_internal:{} in handler {} externalResponseMap {}", result.msg_id, is_external, is_internal, (void*)this, (void*)(&externalResponseMap)));
    std::string funcname;
    LocalGateway::Process* process_ptr;
    if (is_external)
    {
        lock_internal_response.unlock();
        auto& responseEntry = externalResponseMap.at(result.msg_id);
        funcname            = responseEntry.funcname;
        process_ptr         = static_cast<LocalGateway::Process*>(responseEntry.process);
        lg->backProcess(funcname, process_ptr);
        if (result.success)
        {
            std::string msg = fmt::format("{{'status':'OK','data'='{}'}}", result.data);
            responseEntry.response.send(Pistache::Http::Code::Ok, msg);
        }
        else
        {
            std::string msg = fmt::format("{{'status':'Failed','data'='{}'}}", result.data);
            responseEntry.response.send(Pistache::Http::Code::Internal_Server_Error, msg);
        }
        externalResponseMap.erase(result.msg_id);
    }
    else
    {
        lock_external_response.unlock();
        auto& responseEntry = internalResponseMap.at(result.msg_id);
        int responseFD      = responseEntry.responseFD;
        funcname            = responseEntry.funcname;
        process_ptr         = static_cast<LocalGateway::Process*>(responseEntry.process);
        lg->backProcess(funcname, process_ptr);
        internalResponseMap.erase(result.msg_id);
        lock_internal_response.unlock();
        wukong::utils::write_2_fd(responseFD, *((InternalResponse*)(&result)));
    }
}

void LocalGatewayClientHandler::handleInternalRequest(int fd)
{
    InternalRequest request;
    wukong::utils::read_from_fd(fd, &request);
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

void LocalGatewayClientHandler::handleExternalReadyQueue()
{
    for (;;)
    {
        auto entry = externalReadyQueue.popSafe();
        if (!entry)
            break;
        const auto& msg = entry->msg;
        SPDLOG_DEBUG("Handle External Request ， call Func `{}`", msg.function());
        LocalGateway::Process* process;
        auto ret = lg->takeProcess(msg.function(), &process, this);
        if (ret.first)
        {
            WK_CHECK_WITH_ASSERT(process != nullptr, "process == nullptr");
            uint64_t msg_id = msg.id();
            wukong::utils::UniqueLock lock(process->handler->externalResponseMapMutex);
            process->handler->externalResponseMap.emplace(msg_id, ExternalResponseEntry(process, msg.function(), std::move(entry->response)));
            std::string msg_json = wukong::proto::messageToJson(msg);
            WK_CHECK_WITH_ASSERT(msg_json.size() < wukong::utils::Config::InstanceFunctionReadBufferSize(),
                                 "msg is too long");
            msg_json.resize(wukong::utils::Config::InstanceFunctionReadBufferSize(), 0);
            // TODO 缺少封装
            ::write(process->sub_process->write_fd(), msg_json.data(), msg_json.size());
        }
        else
        {
            // TODO 默认创建一定是成功的，不然要加入到 wait-Queue
            WK_CHECK(false, "We can't handle Failed Now");
        }
    }
}
void LocalGatewayClientHandler::handleInternalReadyQueue()
{
    for (;;)
    {
        auto entry = internalReadyQueue.popSafe();
        if (!entry)
            break;
        const auto& msg = entry->msg;
        SPDLOG_DEBUG("Handle Internal Request ， call Func `{}`", msg.function());
        LocalGateway::Process* process;
        auto ret = lg->takeProcess(msg.function(), &process, this);
        if (ret.first)
        {
            WK_CHECK_WITH_ASSERT(process != nullptr, "process == nullptr");
            uint64_t msg_id = msg.id();
            wukong::utils::UniqueLock lock(process->handler->internalResponseMapMutex);
            process->handler->internalResponseMap.emplace(msg_id, InternalResponseEntry(process, msg.function(), entry->internalResponseFD));
            std::string msg_json = wukong::proto::messageToJson(msg);
            WK_CHECK_WITH_ASSERT(msg_json.size() < wukong::utils::Config::InstanceFunctionReadBufferSize(),
                                 "msg is too long");
            msg_json.resize(wukong::utils::Config::InstanceFunctionReadBufferSize(), 0);
            // TODO 缺少封装
            ::write(process->sub_process->write_fd(), msg_json.data(), msg_json.size());
        }
        else
        {
            // TODO 默认创建一定是成功的，不然要加入到 wait-Queue
            WK_CHECK(false, "We can't handle Failed Now");
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
