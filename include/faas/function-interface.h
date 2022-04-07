//
// Created by kingdo on 2022/3/29.
//

#ifndef WUKONG_FUNCTION_INTERFACE_H
#define WUKONG_FUNCTION_INTERFACE_H

#include <pistache/async.h>
#include <utility>
#include <wukong/proto/proto.h>
class Agent;
std::string faas_ping();

struct FaasHandle
{
    FaasHandle(std::shared_ptr<wukong::proto::Message> msg_pyr_, Agent* agent_)
        : msg_ptr(std::move(msg_pyr_))
        , agent(agent_)
    { }
    std::shared_ptr<wukong::proto::Message> msg_ptr;
    std::unordered_map<uint64_t, Pistache::Async::Promise<std::string>> internalCallResultMap;
    Agent* agent;
};

std::string faas_getInput(FaasHandle* handle);

void faas_setOutput(FaasHandle* handle, const std::string& result);

uint64_t faas_call(FaasHandle* handle, const std::string& funcname, const std::string& args);

bool faas_get_call_result(FaasHandle* handle, uint64_t request_id, std::string& result);

bool faas_result_is_success(FaasHandle* handle, uint64_t request_id)
{
    if (!handle->internalCallResultMap.contains(request_id))
    {
        SPDLOG_ERROR("can't find request_id:{}", request_id);
        return false;
    }
    return handle->internalCallResultMap.at(request_id).isFulfilled();
}
bool faas_result_is_failed(FaasHandle* handle, uint64_t request_id)
{
    if (!handle->internalCallResultMap.contains(request_id))
    {
        SPDLOG_ERROR("can't find request_id:{}", request_id);
        return false;
    }
    return handle->internalCallResultMap.at(request_id).isRejected();
}
bool faas_result_is_pending(FaasHandle* handle, uint64_t request_id)
{
    if (!handle->internalCallResultMap.contains(request_id))
    {
        SPDLOG_ERROR("can't find request_id:{}", request_id);
        return false;
    }
    return handle->internalCallResultMap.at(request_id).isPending();
}
bool faas_result_is_complete(FaasHandle* handle, uint64_t request_id)
{
    if (!handle->internalCallResultMap.contains(request_id))
    {
        SPDLOG_ERROR("can't find request_id:{}", request_id);
        return false;
    }
    return handle->internalCallResultMap.at(request_id).isSettled();
}
#endif // WUKONG_FUNCTION_INTERFACE_H
