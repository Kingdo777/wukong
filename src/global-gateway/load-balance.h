//
// Created by kingdo on 2022/3/4.
//

#ifndef WUKONG_LOAD_BALANCE_H
#define WUKONG_LOAD_BALANCE_H

#include <wukong/endpoint/endpoint.h>
#include "wukong/client/client-server.h"
#include <wukong/proto/proto.h>
#include <wukong/utils/json.h>

#include <map>

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

    void asyncCallFunction(LoadBalanceClientHandler::FunctionCallEntry &&entry);

private:
    Pistache::PollableQueue<FunctionCallEntry> functionCallQueue;
    std::map<std::string, FunctionCallEntry> functionCallMap;

    void responseFunctionCall(Pistache::Http::Code code, std::string &result, const std::string &funcEntryIndex);
};

class LoadBalance {
public:

    enum LBStatus {
        Uninitialized,
        Running,
        Stopped
    };

    LoadBalance() = default;

    void handlerInvokerRegister(const std::string &host,
                                const std::string &invokerJson,
                                Pistache::Http::ResponseWriter response);

    void dispatch(wukong::proto::Message &&msg, Pistache::Http::ResponseWriter response);

    void start();

    void stop();


    std::string getInvokersInfo();

private:

    LBStatus status = Uninitialized;

    std::shared_ptr<LoadBalanceClientHandler> pickOneHandler();

    std::pair<bool, std::string> invokerCheck(const wukong::proto::Invoker &invoker);

    wukong::client::ClientServer cs;

    std::atomic_uint invokerIndex = 0;

    /// 用于存储invokerID
    std::set<std::string> invokerSet;
    /// 使用invokerID做索引，用于存储invoker的元数据
    std::unordered_map<std::string, wukong::proto::Invoker> invokers;
};


#endif //WUKONG_LOAD_BALANCE_H
