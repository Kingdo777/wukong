//
// Created by kingdo on 2022/3/9.
//

#ifndef WUKONG_INVOKERENDPOINT_H
#define WUKONG_INVOKERENDPOINT_H

#include <wukong/endpoint/endpoint.h>

class InvokerHandler : public Pistache::Http::Handler {
public:
HTTP_PROTOTYPE(InvokerHandler)

    void onRequest(const Pistache::Http::Request & /*request*/, Pistache::Http::ResponseWriter response) override;

    void onTimeout(const Pistache::Http::Request & /*req*/,
                   Pistache::Http::ResponseWriter response) override;


private:

};

class InvokerEndpoint : public wukong::endpoint::Endpoint {
public:

    typedef wukong::endpoint::Endpoint BASE;

    explicit InvokerEndpoint(const std::string &name = "invoker-endpoint",
                                   const std::shared_ptr<InvokerHandler> &handler = std::make_shared<InvokerHandler>());

    void start() override;

    void stop() override;
};

#endif //WUKONG_INVOKERENDPOINT_H
