//
// Created by kingdo on 2022/3/14.
//

#include <faas/function-interface.h>
#include <string>

void faas_main(FaasHandle* handle)
{
    auto id = faas_call(handle, "who", "");
    std::string result;
    bool success = faas_get_call_result(handle, id, result);
    if (success)
    {
        faas_setOutput(handle, fmt::format("Hello , {}", result));
    }
    else
    {
        std::string msg = fmt::format("failed call internal-function : ", result);
        SPDLOG_ERROR(msg);
        faas_setOutput(handle, msg);
    }
}