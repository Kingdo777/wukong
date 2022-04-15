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

int faas_delete_shm_py(FaasHandle* handle, const char* uuid);

void* faas_get_shm_py(FaasHandle* handle, const char* uuid, size_t* length = nullptr);

size_t faas_read_shm_py(FaasHandle* handle, const char* uuid, size_t offset, char* read_buffer, size_t read_buffer_size);

void* faas_create_shm_py(FaasHandle* handle, size_t length, char* uuid_buffer, size_t uuid_buffer_size);

int faas_get_call_result_py(FaasHandle* handle, uint64_t request_id, char* result_buffer, size_t result_buffer_size);

uint64_t faas_chain_call_py(FaasHandle* handle, const char* funcname, const char* input, size_t input_size);

size_t faas_uuid_size_py();
size_t faas_result_size_py();
size_t faas_shm_size_py(FaasHandle* handle, const char* uuid);

void interface_link();
}
#endif // WUKONG_FUNCTION_INTERFACE_PYTHON_H
