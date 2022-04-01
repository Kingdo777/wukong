//
// Created by kingdo on 2022/2/26.
//

#ifndef WUKONG_PROTO_H
#define WUKONG_PROTO_H

#include <wukong.pb.h>
#include <wukong/utils/json.h>
#include <wukong/utils/timing.h>
#include <wukong/utils/uuid.h>

namespace wukong::proto
{
    const std::array<std::string, 2> FunctionTypeName = {
        "c/cpp",
        "python"
    };
    const std::unordered_map<std::string, proto::Function_FunctionType> FunctionTypeNameMAP = {
        { std::make_pair(FunctionTypeName[Function_FunctionType_C_CPP], Function_FunctionType_C_CPP) },
        { std::make_pair(FunctionTypeName[Function_FunctionType_PYTHON], Function_FunctionType_PYTHON) }
    };

    /// _______________________ For Message __________________________
    std::string messageToJson(const wukong::proto::Message& msg);

    std::string getJsonOutput(const wukong::proto::Message& msg);

    wukong::proto::Message jsonToMessage(const std::string& jsonIn);

    //    std::string messageToJson(const ReplyRegisterInvoker &msg);
    //
    //    wukong::proto::ReplyRegisterInvoker jsonToReplyRegisterInvoker(const std::string &jsonIn);
    /// _____________ For Invoker _______________________
    std::string messageToJson(const Invoker& msg);

    wukong::proto::Invoker jsonToInvoker(const std::string& jsonIn);

    wukong::proto::Invoker hashToInvoker(const std::unordered_map<std::string, std::string>& hash);

    std::unordered_map<std::string, std::string> messageToHash(const wukong::proto::Invoker& invoker);

    /// _____________ For User _______________________
    wukong::proto::User hashToUser(const std::unordered_map<std::string, std::string>& hash);

    std::unordered_map<std::string, std::string> messageToHash(const User& user);

    std::string messageToJson(const User& user);

    wukong::proto::User jsonToUser(const std::string& jsonIn);

    /// _____________ For Application _______________________
    wukong::proto::Application hashToApplication(const std::unordered_map<std::string, std::string>& hash);

    std::string messageToJson(const Application& application);

    std::unordered_map<std::string, std::string> messageToHash(const Application& application);

    wukong::proto::Application jsonToApplication(const std::string& jsonIn);

    /// _____________ For Function _______________________
    std::string messageToJson(const Function& function);

    std::unordered_map<std::string, std::string> messageToHash(const Function& function);

    wukong::proto::Function jsonToFunction(const std::string& jsonIn);

    wukong::proto::Function hashToFunction(const std::unordered_map<std::string, std::string>& hash);

    /// _____________ For Instance _______________________

    std::string messageToJson(const Instance& instance);

    wukong::proto::Instance jsonToInstance(const std::string& jsonIn);

    std::string messageToJson(const ReplyStartupInstance& instance);

    wukong::proto::ReplyStartupInstance jsonToReplyStartupInstance(const std::string& jsonIn);
}

#endif //WUKONG_PROTO_H
