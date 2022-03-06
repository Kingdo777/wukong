//
// Created by kingdo on 2022/2/27.
//

#ifndef WUKONG_GLOBAL_GW_ENDPOINT_H
#define WUKONG_GLOBAL_GW_ENDPOINT_H

#include <wukong/endpoint/endpoint.h>
#include "load-balance.h"

class GlobalGatewayEndpoint;

class GlobalGatewayHandler : public Pistache::Http::Handler {
public:
HTTP_PROTOTYPE(GlobalGatewayHandler)

    void onRequest(const Pistache::Http::Request & /*request*/, Pistache::Http::ResponseWriter response) override;

    void onTimeout(const Pistache::Http::Request & /*req*/,
                   Pistache::Http::ResponseWriter response) override;

    void associateEndpoint(GlobalGatewayEndpoint *e_) {
        this->e = e_;
    }

    GlobalGatewayEndpoint *endpoint() {
        return this->e;
    }

private:
    void handleGetReq(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response);

    void handlePostReq(const Pistache::Http::Request &, Pistache::Http::ResponseWriter);

    GlobalGatewayEndpoint *e;
};

class GlobalGatewayEndpoint : public wukong::endpoint::Endpoint {
public:

    typedef wukong::endpoint::Endpoint BASE;

    explicit GlobalGatewayEndpoint(const std::string &name = "global-gateway-endpoint",
                                   std::shared_ptr<LoadBalance> lb = std::make_shared<LoadBalance>(),
                                   const std::shared_ptr<GlobalGatewayHandler> &handler = std::make_shared<GlobalGatewayHandler>());

    std::shared_ptr<LoadBalance> lb;

    void start() override;

    void stop() override;
};

#endif //WUKONG_GLOBAL_GW_ENDPOINT_H
