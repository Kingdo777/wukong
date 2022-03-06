//
// Created by kingdo on 2022/3/4.
//

#ifndef WUKONG_LOAD_BALANCE_H
#define WUKONG_LOAD_BALANCE_H

#include <wukong/endpoint/endpoint.h>
#include "wukong/client/client-server.h"
#include <wukong/proto/proto.h>

class LoadBalanceClientHandler : public wukong::client::ClientHandler {
PROTOTYPE_OF(Pistache::Aio::Handler, LoadBalanceClientHandler);

public:
    typedef wukong::client::ClientHandler Base;

    explicit LoadBalanceClientHandler(wukong::client::ClientServer *client) : wukong::client::ClientHandler(client) {}

    LoadBalanceClientHandler(const LoadBalanceClientHandler &handler) : ClientHandler(handler), functionCallQueue() {}

    void onReady(const Pistache::Aio::FdSet &fds) override;

    void registerPoller(Pistache::Polling::Epoll &poller) override;

    struct FunctionCallEntry {

        explicit FunctionCallEntry(Pistache::Http::ResponseWriter response_) : response(std::move(response_)) {}

        Pistache::Http::ResponseWriter response;
    };

    void callFunction(Pistache::Http::ResponseWriter response);

private:

    void handleFunctionCallQueue();

    void asyncCallFunction(LoadBalanceClientHandler::FunctionCallEntry &entry);

private:
    Pistache::PollableQueue<FunctionCallEntry> functionCallQueue;

};

class LoadBalance {
public:
    LoadBalance() = default;

    void dispatch(wukong::proto::Message &&msg, Pistache::Http::ResponseWriter response);

    void start();

    void stop();

private:

    std::shared_ptr<LoadBalanceClientHandler> pickOneHandler();

    wukong::client::ClientServer cs{};

};


#endif //WUKONG_LOAD_BALANCE_H
