//
// Created by kingdo on 2022/3/13.
//

#include <wukong/utils/timer.h>

namespace wukong::utils
{
    Timer::Timer(std::string name)
        : timer_fd()
        , threadsName(std::move(name))
        , shutdown_()
        , shutdownFd()
    {
        timer_fd = TRY_RET(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK));
        Pistache::Polling::Tag tag(timer_fd);
        poller.addFd(timer_fd, Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Read), tag);
        shutdownFd.bind(poller);
        thread = std::thread([=, this]() {
            if (!threadsName.empty())
            {
                pthread_setname_np(pthread_self(),
                                   threadsName.substr(0, 15).c_str());
            }
            run();
        });
    }

    void Timer::armMs(std::chrono::milliseconds value, void (*handler)())
    {
        onTimeHandler = handler;
        itimerspec spec { { 0 },
                          { 0 } };
        if (value.count() < 1000)
        {
            spec.it_value.tv_sec     = 0;
            spec.it_value.tv_nsec    = std::chrono::duration_cast<std::chrono::nanoseconds>(value).count();
            spec.it_interval.tv_sec  = 0;
            spec.it_interval.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(value).count();
        }
        else
        {
            spec.it_value.tv_sec     = std::chrono::duration_cast<std::chrono::seconds>(value).count();
            spec.it_value.tv_nsec    = 0;
            spec.it_interval.tv_sec  = std::chrono::duration_cast<std::chrono::seconds>(value).count();
            spec.it_interval.tv_nsec = 0;
        }
        TRY(timerfd_settime(timer_fd, 0, &spec, nullptr));
    }

    void Timer::disarm() const
    {
        assert(timer_fd != -1);
        itimerspec spec { { 0 },
                          { 0 } };
        TRY(timerfd_settime(timer_fd, 0, &spec, nullptr));
    }

    Timer::~Timer()
    {
        if (!shutdown_)
            shutdown();
        if (thread.joinable())
            thread.join();
        if (timer_fd != -1)
            close(timer_fd);
    }

    void Timer::shutdown()
    {
        shutdown_.store(true);
        shutdownFd.notify();
    }

    void Timer::run()
    {
        while (!shutdown_)
        {
            for (;;)
            {
                std::vector<Pistache::Polling::Event> events;
                int ready_fds = poller.poll(events);

                switch (ready_fds)
                {
                case -1:
                case 0:
                    break;
                default:
                    if (shutdown_)
                        return;

                    handleFds(Pistache::Aio::FdSet(std::move(events)));
                }
            }
        }
    }

    void Timer::handleFds(const Pistache::Aio::FdSet& fds)
    {
        for (const auto& entry : fds)
        {
            if (entry.getTag() == Pistache::Polling::Tag(timer_fd))
            {
                uint64_t timer;
                ::read(timer_fd, &timer, sizeof timer);
                onTimeHandler();
                break;
            }
        }
    }
}