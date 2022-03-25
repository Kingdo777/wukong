//
// Created by kingdo on 2022/3/9.
//

#ifndef WUKONG_INVOKERCLIENTSERVER_H
#define WUKONG_INVOKERCLIENTSERVER_H

#include <wukong/client/client-server.h>
#include <wukong/utils/log.h>
#include <wukong/utils/json.h>

static bool clientServerStarted = false;
static wukong::client::ClientServer invokerClientServer;

class InvokerClientServer {
public:
    static void start() {
        if (!clientServerStarted) {
            // TODO Invoker的clientServer线程数，应给设置为1
            auto opts = Pistache::Http::Client::options().
                    threads(wukong::utils::Config::ClientNumThreads()).
                    maxConnectionsPerHost(wukong::utils::Config::ClientMaxConnectionsPerHost());
            invokerClientServer.start(opts);
            clientServerStarted = true;
        }
    }

    static void shutdown() {
        if (clientServerStarted)
            invokerClientServer.shutdown();
    }

    static wukong::client::ClientServer &client() {
        if (clientServerStarted)
            return invokerClientServer;
        SPDLOG_ERROR("ClientServer is not start!");
        assert(false);
    }
};

#endif //WUKONG_INVOKERCLIENTSERVER_H
