//
// Created by kingdo on 2022/3/6.
//

#include "wukong/client/client-server.h"

#include <utility>

namespace wukong::client
{
    void ClientServer::start(const Options& options)
    {
        if (status != Started)
        {
            pool.init(options.maxConnectionsPerHost_, options.maxResponseSize_);
            reactor_->init(Pistache::Aio::AsyncContext(options.threads_));
            transportKey = reactor_->addHandler(handler);
            reactor_->run();
            status = Started;
        }
    }

    std::shared_ptr<ClientHandler> ClientServer::pickOneHandler()
    {
        auto transports = reactor_->handlers(transportKey);
        auto index      = handlerIndex.fetch_add(1) % transports.size();

        auto transport = std::static_pointer_cast<ClientHandler>(transports[index]);
        return transport;
    }

    void ClientServer::setHandler(std::shared_ptr<Pistache::Aio::Handler> handler_)
    {
        handler = std::move(handler_);
    }

    void ClientHandler::onReady(const Pistache::Aio::FdSet& fds)
    {
        Transport::onReady(fds);
    }

    void ClientHandler::registerPoller(Pistache::Polling::Epoll& poller)
    {
        Transport::registerPoller(poller);
    }

    Pistache::Async::Promise<Pistache::Http::Response>
    ClientHandler::post(const std::string& uri, const std::string& data, std::chrono::milliseconds timeout)
    {
        return client->post(uri).body(data).timeout(timeout).send();
    }

    Pistache::Async::Promise<Pistache::Http::Response>
    ClientHandler::get(const std::string& uri, std::chrono::milliseconds timeout)
    {
        return client->get(uri).timeout(timeout).send();
    }

}