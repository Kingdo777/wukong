//
// Created by kingdo on 2022/3/17.
//
#ifndef WUKONG_INSTANCE_PROXY_H
#define WUKONG_INSTANCE_PROXY_H

#include <wukong/utils/config.h>
#include <wukong/utils/log.h>
#include <wukong/utils/radom.h>
#include <wukong/proto/proto.h>
#include <chrono>
#include <set>

#include "../invokerClientServer.h"

#define CHECK_INSTANCE_STATUS_FAILED "Check Your Instance State, it's Unexpected"

#define FunctionName(username, appname, funcname) (fmt::format("{}#{}#{}-{}",username,appname,funcname,wukong::utils::randomString(5)))

class InstanceProxy {
public:

    struct FuncResource {
        uint64_t memory = 0;
        uint64_t cpu = 0;
        uint conc = 0;
    };

    explicit InstanceProxy(uint64_t pause_timeout = wukong::utils::Config::PausedTimeout()) :
            status(NotStarted),
            pauseTimeout(pause_timeout) {
    }

    std::pair<bool, std::string> start(const wukong::proto::Function &func) {
        if (!checkStatus({NotStarted, Removed, Paused}))
            return std::make_pair(false, CHECK_INSTANCE_STATUS_FAILED);
        switch (status) {
            case NotStarted:
            case Removed: {
                /// A. 启动实例
                /// A1. Function的资源配置
                FuncResource fr = {
                        .memory=func.memory(),
                        .cpu=func.cpu(),
                        .conc=func.concurrency()
                };
                /// A2. Function的ID，必须是全局唯一
                std::string funcname = FunctionName(func.user(), func.application(), func.functionname());
                auto result = doStart(fr, funcname);
                if (!result.first) {
                    SPDLOG_WARN(fmt::format("{}#{}#{} Instance Start Failed : {}",
                                            func.user(),
                                            func.application(),
                                            func.functionname(),
                                            result.second));
                    return result;
                }
                ///B.   执行init命令，将函数的代码等传递进去
                ///B1.  判断是否是执行Python函数
                bool isPython = (func.type() == wukong::proto::Function_FunctionType_PYTHON);
                ///B2.  我们将在本地缓存Function Code
                const auto &storageKey = func.storagekey();
                bool localStorage = (status == Removed && lastStorageKey == storageKey);
                ///B3. 更新lastStorageKey
                lastStorageKey = storageKey;
                return doInit(isPython, localStorage, storageKey);
            }
            case Paused:
                // TODO
            default:
                SPDLOG_ERROR("Instance Status Unreachable");
                assert(false);
        }
    }

    std::pair<bool, std::string> pause() {
        if (!checkStatus({Running}))
            return std::make_pair(false, CHECK_INSTANCE_STATUS_FAILED);
        return doPause();
    }

    std::pair<bool, std::string> remove(bool force = false) {
        if (!force && !checkStatus({Paused}))
            return std::make_pair(false, CHECK_INSTANCE_STATUS_FAILED);
        return doRemove();

    }

    std::pair<bool, std::string> ping() {
        if (!checkStatus({Running}))
            return std::make_pair(false, CHECK_INSTANCE_STATUS_FAILED);

        return doPing();
    }

    virtual int getInstancePort() const = 0;

    virtual std::string getInstanceHost() const = 0;

protected:

    virtual std::pair<bool, std::string> doStart(FuncResource fr, const std::string &funcname) = 0;

    virtual std::pair<bool, std::string> doInit(bool isPython, bool localStorage, const std::string &storageKey) = 0;

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

    std::string lastStorageKey;

    bool checkStatus(std::initializer_list<Status> desiredStates);

    std::chrono::seconds pauseTimeout;
};


#endif //WUKONG_INSTANCE_PROXY_H
