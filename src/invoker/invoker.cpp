//
// Created by kingdo on 2022/3/9.
//
#include <wukong/utils/config.h>

#include "invoker.h"

Invoker::Invoker() {
    invokerProto.set_invokerid(wukong::utils::Config::InvokerInitID());
    invokerProto.set_port(wukong::utils::Config::EndpointPort());
    invokerProto.set_cpu(wukong::utils::Config::InvokerCPU());
    invokerProto.set_memory(wukong::utils::Config::InvokerMemory());
    endpoint.associateEndpoint(this);
}

void Invoker::start() {
    if (status != InvokerStatus::Uninitialized) {
        SPDLOG_WARN("Invoker is Started");
        return;
    }
    InvokerClientServer::start();
    auto result = register2LB(toInvokerJson());
    if (!result.first) {
        SPDLOG_ERROR("Connect LB Failed : {}", result.second);
        InvokerClientServer::shutdown();
        exit(0);
    }
    SPDLOG_INFO("Invoker Register Success : {}", result.second);
    invokerProto = wukong::proto::jsonToInvoker(result.second);
    endpoint.start();
    SPDLOG_INFO("{} is Running...", invokerProto.invokerid());
    status = InvokerStatus::Running;
}

void Invoker::stop() {
    if (status != InvokerStatus::Running) {
        SPDLOG_WARN("Invoker is not Running");
        return;
    }
    endpoint.stop();
    InvokerClientServer::shutdown();
    status = InvokerStatus::Stopped;
}

std::pair<bool, std::string> Invoker::register2LB(const std::string &invokerJson) const {
    bool success = false;
    std::string msg = "Invoker Registered";
    if (!registered) {
        std::string LBHost = wukong::utils::Config::LBHost();
        int LBPort = wukong::utils::Config::LBPort();
        std::string uri = LBHost + ":" + std::to_string(LBPort) + "/invoker/register";
        auto rsp = InvokerClientServer::client().post(uri).body(invokerJson).timeout(std::chrono::seconds(5)).send();
        while (rsp.isPending());
        rsp.then(
                [&](Pistache::Http::Response response) {
                    if (response.code() == Pistache::Http::Code::Ok) {
                        msg = response.body();
                        success = true;
                    } else {
                        msg = "Status Code Wrong, " + response.body();
                    }
                },
                [&](const std::exception_ptr &exc) {
                    try {
                        std::rethrow_exception(exc);
                    }
                    catch (const std::exception &e) {
                        msg = e.what();
                    }
                });
    }
    return std::make_pair(success, msg);
}

void Invoker::startupInstance(const wukong::proto::Application &app, Pistache::Http::ResponseWriter response) {
    wukong::utils::UniqueLock lock(proxy_mutex);
    auto username = app.user();
    auto appname = app.appname();
    auto app_index = APP_INDEX(username, appname);

    if (!proxyMap.contains(app_index)) {
        auto proxy_prt = std::make_shared<ProcessInstanceProxy>();
        proxyMap.emplace(app_index, proxy_prt);
        auto res = proxy_prt->start(app);
        if (!res.first) {
            response.send(Pistache::Http::Code::Internal_Server_Error, res.second);
            return;
        }
    }
    auto proxy = proxyMap[app_index];
    wukong::proto::ReplyStartupInstance reply;
    reply.set_host(proxy->getInstanceHost());
    reply.set_port(std::to_string(proxy->getInstancePort()));
    response.send(Pistache::Http::Code::Ok, wukong::proto::messageToJson(reply));
}

std::string Invoker::toInvokerJson() const {
    return wukong::proto::messageToJson(invokerProto);
}
