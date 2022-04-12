//
// Created by kingdo on 2022/3/29.
//

#ifndef WUKONG_FUNCTION_INTERFACE_H
#define WUKONG_FUNCTION_INTERFACE_H

#include <pistache/async.h>
#include <utility>
#include <wukong/proto/proto.h>
#include <wukong/utils/locks.h>
#include <wukong/utils/shm/ShareMemoryObject.h>

#define WK_FAAS_FUNC_HANDLE_RET(funcname, ret)                                                     \
    do                                                                                             \
    {                                                                                              \
        if (!(ret))                                                                                \
        {                                                                                          \
            std::string msg = fmt::format("Failed to Exec `{}` : {}", funcname, handle->errors()); \
            SPDLOG_ERROR(msg);                                                                     \
            faas_setOutput(handle, msg);                                                           \
            return;                                                                                \
        }                                                                                          \
    } while (false)
#define WK_FAAS_FUNC(func) WK_FAAS_FUNC_HANDLE_RET(#func, func(handle))
#define WK_FAAS_FUNC_1(func, a1) WK_FAAS_FUNC_HANDLE_RET(#func, func(handle, a1))
#define WK_FAAS_FUNC_2(func, a1, a2) WK_FAAS_FUNC_HANDLE_RET(#func, func(handle, a1, a2))
#define WK_FAAS_FUNC_3(func, a1, a2, a3) WK_FAAS_FUNC_HANDLE_RET(#func, func(handle, a1, a2, a3))

class WorkerFuncAgent;
std::string faas_ping();

struct FaasHandle
{
    FaasHandle(std::shared_ptr<wukong::proto::Message> msg_pyr_, WorkerFuncAgent* agent_)
        : msg_ptr(std::move(msg_pyr_))
        , agent(agent_)
    { }

    void set_errors(const std::string& error)
    {
        errors_ = error;
    }
    std::string errors() const
    {
        return errors_;
    }

    std::string who() const
    {
        return msg_ptr->function();
    }

    std::shared_ptr<wukong::proto::Message> msg_ptr;
    std::unordered_map<uint64_t, Pistache::Async::Promise<std::string>> internalCallResultMap;
    std::unordered_map<std::string, std::pair<void*, size_t>> shmMap;
    std::recursive_mutex mutex; // 确保线程安全
    std::string errors_; // 打印错误信息
    WorkerFuncAgent* agent;
};

bool faas_getInput(FaasHandle* handle, std::string&);

bool faas_setOutput(FaasHandle* handle, const std::string& result);

bool faas_call(FaasHandle* handle, const std::string& funcname, const std::string& args, uint64_t* request_id);

bool faas_get_call_result(FaasHandle* handle, uint64_t request_id, std::string& result);

bool faas_result_is_success(FaasHandle* handle, uint64_t request_id);
bool faas_result_is_failed(FaasHandle* handle, uint64_t request_id);
bool faas_result_is_pending(FaasHandle* handle, uint64_t request_id);
bool faas_result_is_complete(FaasHandle* handle, uint64_t request_id);

bool faas_delete_shm(FaasHandle* handle, const std::string& uuid);
bool faas_create_shm(FaasHandle* handle, size_t length, std::string& uuid, void** addr);
bool faas_get_shm(FaasHandle* handle, const std::string& uuid, size_t length, void** addr);

#endif // WUKONG_FUNCTION_INTERFACE_H
