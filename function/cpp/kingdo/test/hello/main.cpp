//
// Created by kingdo on 2022/3/14.
//

#include <cstdio>
#include <string>
#include <faas/function-interface.h>

void faas_main(FaasHandle *handle) {
    auto input = faas_getInput(handle);
    auto result = fmt::format("Hello , {}", input);
    faas_setOutput(handle, result);
}