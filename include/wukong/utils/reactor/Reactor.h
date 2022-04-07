//
// Created by kingdo on 2022/4/4.
//

#ifndef WUKONG_REACTOR_H
#define WUKONG_REACTOR_H

#include "wukong/utils/macro.h"
#include <boost/filesystem.hpp>
#include <pistache/async.h>
#include <pistache/reactor.h>
#include <spdlog/spdlog.h>

class Reactor
{
public:
    Reactor();

    void init(int thread_count, const std::string& thread_name);

    void set_handler(std::shared_ptr<Pistache::Aio::Handler> handler);

    virtual void run();

    virtual void shutdown();

protected:
    virtual void onReady(const Pistache::Polling::Event& event) = 0;

    std::shared_ptr<Pistache::Aio::Handler> pickHandler();

    std::shared_ptr<Pistache::Aio::Handler> handler_ = nullptr;

    std::shared_ptr<Pistache::Aio::Reactor> reactor_;
    Pistache::Aio::Reactor::Key handlerKey_;
    std::atomic<uint64_t> handlerIndex_;

    std::thread task;

    Pistache::Polling::Epoll poller;
    Pistache::NotifyFd shutdownFd;
};

#endif // WUKONG_REACTOR_H
