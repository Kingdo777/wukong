//
// Created by kingdo on 2022/2/27.
//

#include "endpoint.h"

GlobalGatewayEndpoint::GlobalGatewayEndpoint(
        const std::string &name,
        const std::shared_ptr<LoadBalance> &lb_,
        const std::shared_ptr<GlobalGatewayHandler> &handler) :
        Endpoint(name, handler),
        lb(lb_) {
    handler->associateEndpoint(this);
}

void GlobalGatewayEndpoint::start() {
    lb->start();

    //TODO

    BASE::start();
}

void GlobalGatewayEndpoint::stop() {
    // TODO

    BASE::stop();
    lb->stop();
}
