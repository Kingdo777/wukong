//
// Created by kingdo on 2022/3/17.
//

#ifndef WUKONG_PROCESS_INSTANCE_PROXY_H
#define WUKONG_PROCESS_INSTANCE_PROXY_H

#include "../InstanceProxy.h"
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <wukong/utils/os.h>
#include <wukong/utils/process/DefaultSubProcess.h>

class ProcessInstanceProxy : public InstanceProxy
{
public:
    ProcessInstanceProxy() = default;

    [[nodiscard]] int getInstancePort() const override
    {
        return instancePort;
    }

    [[nodiscard]] std::string getInstanceHost() const override
    {
        return instanceHost;
    };

private:
    std::pair<bool, std::string> doStart(const std::string& username, const std::string& appname) override;

    std::pair<bool, std::string> doInit(const std::string& username, const std::string& appname) override;

    std::pair<bool, std::string> doPause() override
    {
        return {};
    }

    std::pair<bool, std::string> doRemove() override
    {
        bool ret        = process.kill();
        std::string msg = ret ? "Success" : "Failed";
        return std::make_pair(ret, msg);
    }

    std::pair<bool, std::string> doPing() override;

private:
    boost::filesystem::path exec_path;
    wukong::utils::DefaultSubProcess process;
    int instancePort         = 0;
    std::string instanceHost = "localhost";
};

#endif //WUKONG_PROCESS_INSTANCE_PROXY_H
