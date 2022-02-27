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

}

#endif //WUKONG_PROTO_H
