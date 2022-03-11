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

    std::unordered_map<std::string, std::string> invokerToHash(const wukong::proto::Invoker &invoker);

}

#endif //WUKONG_PROTO_H
