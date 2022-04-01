//
// Created by kingdo on 2022/3/27.
//

#ifndef WUKONG_AGENT_H
#define WUKONG_AGENT_H

#include <boost/filesystem.hpp>
#include <pistache/async.h>
#include <pistache/reactor.h>
#include <wukong/proto/proto.h>
#include <wukong/utils/config.h>
#include <wukong/utils/dl.h>
#include <wukong/utils/log.h>
#include <wukong/utils/os.h>
#include <wukong/utils/redis.h>

#include "AgentHandler.h"
#include <utility>

typedef void (*Faas_Main)(FaasHandle*);

class Agent
{
public:
    enum Type {
        C_PP,
        Python,
        Storage
    };

    struct Options
    {
        friend class Agent;

        Options();

        static Options options();

        Options& threads(int val);

        Options& type(Type val);

    private:
        int threads_;
        int read_fd;
        uint64_t max_read_buffer_size;
        int write_fd;
        Type type_;
    };

    Agent();

    void init(Options& options);

    void set_handler(std::shared_ptr<AgentHandler> handler);

    void run();

    void shutdown();

    void doExec(FaasHandle* h);

    void finishExec(wukong::proto::Message msg);

private:
    void onRunning() const;

    void onFailed() const;

    void loadFunc(Options& options);

    std::shared_ptr<AgentHandler> pickOneHandler();

    void handlerWriteQueue();

    void handlerIncoming();

    std::shared_ptr<AgentHandler> handler_ = nullptr;

    std::shared_ptr<Pistache::Aio::Reactor> reactor_;
    Pistache::Aio::Reactor::Key handlerKey_;
    std::atomic<uint64_t> handlerIndex_;

    std::thread task;

    Pistache::Polling::Epoll poller;
    Pistache::NotifyFd shutdownFd;

    int read_fd;
    uint64_t max_read_buffer_size;
    int write_fd;
    Pistache::PollableQueue<wukong::proto::Message> writeQueue;

    Type type;

    wukong::utils::Lib lib;
    Faas_Main func_entry = nullptr;
};

#endif //WUKONG_AGENT_H
