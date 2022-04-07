//
// Created by kingdo on 2022/3/29.
//

#include "faas/function-interface.h"
#include "../src/instance/function/common/Agent.h"

std::string faas_ping()
{
    return "pong";
}

std::string faas_getInput(FaasHandle* handle)
{
    return handle->msg_ptr->inputdata();
}

void faas_setOutput(FaasHandle* handle, const std::string& result)
{
    handle->msg_ptr->set_outputdata(result);
}
uint64_t faas_call(FaasHandle* handle, const std::string& funcname, const std::string& args)
{
    uint64_t request_id = wukong::utils::uuid();
    Pistache::Async::Promise<std::string> promise([&](Pistache::Async::Deferred<std::string> deferred) {
        handle->agent->internalCall(funcname, args, request_id, std::move(deferred));
    });
    handle->internalCallResultMap.emplace(request_id, std::move(promise));
    return request_id;
}
bool faas_get_call_result(FaasHandle* handle, uint64_t request_id, std::string& result)
{
    if (!handle->internalCallResultMap.contains(request_id))
    {
        SPDLOG_ERROR("can't find request_id:{}", request_id);
        return false;
    }
    auto& promise = handle->internalCallResultMap.at(request_id);
    while (!promise.isSettled())
        ;
    bool success;
    promise.then(
        [&](const std::string& result_) {
            result  = result_;
            success = true;
        },
        [&](std::exception_ptr& eptr) {
            try
            {
                std::rethrow_exception(eptr);
            }
            catch (const std::exception& e)
            {
                result  = e.what();
                success = false;
            }
        });
    WK_CHECK_WITH_ASSERT(promise.isSettled(), "Unreachable");
    SPDLOG_DEBUG("success : {}, result : {}", success, result);
    return success;
}
