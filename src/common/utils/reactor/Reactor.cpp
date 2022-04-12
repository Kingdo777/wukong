//
// Created by kingdo on 2022/4/4.
//

#include <wukong/utils/reactor/Reactor.h>
Reactor::Reactor()
    : reactor_(Pistache::Aio::Reactor::create())
    , handlerKey_()
    , handlerIndex_(0)
{
}
void Reactor::init(int thread_count, const std::string& thread_name)
{
    reactor_->init(Pistache::Aio::AsyncContext(thread_count, thread_name));
    if (!shutdownFd.isBound())
        shutdownFd.bind(poller);
}
void Reactor::set_handler(std::shared_ptr<Pistache::Aio::Handler> handler)
{
    handler_    = std::move(handler);
    handlerKey_ = reactor_->addHandler(handler_);
}
void Reactor::run()
{
    WK_CHECK_WITH_EXIT(handler_ != nullptr, "handler == nullptr");
    reactor_->run();
    task = std::thread([=, this] {
        for (;;)
        {
            std::vector<Pistache::Polling::Event> events;
            int ready_fds = poller.poll(events);
            WK_CHECK_WITH_EXIT(ready_fds != -1, "Pistache::Polling");
            for (const auto& event : events)
            {
                if (event.tag == shutdownFd.tag())
                    return;
                onReady(event);
            }
        }
    });
}
void Reactor::shutdown()
{
    if (shutdownFd.isBound())
        shutdownFd.notify();
    task.join();
    reactor_->shutdown();
}
std::shared_ptr<Pistache::Aio::Handler> Reactor::pickHandler()
{
    auto handlers = reactor_->handlers(handlerKey_);
    auto index    = handlerIndex_.fetch_add(1) % handlers.size();
    return handlers[index];
}
