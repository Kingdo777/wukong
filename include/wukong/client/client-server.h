//
// Created by kingdo on 2022/3/6.
//

#ifndef WUKONG_CLIENT_SERVER_H
#define WUKONG_CLIENT_SERVER_H

#include <pistache/client.h>

namespace wukong::client {

    class ClientServer;

    class ClientHandler : public Pistache::Http::Transport {
    PROTOTYPE_OF(Pistache::Aio::Handler, ClientHandler)

    public:
        typedef Pistache::Http::Transport Base;

        ClientHandler() = delete;

        ClientHandler(const ClientHandler &ch) = default;

        explicit ClientHandler(ClientServer *client_) : client(client_) {}

        Pistache::Async::Promise<Pistache::Http::Response>
        post(const std::string &uri, const std::string &data,
             std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

        Pistache::Async::Promise<Pistache::Http::Response>
        get(const std::string &uri, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    protected:
        void onReady(const Pistache::Aio::FdSet &fds) override;

        void registerPoller(Pistache::Polling::Epoll &poller) override;

    private:
        ClientServer *client = nullptr;

    };

    class ClientServer : public Pistache::Http::Client {
        typedef Pistache::Http::Client Base;
    public:
        ClientServer() = default;

        void start(const Options &options);

        void stop() {
            if (status != Shutdown)
                Pistache::Http::Client::shutdown();
        }

        bool isStarted() const {
            return status == Started;
        }

        std::shared_ptr<ClientHandler> pickOneHandler();

        void setHandler(std::shared_ptr<Pistache::Aio::Handler> handler_);

    private:

        enum ClientServerStatus {
            Created,
            Started,
            Shutdown
        };

        ClientServerStatus status = ClientServerStatus::Created;

        std::shared_ptr<Pistache::Aio::Handler> handler = std::make_shared<ClientHandler>(this);

        std::atomic<uint64_t> handlerIndex{0};
    };
}

#endif //WUKONG_CLIENT_SERVER_H
