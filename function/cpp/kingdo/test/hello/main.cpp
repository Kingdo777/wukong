//
// Created by kingdo on 2022/3/14.
//

#include <faas/function-interface.h>
#include <string>

void faas_main(FaasHandle* handle)
{
    uint64_t requestID;
    SPDLOG_DEBUG("faas_call who");
    WK_FAAS_FUNC_3(faas_call, "who", "", &requestID);
    std::string result;
    SPDLOG_DEBUG("wait-result who");
    WK_FAAS_FUNC_2(faas_get_call_result, requestID, result);
    char* s;
    std::string uuid;
    size_t length = result.size();
    SPDLOG_DEBUG("create_shm");
    WK_FAAS_FUNC_3(faas_create_shm, length, uuid, reinterpret_cast<void**>(&s));
    strcpy(s, result.c_str());
    wukong::utils::Json json_data;
    json_data.set("uuid", uuid);
    json_data.setUInt64("length", length);
    SPDLOG_DEBUG("faas_call toupper");
    WK_FAAS_FUNC_3(faas_call, "toupper", json_data.serialize(), &requestID);
    SPDLOG_DEBUG("wait-result toupper");
    WK_FAAS_FUNC_2(faas_get_call_result, requestID, result);
    SPDLOG_DEBUG("faas_setOutput {}", result);
    faas_setOutput(handle, fmt::format("Hello , {}", result));
    SPDLOG_DEBUG("Done");
}