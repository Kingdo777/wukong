//
// Created by kingdo on 2022/4/13.
//

#include <faas/python/function-interface.h>

extern "C" {

size_t faas_getInputSize_py(FaasHandle* handle)
{
    return faas_getInputSize(handle);
}
int faas_getInput_py(FaasHandle* handle, char* buff, size_t size)
{
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
    std::string output { buff, size };
    faas_setOutput(handle, output);
    return 0;
}

}
void interface_link() {}
