//
// Created by kingdo on 2022/3/4.
//

#include <wukong/utils/log.h>
#include <wukong/utils/radom.h>

#include <utility>
#include "load-balance.h"


void LoadBalanceClientHandler::registerPoller(Pistache::Polling::Epoll &poller) {
    functionCallQueue.bind(poller);
    Base::registerPoller(poller);
}

void LoadBalanceClientHandler::onReady(const Pistache::Aio::FdSet &fds) {
    for (auto fd: fds) {
        if (fd.getTag() == functionCallQueue.tag()) {
            handleFunctionCallQueue();
        }
    }
    Base::onReady(fds);
}

void LoadBalanceClientHandler::handleFunctionCallQueue() {
    for (;;) {
        auto function = functionCallQueue.popSafe();
        if (!function)
            break;
        asyncCallFunction(std::move(*function));
    }
}

void LoadBalanceClientHandler::asyncCallFunction(LoadBalanceClientHandler::FunctionCallEntry &&entry) {
    auto res = post("127.0.0.1:9080", "");
    std::string funcEntryIndex = wukong::utils::randomString(15);
    functionCallMap.insert(std::make_pair(funcEntryIndex, std::move(entry)));
    res.then(
            [=](Pistache::Http::Response response) {
                auto code = response.code();
                std::string result = response.body();
                responseFunctionCall(code, result, funcEntryIndex);
            },
            [=](std::exception_ptr exc) {
                try {
                    std::rethrow_exception(std::move(exc));
                }
                catch (const std::exception &e) {
                    auto code = Pistache::Http::Code::Internal_Server_Error;
                    std::string result = e.what();
                    responseFunctionCall(code, result, funcEntryIndex);
                }
            });
}

void LoadBalanceClientHandler::callFunction(Pistache::Http::ResponseWriter response) {
    FunctionCallEntry functionCallEntry(std::move(response));
    functionCallQueue.push(std::move(functionCallEntry));
}

void LoadBalanceClientHandler::responseFunctionCall(Pistache::Http::Code code, std::string &result,
                                                    const std::string &funcEntryIndex) {
    auto iter = functionCallMap.find(funcEntryIndex);
    if (iter == functionCallMap.end()) {
        code = Pistache::Http::Code::Internal_Server_Error;
        result = "functionCall not Find in functionCallMap\n";
    }
    iter->second.response.send(code, result);
    functionCallMap.erase(iter);
}

void LoadBalance::start() {
    auto opts = Pistache::Http::Client::options().
            threads(wukong::utils::Config::ClientNumThreads()).
            maxConnectionsPerHost(wukong::utils::Config::ClientMaxConnectionsPerHost());
    cs.setHandler(std::make_shared<LoadBalanceClientHandler>(&cs));
    SPDLOG_INFO("Starting LoadBalance with {} threads", wukong::utils::Config::ClientNumThreads());
    cs.start(opts);
}

void LoadBalance::stop() {
    SPDLOG_INFO("Shutting down LoadBalance");
    cs.shutdown();
}

void LoadBalance::dispatch(wukong::proto::Message &&msg, Pistache::Http::ResponseWriter response) {
    pickOneHandler()->callFunction(std::move(response));
}

std::shared_ptr<LoadBalanceClientHandler> LoadBalance::pickOneHandler() {
    return std::static_pointer_cast<LoadBalanceClientHandler>(cs.pickOneHandler());
}
