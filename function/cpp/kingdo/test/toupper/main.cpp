//
// Created by kingdo on 2022/3/14.
//

#include <cstdio>
#include <faas/function-interface.h>
#include <string>

void faas_main(FaasHandle* handle)
{
    std::string data;
    faas_getInput(handle, data);
    wukong::utils::Json json_data(data);
    std::string uuid = json_data.get("uuid");
    size_t length    = json_data.getUInt64("length", 0);
    char* s;
    WK_FAAS_FUNC_3(faas_get_shm, uuid, length, reinterpret_cast<void**>(&s));
    std::string result { s, length };
    WK_FAAS_FUNC_1(faas_delete_shm, uuid);
    for (auto& ch : result)
    {
        ch = (char)toupper(ch);
    }
    faas_setOutput(handle, result);
}