//
// Created by kingdo on 2022/4/13.
//

#ifndef WUKONG_FUNCTION_INTERFACE_PYTHON_H
#define WUKONG_FUNCTION_INTERFACE_PYTHON_H

#include <faas/cpp/function-interface.h>
extern "C" {
size_t faas_getInputSize_py(FaasHandle* handle);

// TODO 这里复制了两次
int faas_getInput_py(FaasHandle* handle, char* buff, size_t size);

// TODO 这里复制了两次
int faas_setOutput_py(FaasHandle* handle, char* buff, size_t size);

void interface_link();
}
#endif // WUKONG_FUNCTION_INTERFACE_PYTHON_H
