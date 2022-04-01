//
// Created by kingdo on 2022/3/27.
//

#ifndef WUKONG_AGENT_HANDLER_H
#define WUKONG_AGENT_HANDLER_H

#include <faas/function-interface.h>
#include <pistache/http.h>
#include <pistache/reactor.h>
#include <wukong/proto/proto.h>

class Agent;

class AgentHandler : public Pistache::Aio::Handler
{
public:
    PROTOTYPE_OF(Pistache::Aio::Handler, AgentHandler)

    explicit AgentHandler(Agent* agent_)
        : agent(agent_) {};

    AgentHandler(const AgentHandler& handler)
        : agent(handler.agent)
    { }

    void onReady(const Pistache::Aio::FdSet& fds) override;

    void registerPoller(Pistache::Polling::Epoll& poller) override;

    struct MessageEntry
    {
        explicit MessageEntry(wukong::proto::Message msg_)
            : msg(std::move(msg_))
        { }
        wukong::proto::Message msg;
    };

    void putMessage(wukong::proto::Message msg);

private:
    void handlerMessage();

    Agent* agent;
    Pistache::PollableQueue<MessageEntry> messageQueue;
};

#endif //WUKONG_AGENT_HANDLER_H
