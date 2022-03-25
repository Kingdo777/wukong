//
// Created by kingdo on 2022/3/9.
//

#ifndef WUKONG_INVOKER_H
#define WUKONG_INVOKER_H

#include <wukong/proto/proto.h>
#include <wukong/utils/locks.h>
#include "invokerClientServer.h"
#include "invokerEndpoint.h"
#include "instance-proxy/InstanceProxy.h"

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

class Invoker {

public:

    Invoker();

    void start();

    void stop();

    wukong::proto::Invoker invokerProto;

    std::string toInvokerJson() const {
        return wukong::proto::messageToJson(invokerProto);
    }

#define FUNCTION_INDEX(username, appname, funcname) fmt::format("{}#{}#{}",username,appname,funcname)

    void startupInstance(const wukong::proto::Function &func, Pistache::Http::ResponseWriter response) {
        wukong::utils::UniqueLock lock(proxy_mutex);
        auto username = func.user();
        auto appname = func.application();
        auto funcname = func.functionname();
        auto func_index = FUNCTION_INDEX(username, appname, funcname);

        if (!proxyMap.contains(func_index)) {
            auto proxy_prt = std::make_shared<ProcessInstanceProxy>();
            proxyMap.emplace(func_index, proxy_prt);
        }
        auto proxy = proxyMap[func_index];
        auto res = proxy->start(func);
        if (res.first) {
            wukong::proto::ReplyStartupInstance reply;
            reply.set_host(proxy->getInstanceHost());
            reply.set_port(std::to_string(proxy->getInstancePort()));
            response.send(Pistache::Http::Code::Ok, wukong::proto::messageToJson(reply));
        } else {
            response.send(Pistache::Http::Code::Internal_Server_Error, res.second);
        }
    }

    void shutdownInstance(const wukong::proto::Function &func, Pistache::Http::ResponseWriter response) {
        wukong::utils::UniqueLock lock(proxy_mutex);

    }

private:
    enum InvokerStatus {
        Uninitialized,
        Running,
        Stopped
    };

    InvokerStatus status = Uninitialized;
    bool registered = false;

    InvokerEndpoint endpoint;

    std::pair<bool, std::string> register2LB(const std::string &invokerJson) const;

    std::unordered_map<std::string, std::shared_ptr<PROXY_CLASS>> proxyMap;
    std::mutex proxy_mutex;
};

#endif //WUKONG_INVOKER_H
