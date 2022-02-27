//
// Created by kingdo on 2022/2/27.
//

#ifndef WUKONG_GLOBAL_GW_HANDLER_H
#define WUKONG_GLOBAL_GW_HANDLER_H

#include <wukong/endpoint/endpoint.h>

class GlobalGatewayHandler : public Pistache::Http::Handler {
public:
    HTTP_PROTOTYPE(GlobalGatewayHandler)

    void onRequest(const Pistache::Http::Request & /*request*/, Pistache::Http::ResponseWriter response) override;

    void onTimeout(const Pistache::Http::Request & /*req*/,
                   Pistache::Http::ResponseWriter response) override;

private:
    void handleGetReq(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response);

    void handlePostReq(const Pistache::Http::Request &, Pistache::Http::ResponseWriter);
};

#endif //WUKONG_GLOBAL_GW_HANDLER_H
