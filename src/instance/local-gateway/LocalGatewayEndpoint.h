//
// Created by kingdo on 2022/3/23.
//

#ifndef WUKONG_LOCAL_GATEWAY_ENDPOINT_H
#define WUKONG_LOCAL_GATEWAY_ENDPOINT_H

#include <boost/filesystem.hpp>
#include <wukong/client/client-server.h>
#include <wukong/endpoint/endpoint.h>
#include <wukong/utils/dl.h>
#include <wukong/utils/redis.h>

#include "LocalGatewayClientServer.h"

class LocalGatewayEndpoint;

class LocalGateway;

class LocalGatewayHandler : public Pistache::Http::Handler
{
public:
    HTTP_PROTOTYPE(LocalGatewayHandler)

    void onRequest(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter response) override;

    void onTimeout(const Pistache::Http::Request& /*req*/,
                   Pistache::Http::ResponseWriter response) override;

    void associateEndpoint(LocalGatewayEndpoint* e_)
    {
        this->e = e_;
    }

    LocalGatewayEndpoint* endpoint()
    {
        return this->e;
    }

private:
    void handleGetReq(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter response);

    void handlePostReq(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter response);

    LocalGatewayEndpoint* e;
};

class LocalGatewayEndpoint : public wukong::endpoint::Endpoint
{

public:
    explicit LocalGatewayEndpoint(LocalGateway* lg_,
                                  const std::string& name                             = "local-gateway-endpoint",
                                  const std::shared_ptr<LocalGatewayHandler>& handler = std::make_shared<LocalGatewayHandler>())
        : Endpoint(name, handler)
        , lg(lg_)
    {
        handler->associateEndpoint(this);
    }

    LocalGateway* lg;
};

#endif //WUKONG_LOCAL_GATEWAY_ENDPOINT_H
