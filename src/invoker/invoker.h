//
// Created by kingdo on 2022/3/9.
//

#ifndef WUKONG_INVOKER_H
#define WUKONG_INVOKER_H

#include <wukong/proto/proto.h>
#include "invokerClientServer.h"
#include "invokerEndpoint.h"

class Invoker {

public:

    Invoker();

    void start();

    void stop();

    wukong::proto::Invoker invokerProto;

    std::string toInvokerJson() const {
        return wukong::proto::messageToJson(invokerProto);
    }


private:

    enum InvokerStatus {
        Uninitialized,
        Running,
        Stopped
    };

    InvokerStatus status = Uninitialized;

    InvokerClientServer client;
    InvokerEndpoint endpoint;
};


#endif //WUKONG_INVOKER_H
