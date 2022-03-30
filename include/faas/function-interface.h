//
// Created by kingdo on 2022/3/29.
//

#ifndef WUKONG_FUNCTION_INTERFACE_H
#define WUKONG_FUNCTION_INTERFACE_H

#include <wukong/proto/proto.h>

std::string faas_ping();

struct FaasHandle {
    FaasHandle(const std::shared_ptr<wukong::proto::Message> &msg_pyr_) : msg_ptr(msg_pyr_) {}

    std::shared_ptr<wukong::proto::Message> msg_ptr;
};

std::string faas_getInput(FaasHandle *handle);

void faas_setOutput(FaasHandle *handle, const std::string &result);

#endif //WUKONG_FUNCTION_INTERFACE_H
