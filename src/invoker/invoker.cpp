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
}

void Invoker::start() {
    if (status != InvokerStatus::Uninitialized) {
        SPDLOG_WARN("Invoker is Started");
        return;
    }
    auto opts = Pistache::Http::Client::options().
            threads(wukong::utils::Config::ClientNumThreads()).
            maxConnectionsPerHost(wukong::utils::Config::ClientMaxConnectionsPerHost());
    client.start(opts);
    auto result = client.register2LB(toInvokerJson());
    if (!result.first) {
        SPDLOG_ERROR("Connect LB Failed : {}", result.second);
        client.shutdown();
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
    client.shutdown();
    status = InvokerStatus::Stopped;
}
