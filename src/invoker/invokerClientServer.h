//
// Created by kingdo on 2022/3/9.
//

#ifndef WUKONG_INVOKERCLIENTSERVER_H
#define WUKONG_INVOKERCLIENTSERVER_H

#include <wukong/client/client-server.h>
#include <wukong/utils/log.h>
#include <wukong/utils/json.h>


class InvokerClientServer : public wukong::client::ClientServer {
    typedef wukong::client::ClientServer Base;
public:
    std::pair<bool, std::string> register2LB(const std::string& invokerJson);

    bool registered = false;
};


#endif //WUKONG_INVOKERCLIENTSERVER_H
