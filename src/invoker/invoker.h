//
// Created by kingdo on 2022/3/9.
//

#ifndef WUKONG_INVOKER_H
#define WUKONG_INVOKER_H

#include "instance-proxy/InstanceProxy.h"
#include "invokerEndpoint.h"
#include <wukong/client/client-server.h>
#include <wukong/proto/proto.h>
#include <wukong/utils/locks.h>

#if INSTANCE_PROXY == FIRECRACKER
#include "instance-proxy/firecracker/FirecrackerInstanceProxy.h"
#define PROXY_CLASS FirecrackerInstanceProxy
#elif INSTANCE_PROXY == DOCKER
#include "instance-proxy/docker/DockerInstanceProxy.h"
#define PROXY_CLASS DockerInstanceProxy
#elif INSTANCE_PROXY == PROCESS

#include "instance-proxy/process/ProcessInstanceProxy.h"

#define PROXY_CLASS ProcessInstanceProxy
#endif

class Invoker
{

public:
    Invoker();

    void start();

    void stop();

    wukong::proto::Invoker invokerProto;

    std::string toInvokerJson() const;

#define FUNCTION_INDEX(username, appname, funcname) fmt::format("{}#{}#{}", username, appname, funcname)
#define APP_INDEX(username, appname) fmt::format("{}#{}", username, appname)

    void startupInstance(const wukong::proto::Application& app, Pistache::Http::ResponseWriter response);

    void shutdownInstance(const wukong::proto::Application& app, Pistache::Http::ResponseWriter response)
    {
        wukong::utils::UniqueLock lock(proxy_mutex);
    }

private:
    enum InvokerStatus {
        Uninitialized,
        Running,
        Stopped
    };

    InvokerStatus status = Uninitialized;
    bool registered      = false;

    InvokerEndpoint endpoint;

    wukong::client::ClientServer cs;

    std::pair<bool, std::string> register2LB(const std::string& invokerJson);

    std::unordered_map<std::string, std::shared_ptr<PROXY_CLASS>> proxyMap;
    std::mutex proxy_mutex;
};

#endif //WUKONG_INVOKER_H
