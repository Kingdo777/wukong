//
// Created by kingdo on 2022/3/13.
//

#ifndef TIMER_TEST_TIMER_H
#define TIMER_TEST_TIMER_H

#include <atomic>
#include <sys/timerfd.h>
#include <thread>
#include <unistd.h>
#include <utility>

#include <cassert>
#include <pistache/os.h>
#include <pistache/reactor.h>

namespace wukong::utils
{

    class Timer
    {
    public:
        explicit Timer(std::string name = "timer");

        void armMs(std::chrono::milliseconds value, void handler());

        void disarm() const;

        ~Timer();

        void shutdown();

    private:
        void run();

        void handleFds(const Pistache::Aio::FdSet& fds);

        int timer_fd;

        Pistache::Polling::Epoll poller;

        void (*onTimeHandler)() = []() {};

        std::thread thread;
        std::string threadsName;

        std::atomic<bool> shutdown_;
        Pistache::NotifyFd shutdownFd;
    };

}
#endif //TIMER_TEST_TIMER_H
