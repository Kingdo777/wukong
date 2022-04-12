//
// Created by kingdo on 2022/3/9.
//

#ifndef WUKONG_INVOKERENDPOINT_H
#define WUKONG_INVOKERENDPOINT_H

#include <wukong/endpoint/endpoint.h>
#include <wukong/proto/proto.h>
#include <wukong/utils/macro.h>

class Invoker;

class InvokerEndpoint;

class InvokerHandler : public Pistache::Http::Handler
{
public:
    HTTP_PROTOTYPE(InvokerHandler)

    void onRequest(const Pistache::Http::Request& /*request*/, Pistache::Http::ResponseWriter response) override;

    void onTimeout(const Pistache::Http::Request& /*req*/,
                   Pistache::Http::ResponseWriter response) override;

    void associateEndpoint(InvokerEndpoint* e_)
    {
        this->e = e_;
    }

    InvokerEndpoint* endpoint()
    {
        return this->e;
    }

private:
    InvokerEndpoint* e;

    void handleGetReq(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter response);

    void handlePostReq(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter response);
};

class InvokerEndpoint : public wukong::endpoint::Endpoint
{
public:
    typedef wukong::endpoint::Endpoint BASE;

    explicit InvokerEndpoint(const std::string& name                        = "invoker-endpoint",
                             const std::shared_ptr<InvokerHandler>& handler = std::make_shared<InvokerHandler>());

    void start() override;

    void stop() override;

    void associateEndpoint(Invoker* i)
    {
        this->ink = i;
    }

    Invoker* invoker()
    {
        WK_CHECK_WITH_EXIT(ink != nullptr, "invoker is NULL");
        return ink;
    }

private:
    Invoker* ink = nullptr;
};

#endif //WUKONG_INVOKERENDPOINT_H
