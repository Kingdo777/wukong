//
// Created by kingdo on 2022/3/4.
//

#include <wukong/utils/log.h>
#include <wukong/utils/radom.h>
#include <wukong/utils/redis.h>
#include <wukong/utils/timing.h>
#include <wukong/utils/string-tool.h>

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
            [=, this](Pistache::Http::Response response) {
                auto code = response.code();
                std::string result = response.body();
                responseFunctionCall(code, result, funcEntryIndex);
            },
            [=, this](std::exception_ptr exc) {
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
        SPDLOG_ERROR(fmt::format("functionCall not Find in functionCallMap, with funcEntryIndex `{}`",
                                 funcEntryIndex));
        return;
    }
    iter->second.response.send(code, result);
    functionCallMap.erase(iter);
}

void LoadBalance::start() {
    if (status != Uninitialized) {
        SPDLOG_ERROR("LB is not Uninitialized");
        return;
    }
    /// start LB Client
    auto opts = Pistache::Http::Client::options().
            threads(wukong::utils::Config::ClientNumThreads()).
            maxConnectionsPerHost(wukong::utils::Config::ClientMaxConnectionsPerHost());
    cs.setHandler(std::make_shared<LoadBalanceClientHandler>(&cs));
    SPDLOG_INFO("Starting LoadBalance with {} threads", wukong::utils::Config::ClientNumThreads());
    cs.start(opts);

    /// update invoker Info
    auto &redis = wukong::utils::Redis::getRedis();
    invokerSet = redis.smembers(SET_INVOKER_ID);
    std::vector<std::string> illegalInvokerID;
    for (const auto &invokerID: invokerSet) {
        auto invokerHash = redis.hgetall(invokerID);
        bool pong = false;
        int tryPing = 3;
        if (!invokerHash.empty()) {
            for (int try_time = 0; try_time < tryPing; ++try_time) {
                if (try_time)
                    SPDLOG_WARN("Try ping {} {} times", invokerID, try_time);
                auto rsp = cs.
                        get(invokerHash["IP"] + ":" + invokerHash["port"] + "/ping").
                        timeout(std::chrono::seconds(5)).
                        send();
                while (rsp.isPending());
                rsp.then(
                        [&](Pistache::Http::Response response) {
                            if (response.code() == Pistache::Http::Code::Ok && response.body() == "PONG") {
                                pong = true;
                                invokers.insert(std::make_pair(invokerID, wukong::proto::hashToInvoker(invokerHash)));
                            }
                        },
                        [&](const std::exception_ptr &exc) {

                        });
                if (pong)
                    break;
            }
        }
        if (!pong) {
            /// 删除redis中的hash
            redis.del(invokerID);
            /// 删除redis中invokerSet中指定的invokerID
            redis.srem(SET_INVOKER_ID, invokerID);
            illegalInvokerID.push_back(invokerID);
        }
    }
    /// 删除LB缓存的invokerSet中的invokerID
    for (const auto &invokerID: illegalInvokerID) {
        invokerSet.erase(invokerID);
    }

    SPDLOG_INFO("Available invokers detected: {}", invokerSet.empty() ? "NONE" : "");
    for (const auto &invoker: invokerSet) {
        SPDLOG_INFO("{}", invoker);
    }


    status = Running;
}

void LoadBalance::stop() {
    if (status != Running) {
        SPDLOG_ERROR("LB is not Running");
        return;
    }
    SPDLOG_INFO("Shutting down LoadBalance");
    cs.shutdown();
    status = Stopped;
}

void LoadBalance::dispatch(wukong::proto::Message &&msg, Pistache::Http::ResponseWriter response) {
    pickOneHandler()->callFunction(std::move(response));
}

std::shared_ptr<LoadBalanceClientHandler> LoadBalance::pickOneHandler() {
    return std::static_pointer_cast<LoadBalanceClientHandler>(cs.pickOneHandler());
}

std::pair<bool, std::string> LoadBalance::invokerCheck(const wukong::proto::Invoker &invoker) {

    if (invoker.invokerid().empty()) {
        return std::make_pair(false, "invokerID is empty !");
    } else if (invoker.port() <= 0) {
        return std::make_pair(false, fmt::format("port {} is illegal", invoker.port()));
    } else if (invoker.cpu() < 1000) {
        /// CPU 不能少于1个core
        return std::make_pair(false, fmt::format("cpu {} is to small， at least 1 core", invoker.cpu()));
    } else if (invoker.memory() < 2 * 1024) {
        /// CPU 不能少于1个core
        return std::make_pair(false, fmt::format("memory {} is to small， at least 2GB", invoker.memory()));
    } else if (invokerSet.contains(invoker.invokerid())) {
        return std::make_pair(false, fmt::format("InvokerID {} already exists！", invoker.invokerid()));
    }
    for (const auto &existingInvoker: invokers) {
        if (invoker.ip() == existingInvoker.second.ip() && invoker.port() == existingInvoker.second.port())
            return std::make_pair(false, fmt::format("{} has the same ip:port with existing invoker {}！",
                                                     invoker.invokerid(),
                                                     existingInvoker.second.invokerid()));
    }
    return std::make_pair(true, "OK");
}

/// 由于注册Invoker并不是一个经常发生的过程，因此交由endpoint的线程完成，就不需要给LB的Client线程去实现了。
void LoadBalance::handlerInvokerRegister(const std::string &host,
                                         const std::string &invokerJson,
                                         Pistache::Http::ResponseWriter response) {
    wukong::proto::Invoker invoker = wukong::proto::jsonToInvoker(invokerJson);
    invoker.set_registertime(wukong::utils::getMillsTimestamp());
    invoker.set_ip(host);

    auto check_result = invokerCheck(invoker);

    if (check_result.first) {
        auto redis = wukong::utils::Redis::getRedis();
        invokerSet.insert(invoker.invokerid());
        invokers.insert(std::make_pair(invoker.invokerid(), invoker));
        redis.sadd(SET_INVOKER_ID, invoker.invokerid());
        redis.hmset(invoker.invokerid(), wukong::proto::invokerToHash(invoker));
        response.send(Pistache::Http::Code::Ok, wukong::proto::messageToJson(invoker));
        SPDLOG_DEBUG("Invoker Register Done. Success");
    } else {
        response.send(Pistache::Http::Code::Bad_Request, check_result.second);
        SPDLOG_DEBUG("Invoker Register Done. Failed: {}", check_result.second);
    }


}

std::string LoadBalance::getInvokersInfo() {
//    rapidjson::Document invokersInfo;
//    invokersInfo.SetObject();
//    rapidjson::Document::AllocatorType &a = invokersInfo.GetAllocator();
//    for (const auto &invokerID: invokerSet) {
//        wukong::proto::Invoker invoker = invokers[invokerID];
//        auto invokerJsonString = messageToJson(invoker);
//        SPDLOG_DEBUG(invokerJsonString);
//        rapidjson::MemoryStream ms(invokerJsonString.c_str(), invokerJsonString.size());
//        rapidjson::Document d;
//        d.ParseStream(ms);
//        rapidjson::Value invokerIDValue;
//        invokerIDValue.SetString(invokerID.c_str(), a);
//        invokersInfo.AddMember(invokerIDValue, d.GetObject(), a);
//    }
//    rapidjson::StringBuffer sb;
//    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
//    invokersInfo.Accept(writer);
//    return sb.GetString();

    std::string invokersInfoJson = "[";

    for (const auto &invokerID: invokerSet) {
        wukong::proto::Invoker invoker = invokers[invokerID];
        auto invokerJsonString = messageToJson(invoker);
        SPDLOG_DEBUG(invokerJsonString);
        invokersInfoJson += fmt::format("{},", invokerJsonString);
    }
    invokersInfoJson = invokersInfoJson.substr(0, invokersInfoJson.length() - 1);
    invokersInfoJson += "]";
    return invokersInfoJson;
}
