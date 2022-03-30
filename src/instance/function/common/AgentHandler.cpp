//
// Created by kingdo on 2022/3/27.
//

#include "AgentHandler.h"
#include "Agent.h"

void AgentHandler::handlerMessage() {
    for (;;) {
        auto entry = messageQueue.popSafe();
        if (!entry)
            break;
        auto msg_ptr = std::make_shared<wukong::proto::Message>(entry->msg);
        FaasHandle handle(msg_ptr);
        agent->doExec(&handle);
        agent->finishExec(std::move(*msg_ptr));
    }
}

void AgentHandler::putMessage(wukong::proto::Message msg) {
    MessageEntry entry(std::move(msg));
    messageQueue.push(std::move(entry));
}

void AgentHandler::onReady(const Pistache::Aio::FdSet &fds) {
    for (auto fd: fds) {
        if (fd.getTag() == messageQueue.tag()) {
            handlerMessage();
        }
    }
}

void AgentHandler::registerPoller(Pistache::Polling::Epoll &poller) {
    messageQueue.bind(poller);
}
