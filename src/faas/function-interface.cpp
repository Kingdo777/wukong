//
// Created by kingdo on 2022/3/29.
//

#include "faas/function-interface.h"

std::string faas_ping()
{
    return "pong";
}

std::string faas_getInput(FaasHandle* handle)
{
    return handle->msg_ptr->inputdata();
}

void faas_setOutput(FaasHandle* handle, const std::string& result)
{
    handle->msg_ptr->set_outputdata(result);
}
