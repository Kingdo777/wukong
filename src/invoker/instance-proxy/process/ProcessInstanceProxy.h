//
// Created by kingdo on 2022/3/17.
//

#ifndef WUKONG_PROCESS_INSTANCE_PROXY_H
#define WUKONG_PROCESS_INSTANCE_PROXY_H

#include <wukong/utils/process/DefaultSubProcess.h>
#include <wukong/utils/os.h>
#include "../InstanceProxy.h"

class ProcessInstanceProxy : public InstanceProxy {
public:
    ProcessInstanceProxy() = default;

    void test() {
        InvokerClientServer::start();
        auto res = doStart({0}, "");
        if (res.first) {
            res = doInit(false, true, "key0");
            if (res.first) {
                SPDLOG_INFO("!!!!!!!!!!!!!!!!!!!!!!!!");
                wait(nullptr);
            }
        } else
            SPDLOG_ERROR(res.second);
    }

    [[nodiscard]] int getInstancePort() const override {
        return instancePort;
    }

    [[nodiscard]] std::string getInstanceHost() const override {
        return instanceHost;

    };

private:
    std::pair<bool, std::string> doStart(FuncResource fr, const std::string &funcname) override;

    std::pair<bool, std::string> doInit(bool isPython, bool localStorage, const std::string &storageKey) override;

    std::pair<bool, std::string> doPause() override {
        return {};
    }

    std::pair<bool, std::string> doRemove() override {
        return {};
    }

    std::pair<bool, std::string> doPing() override;

private:
    std::string exec_path;
    wukong::utils::DefaultSubProcess process;
    int instancePort = 0;
    std::string instanceHost = "localhost";
};


#endif //WUKONG_PROCESS_INSTANCE_PROXY_H
