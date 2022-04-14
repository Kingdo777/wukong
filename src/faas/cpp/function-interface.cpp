//
// Created by kingdo on 2022/3/29.
//

#include "../src/instance/function/worker-function/WorkerFuncAgent.h"
#include <faas/cpp/function-interface.h>

#define WK_FAAS_FUNC_CHECK(condition, op_name, wrong_msg)                          \
    do                                                                             \
    {                                                                              \
        if (!(condition))                                                          \
        {                                                                          \
            handle->set_errors(fmt::format("{} Failed : {}", op_name, wrong_msg)); \
            return false;                                                          \
        }                                                                          \
                                                                                   \
    } while (false)

std::string faas_ping()
{
    return "pong";
}

size_t faas_getInputSize(FaasHandle* handle)
{
    wukong::utils::UniqueRecursiveLock lock(handle->mutex);
    return handle->msg_ptr->inputdata().size();
}

bool faas_getInput(FaasHandle* handle, std::string& result)
{
    wukong::utils::UniqueRecursiveLock lock(handle->mutex);
    result = handle->msg_ptr->inputdata();
    return true;
}

bool faas_setOutput(FaasHandle* handle, const std::string& result)
{
    wukong::utils::UniqueRecursiveLock lock(handle->mutex);
    handle->msg_ptr->set_outputdata(result);
    return true;
}
bool faas_call(FaasHandle* handle, const std::string& funcname, const std::string& args, uint64_t* request_id)
{
    wukong::utils::UniqueRecursiveLock lock(handle->mutex);
    *request_id = wukong::utils::uuid();
    Pistache::Async::Promise<std::string> promise([&](Pistache::Async::Deferred<std::string> deferred) {
        handle->agent->internalCall(funcname, args, *request_id, std::move(deferred));
    });
    handle->internalCallResultMap.emplace(*request_id, std::move(promise));
    return true;
}
bool faas_get_call_result(FaasHandle* handle, uint64_t request_id, std::string& result)
{
    wukong::utils::UniqueRecursiveLock lock(handle->mutex);
    if (!handle->internalCallResultMap.contains(request_id))
    {
        handle->errors_ = fmt::format("can't find request_id:{}", request_id);
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
    WK_CHECK_WITH_EXIT(promise.isSettled(), "Unreachable");
    SPDLOG_DEBUG("success : {}, result : {}", success, result);
    return success;
}
bool faas_result_is_success(FaasHandle* handle, uint64_t request_id)
{
    wukong::utils::UniqueRecursiveLock lock(handle->mutex);
    if (!handle->internalCallResultMap.contains(request_id))
    {
        handle->errors_ = fmt::format("can't find request_id:{}", request_id);
        return false;
    }
    return handle->internalCallResultMap.at(request_id).isFulfilled();
}
bool faas_result_is_failed(FaasHandle* handle, uint64_t request_id)
{
    wukong::utils::UniqueRecursiveLock lock(handle->mutex);
    if (!handle->internalCallResultMap.contains(request_id))
    {
        handle->errors_ = fmt::format("can't find request_id:{}", request_id);
        return false;
    }
    return handle->internalCallResultMap.at(request_id).isRejected();
}
bool faas_result_is_pending(FaasHandle* handle, uint64_t request_id)
{
    wukong::utils::UniqueRecursiveLock lock(handle->mutex);
    if (!handle->internalCallResultMap.contains(request_id))
    {
        handle->errors_ = fmt::format("can't find request_id:{}", request_id);
        return false;
    }
    return handle->internalCallResultMap.at(request_id).isPending();
}
bool faas_result_is_complete(FaasHandle* handle, uint64_t request_id)
{
    wukong::utils::UniqueRecursiveLock lock(handle->mutex);
    if (!handle->internalCallResultMap.contains(request_id))
    {
        auto msg        = fmt::format("can't find request_id:{}", request_id);
        handle->errors_ = msg;
        return false;
    }
    return handle->internalCallResultMap.at(request_id).isSettled();
}
// TODO UUID不应该与实际shm的路径一致，否则就是直接暴露给用户了
bool faas_create_shm(FaasHandle* handle, size_t length, std::string& uuid, void** addr)
{
    *addr = nullptr;
    wukong::utils::UniqueRecursiveLock lock(handle->mutex);
    std::string funcname = fmt::format("{}/{}", STORAGE_FUNCTION_NAME, StorageFuncOpTypeName[StorageFuncOpType::Create]);
    uint64_t request_id  = wukong::utils::uuid();
    WK_FAAS_FUNC_CHECK(faas_call(handle, funcname, std::to_string(length), &request_id), "faas_create_shm.faas_call", handle->errors());
    std::string result;
    WK_FAAS_FUNC_CHECK(faas_get_call_result(handle, request_id, result), "faas_create_shm.faas_get_call_result", handle->errors());
    wukong::utils::Json json_data(result);
    uuid           = json_data.get("uuid");
    auto check_ret = ShareMemoryObject::uuid_check(uuid);
    WK_FAAS_FUNC_CHECK(check_ret.first, "faas_create_shm.uuid_check", check_ret.second);
    auto ret = ShareMemoryObject::open(uuid, length, addr, true);
    WK_FAAS_FUNC_CHECK(ret.first, "faas_create_shm.ShareMemoryObject::open", ret.second);
    WK_CHECK_WITH_EXIT(*addr != nullptr, "Unreachable");
    handle->shmMap.emplace(uuid, std::make_pair(*addr, length));
    return true;
}
bool faas_get_shm(FaasHandle* handle, const std::string& uuid, size_t length, void** addr)
{
    wukong::utils::UniqueRecursiveLock lock(handle->mutex);
    if (handle->shmMap.contains(uuid))
    {
        const auto item = handle->shmMap.at(uuid);
        WK_FAAS_FUNC_CHECK(item.second == length, "faas_get_shm",
                           fmt::format("The length {} in Parameter is inconsistent with that cached in the shmMap {}",
                                       length, item.second));
        *addr = item.first;
        return true;
    }
    auto ret = ShareMemoryObject::open(uuid, length, addr, false);
    WK_FAAS_FUNC_CHECK(ret.first, "faas_get_shm.ShareMemoryObject::open", ret.second);
    WK_CHECK_WITH_EXIT(*addr != nullptr, "Unreachable");
    handle->shmMap.emplace(uuid, std::make_pair(*addr, length));
    return true;
}
bool faas_delete_shm(FaasHandle* handle, const std::string& uuid)
{
    wukong::utils::UniqueRecursiveLock lock(handle->mutex);
    WK_FAAS_FUNC_CHECK(handle->shmMap.contains(uuid), "faas_delete_shm", fmt::format("uuid {} is not exists in shmMap", uuid));
    std::string funcname = fmt::format("{}/{}", STORAGE_FUNCTION_NAME, StorageFuncOpTypeName[StorageFuncOpType::Delete]);
    uint64_t request_id;
    WK_FAAS_FUNC_CHECK(faas_call(handle, funcname, uuid, &request_id), "faas_delete_shm.faas_call", handle->errors());
    return true;
}