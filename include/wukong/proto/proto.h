//
// Created by kingdo on 2022/2/26.
//

#ifndef WUKONG_PROTO_H
#define WUKONG_PROTO_H

#include <wukong.pb.h>

namespace wukong::proto {
    std::string messageToJson(const wukong::proto::Message &msg);

    std::string getJsonOutput(const wukong::proto::Message &msg);

    wukong::proto::Message jsonToMessage(const std::string &jsonIn);


//    std::string messageToJson(const ReplyRegisterInvoker &msg);
//
//    wukong::proto::ReplyRegisterInvoker jsonToReplyRegisterInvoker(const std::string &jsonIn);


    std::string messageToJson(const Invoker &msg);

    wukong::proto::Invoker jsonToInvoker(const std::string &jsonIn);

    wukong::proto::Invoker hashToInvoker(const std::unordered_map<std::string, std::string> &hash);

    std::unordered_map<std::string, std::string> messageToHash(const wukong::proto::Invoker &invoker);

    std::unordered_map<std::string, std::string> messageToHash(const User &user);

    std::string messageToJson(const User &user);

    std::unordered_map<std::string, std::string> messageToHash(const Application &application);

    std::string messageToJson(const Application &application);

    std::unordered_map<std::string, std::string> messageToHash(const Function &function);

    wukong::proto::Function hashToFunction(const std::unordered_map<std::string, std::string> &hash);

    std::string messageToJson(const Function &function);

    wukong::proto::User hashToUser(const std::unordered_map<std::string, std::string> &hash);

    wukong::proto::Application hashToApplication(const std::unordered_map<std::string, std::string> &hash);

    wukong::proto::Application jsonToApplication(const std::string &jsonIn);

    wukong::proto::User jsonToUser(const std::string &jsonIn);

}

#endif //WUKONG_PROTO_H
