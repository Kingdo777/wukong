//
// Created by kingdo on 2022/3/14.
//

#include <cstdio>
#include <faas/cpp/function-interface.h>
#include <string>

void faas_main(FaasHandle* handle)
{
    faas_setOutput(handle, "kingdo");
}