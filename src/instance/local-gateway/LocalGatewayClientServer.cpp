//
// Created by kingdo on 2022/3/23.
//

#include "LocalGatewayClientServer.h"
#include "LocalGateway.h"

void LocalGatewayClientHandler::onReady(const Pistache::Aio::FdSet& fds)
{
    for (auto fd : fds)
    {
        if (fd.getTag() == functionCallQueue.tag())
        {
            handleFunctionCallQueue();
        }
        else if (lg->getReadFDs().contains((int)fd.getTag().value()))
        {
            handleResult((int)fd.getTag().value());
        }
    }
    Base::onReady(fds);
}

void LocalGatewayClientHandler::registerPoller(Pistache::Polling::Epoll& poller)
{
    Base::registerPoller(poller);
    functionCallQueue.bind(poller);
}

void LocalGatewayClientHandler::callFunc(int write_fd_, int read_fd_, const wukong::proto::Message& msg,
                                         Pistache::Http::ResponseWriter response)
{
    FunctionCallEntry functionCallEntry = FunctionCallEntry(write_fd_, read_fd_, msg, std::move(response));
    functionCallQueue.push(std::move(functionCallEntry));
}

void LocalGatewayClientHandler::handleFunctionCallQueue()
{
    for (;;)
    {
        auto entry = functionCallQueue.popSafe();
        if (!entry)
            break;
        const auto& msg = entry->msg;
        uint64_t msg_id = msg.id();
        wukong::utils::UniqueLock lock(msgResponseMapMutex);
        msgResponseMap.emplace(msg_id, std::move(entry->response));
        std::string msg_json = wukong::proto::messageToJson(msg);
        WK_CHECK_WITH_ASSERT(msg_json.size() < wukong::utils::Config::InstanceFunctionReadBufferSize(),
                             "msg is too long");
        msg_json.resize(wukong::utils::Config::InstanceFunctionReadBufferSize(), 0);
        // TODO 缺少封装
        ::write(entry->write_fd, msg_json.data(), msg_json.size());
    }
}

void LocalGatewayClientHandler::handleResult(int fd)
{
    struct
    {
        bool success    = false;
        uint64_t msg_id = 0;
        char data[2048] = { 0 };
    } result;
    wukong::utils::read_from_fd(fd, &result);
    //    SPDLOG_DEBUG("Read : {} {} {}", result.success, result.msg_id, result.data);
    wukong::utils::UniqueLock lock(msgResponseMapMutex);
    WK_CHECK_WITH_ASSERT(msgResponseMap.contains(result.msg_id), "result is illegal");
    if (result.success)
    {
        std::string msg = fmt::format("{{'status':'OK','data'='{}'}}", result.data);
        msgResponseMap.at(result.msg_id).send(Pistache::Http::Code::Ok, msg);
    }
    else
    {
        std::string msg = fmt::format("{{'status':'Failed','data'='{}'}}", result.data);
        msgResponseMap.at(result.msg_id).send(Pistache::Http::Code::Internal_Server_Error, msg);
    }
    msgResponseMap.erase(result.msg_id);
}
