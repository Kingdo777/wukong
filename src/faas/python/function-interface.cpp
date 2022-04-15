//
// Created by kingdo on 2022/4/13.
//

#include <faas/python/function-interface.h>

extern "C" {

bool check_handle(FaasHandle* handle)
{
    return handle && MAGIC_NUMBER_CHECK(handle->magic_number);
}

size_t faas_getInputSize_py(FaasHandle* handle)
{
    if (!check_handle(handle))
    {
        SPDLOG_ERROR("Handle Wrong!");
        return 1;
    }
    return faas_getInputSize(handle);
}
int faas_getInput_py(FaasHandle* handle, char* buff, size_t size)
{
    if (!check_handle(handle))
    {
        SPDLOG_ERROR("Handle Wrong!");
        return 1;
    }
    std::string input;
    faas_getInput(handle, input);
    size_t data_size = input.size();
    if (size < data_size)
    {
        SPDLOG_WARN("given Size `{}` is less than Input Dada Size `{}`", size, data_size);
        memcpy(buff, handle->msg_ptr->inputdata().data(), size);
        return 0;
    }
    if (size > data_size)
    {
        SPDLOG_WARN("given Size `{}` is greater than Input Dada Size `{}`", size, data_size);
    }
    memcpy(buff, handle->msg_ptr->inputdata().data(), data_size);
    return 0;
}
int faas_setOutput_py(FaasHandle* handle, char* buff, size_t size)
{
    if (!check_handle(handle))
    {
        SPDLOG_ERROR("Handle Wrong!");
        return 1;
    }
    std::string output { buff, size };
    faas_setOutput(handle, output);
    return 0;
}

uint64_t faas_chain_call_py(FaasHandle* handle,
                            const char* funcname,
                            const char* input, size_t input_size)
{
    if (!check_handle(handle))
    {
        SPDLOG_ERROR("Handle Wrong!");
        return 1;
    }
    uint64_t request_id;
    bool success = faas_call(handle,
                             std::string { funcname },
                             std::string { input, input_size },
                             &request_id);
    if (!success)
    {
        SPDLOG_ERROR("faas_chain_call_py failed ： {}", handle->errors());
        return 1;
    }
    return request_id;
}

int faas_get_call_result_py(FaasHandle* handle,
                            uint64_t request_id,
                            char* result_buffer, size_t result_buffer_size)
{
    if (!check_handle(handle))
    {
        SPDLOG_ERROR("Handle Wrong!");
        return 1;
    }
    std::string result;
    bool success = faas_get_call_result(handle, request_id, result);
    if (!success)
    {
        SPDLOG_ERROR("faas_get_call_result_py failed ： {}", handle->errors());
        return 1;
    }
    size_t data_size = result.size();

    if (result_buffer_size < data_size)
    {
        SPDLOG_WARN("given Size `{}` is less than Result Dada Size `{}`", result_buffer_size, data_size);
        memcpy(result_buffer, result.data(), result_buffer_size);
    }
    strcpy(result_buffer, result.data());
    return 0;
}

size_t faas_uuid_size_py()
{
    return ShareMemoryObject::uuid_size();
}

size_t faas_result_size_py()
{
    return WUKONG_MESSAGE_SIZE;
}

void* faas_create_shm_py(FaasHandle* handle,
                         size_t length,
                         char* uuid_buffer, size_t uuid_buffer_size)
{
    if (!check_handle(handle))
    {
        SPDLOG_ERROR("Handle Wrong!");
        return nullptr;
    }
    size_t recommend_uuid_size = faas_uuid_size_py();
    if (uuid_buffer_size < recommend_uuid_size)
    {
        SPDLOG_ERROR("given uuid-size `{}` is less than the recommend value `{}`", uuid_buffer_size, recommend_uuid_size);
        return nullptr;
    }
    void* addr;
    std::string uuid;
    bool success = faas_create_shm(handle, length, uuid, &addr);
    if (!success)
    {
        SPDLOG_ERROR("faas_create_shm_py failed ： {}", handle->errors());
        return nullptr;
    }
    memcpy(uuid_buffer, uuid.data(), recommend_uuid_size);
    return addr;
}

size_t faas_shm_size_py(FaasHandle* handle, const char* uuid)
{
    if (!check_handle(handle))
    {
        SPDLOG_ERROR("Handle Wrong!");
        return 0;
    }
    size_t length;
    bool success = faas_get_shm(handle, std::string { uuid }, nullptr, &length);
    if (!success)
    {
        SPDLOG_ERROR("faas_get_shm_py failed ： {}", handle->errors());
        return 0;
    }
    return length;
}

void* faas_get_shm_py(FaasHandle* handle,
                      const char* uuid, size_t* length)
{
    if (!check_handle(handle))
    {
        SPDLOG_ERROR("Handle Wrong!");
        return nullptr;
    }
    void* addr;
    bool success = faas_get_shm(handle, std::string { uuid }, &addr, length);
    if (!success)
    {
        SPDLOG_ERROR("faas_get_shm_py failed ： {}", handle->errors());
        return nullptr;
    }
    return addr;
}

size_t faas_read_shm_py(FaasHandle* handle,
                        const char* uuid,
                        size_t offset,
                        char* read_buffer, size_t read_buffer_size)
{
    if (!check_handle(handle))
    {
        SPDLOG_ERROR("Handle Wrong!");
        return 0;
    }
    size_t shm_size;
    void* addr = faas_get_shm_py(handle, uuid, &shm_size);
    if (offset > shm_size)
    {
        SPDLOG_ERROR("faas_read_shm_py : offset `{}` > shm_size `{}` ", offset, shm_size);
        return 0;
    }
    if (offset + read_buffer_size > shm_size)
    {
        SPDLOG_WARN("faas_read_shm_py : offset `{}` + read_buffer_size `{}` > shm_size `{}` ", offset, read_buffer_size, shm_size);
        read_buffer_size = shm_size - offset;
    }
    memcpy(read_buffer, addr, read_buffer_size);
    return read_buffer_size;
}

size_t faas_write_shm_py(FaasHandle* handle,
                         const char* uuid,
                         size_t offset,
                         const char* write_data, size_t write_data_size)
{
    if (!check_handle(handle))
    {
        SPDLOG_ERROR("Handle Wrong!");
        return 0;
    }
    size_t shm_size;
    void* addr = faas_get_shm_py(handle, uuid, &shm_size);
    if (offset > shm_size)
    {
        SPDLOG_ERROR("faas_read_shm_py : offset `{}` > shm_size `{}` ", offset, shm_size);
        return 0;
    }
    if (offset + write_data_size > shm_size)
    {
        SPDLOG_WARN("faas_read_shm_py : offset `{}` +  `{}` > shm_size `{}` ", offset, write_data_size, shm_size);
        write_data_size = shm_size - offset;
    }
    memcpy(addr, write_data, write_data_size);
    return write_data_size;
}

int faas_delete_shm_py(FaasHandle* handle,
                       const char* uuid)
{
    if (!check_handle(handle))
    {
        SPDLOG_ERROR("Handle Wrong!");
        return 1;
    }
    bool success = faas_delete_shm(handle, std::string { uuid });
    if (!success)
    {
        SPDLOG_ERROR("faas_delete_shm_py failed ： {}", handle->errors());
        return 1;
    }
    return 0;
}
}
void interface_link() { }
