//
// Created by kingdo on 2022/3/17.
//

#include "InstanceProxy.h"

bool InstanceProxy::checkStatus(std::initializer_list<Status> desiredStates)
{
    if (std::find(desiredStates.begin(), desiredStates.end(), status) == desiredStates.end())
    {
        std::string desiredStatesList;
        for (auto s : desiredStates)
        {
            desiredStatesList += (StatusName[s] + ", ");
        }
        SPDLOG_ERROR("current status is {}, But [  {} ] is Needed", StatusName[status], desiredStatesList);
        return false;
    }
    return true;
}

std::pair<bool, std::string> InstanceProxy::start(const wukong::proto::Application& app)
{
    if (!checkStatus({ NotStarted, Removed, Paused }))
        return std::make_pair(false, CHECK_INSTANCE_STATUS_FAILED);
    switch (status)
    {
    case NotStarted:
    case Removed: {
        const std::string& username = app.user();
        const std::string& appname  = app.appname();
        auto result                 = doStart(username, appname);
        if (!result.first)
        {
            SPDLOG_WARN(fmt::format("{}#{} Instance Start Failed : {}",
                                    username, appname,
                                    result.second));
            return result;
        }
        return doInit(username, appname);
        //            /// A. 启动实例
        //            /// A1. Function的资源配置
        //            FuncResource fr = {
        //                    .memory=func.memory(),
        //                    .cpu=func.cpu(),
        //                    .conc=func.concurrency()
        //            };
        //            /// A2. Function的ID，必须是全局唯一
        //            std::string funcname = FunctionName(func.user(), func.application(), func.functionname());
        //            auto result = doStart(fr, funcname);
        //            if (!result.first) {
        //                SPDLOG_WARN(fmt::format("{}#{}#{} Instance Start Failed : {}",
        //                                        func.user(),
        //                                        func.application(),
        //                                        func.functionname(),
        //                                        result.second));
        //                return result;
        //            }
        //            ///B.   执行init命令，将函数的代码等传递进去
        //            ///B1.  判断是否是执行Python函数
        //            bool isPython = (func.type() == wukong::proto::Function_FunctionType_PYTHON);
        //            ///B2.  我们将在本地缓存Function Code
        //            const auto &storageKey = func.storagekey();
        //            bool localStorage = (status == Removed && lastStorageKey == storageKey);
        //            ///B3. 更新lastStorageKey
        //            lastStorageKey = storageKey;
        //            return doInit(isPython, localStorage, storageKey);
    }
    case Paused:
        // TODO
    default:
        SPDLOG_ERROR("Instance Status Unreachable");
        assert(false);
    }
}

std::pair<bool, std::string> InstanceProxy::ping()
{
    if (!checkStatus({ Running }))
        return std::make_pair(false, CHECK_INSTANCE_STATUS_FAILED);

    return doPing();
}

std::pair<bool, std::string> InstanceProxy::pause()
{
    if (!checkStatus({ Running }))
        return std::make_pair(false, CHECK_INSTANCE_STATUS_FAILED);
    return doPause();
}

std::pair<bool, std::string> InstanceProxy::remove(bool force)
{
    if (!force && !checkStatus({ Paused }))
        return std::make_pair(false, CHECK_INSTANCE_STATUS_FAILED);
    return doRemove();
}

wukong::client::ClientServer* InstanceProxy::client()
{
    WK_CHECK_WITH_EXIT(cs->isStarted(), "Client Server is not Started");
    return cs;
}

void InstanceProxy::setClient(wukong::client::ClientServer* cs_)
{
    cs = cs_;
}
