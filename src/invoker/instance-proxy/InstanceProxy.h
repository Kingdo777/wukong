//
// Created by kingdo on 2022/3/17.
//
#ifndef WUKONG_INSTANCE_PROXY_H
#define WUKONG_INSTANCE_PROXY_H

#include <chrono>
#include <set>
#include <wukong/client/client-server.h>
#include <wukong/proto/proto.h>
#include <wukong/utils/config.h>
#include <wukong/utils/log.h>
#include <wukong/utils/radom.h>

#define CHECK_INSTANCE_STATUS_FAILED "Check Your Instance State, it's Unexpected"

#define FunctionName(username, appname, funcname) (fmt::format("{}#{}#{}-{}", username, appname, funcname, wukong::utils::randomString(5)))
#define AppName(username, appname) (fmt::format("{}#{}-{}", username, appname, wukong::utils::randomString(5)))

class InstanceProxy
{
public:
    struct FuncResource
    {
        uint64_t memory = 0;
        uint64_t cpu    = 0;
        uint conc       = 0;
    };

    explicit InstanceProxy(uint64_t pause_timeout = wukong::utils::Config::PausedTimeout())
        : status(NotStarted)
        , pauseTimeout(pause_timeout)
        , cs(nullptr)
    {
    }

    std::pair<bool, std::string> start(const wukong::proto::Application& app);

    std::pair<bool, std::string> pause();

    std::pair<bool, std::string> remove(bool force = false);

    std::pair<bool, std::string> ping();

    [[nodiscard]] virtual int getInstancePort() const = 0;

    [[nodiscard]] virtual std::string getInstanceHost() const = 0;

    void setClient(wukong::client::ClientServer* cs_);

    wukong::client::ClientServer* client();

protected:
    virtual std::pair<bool, std::string> doStart(const std::string& username, const std::string& appname) = 0;

    virtual std::pair<bool, std::string> doInit(const std::string& username, const std::string& appname) = 0;

    virtual std::pair<bool, std::string> doPause() = 0;

    virtual std::pair<bool, std::string> doRemove() = 0;

    virtual std::pair<bool, std::string> doPing() = 0;

    enum Status {
        NotStarted,
        Running,
        Paused,
        Removed,
        StatusCount
    };

    const std::string StatusName[StatusCount] = {
        "NotStarted",
        "Running",
        "Paused",
        "Removed"
    };

    Status status;

    //    std::string lastStorageKey;

    bool checkStatus(std::initializer_list<Status> desiredStates);

    std::chrono::seconds pauseTimeout;

    wukong::client::ClientServer* cs;
};

#endif //WUKONG_INSTANCE_PROXY_H
