//
// Created by kingdo on 2022/3/23.
//

#ifndef WUKONG_LOCAL_GATEWAY_ENDPOINT_H
#define WUKONG_LOCAL_GATEWAY_ENDPOINT_H

#include <wukong/endpoint/endpoint.h>
#include <wukong/client/client-server.h>

#include "LocalGatewayClientServer.h"

class LocalGatewayEndpoint;

class LocalGatewayHandler : public Pistache::Http::Handler {
public:
HTTP_PROTOTYPE(LocalGatewayHandler)

    void onRequest(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response) override {
        // Very permissive CORS
        response.headers().add<Pistache::Http::Header::AccessControlAllowOrigin>(
                "*");
        response.headers().add<Pistache::Http::Header::AccessControlAllowMethods>(
                "GET,POST,PUT,OPTIONS");
        response.headers().add<Pistache::Http::Header::AccessControlAllowHeaders>(
                "User-Agent,Content-Type");
        // Text response type
        response.headers().add<Pistache::Http::Header::ContentType>(
                Pistache::Http::Mime::MediaType("text/plain"));

        switch (request.method()) {
            case Pistache::Http::Method::Get:
                handleGetReq(request, std::move(response));
                break;
            case Pistache::Http::Method::Post:
                handlePostReq(request, std::move(response));
                break;
            default:
                response.send(Pistache::Http::Code::Method_Not_Allowed, "Only GET && POST Method Allowed\n");
                break;
        }
    }

    void onTimeout(const Pistache::Http::Request & /*req*/,
                   Pistache::Http::ResponseWriter response) override {
        response
                .send(Pistache::Http::Code::Request_Timeout, "Timeout")
                .then([=](ssize_t) {}, PrintException());
    }

    void associateEndpoint(LocalGatewayEndpoint *e_) {
        this->e = e_;
    }

    LocalGatewayEndpoint *endpoint() {
        return this->e;
    }

private:
    void handleGetReq(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response) {
        const std::string &uri = request.resource();
        if (uri == "/ping") {
            SPDLOG_DEBUG("LocalGatewayHandler received ping request");
            response.send(Pistache::Http::Code::Ok, "PONG");
            return;
        }
        SPDLOG_WARN(fmt::format("LocalGatewayHandler received Unsupported request ： {}", uri));
        response.send(Pistache::Http::Code::Bad_Request, fmt::format("Unsupported request : {}", uri));
    }

    void handlePostReq(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response) {
        const std::string &uri = request.resource();
        if (uri == "/ping") {
            SPDLOG_DEBUG("LocalGatewayHandler received ping request");
            response.send(Pistache::Http::Code::Ok, "PONG");
            return;
        }

        if (uri == "/init") {
            SPDLOG_DEBUG("LocalGatewayHandler received ping request");
            for (auto c: request.cookies()) {
                SPDLOG_INFO("{}={}", c.name, c.value);
            }
            response.send(Pistache::Http::Code::Ok, "ok");
            return;
        }

        if (uri.starts_with("/function")) {
            if (uri.starts_with("/function/call")) {
                response.send(Pistache::Http::Code::Ok, "func call test Success!");
                return;
            }
        }

        SPDLOG_WARN(fmt::format("LocalGatewayHandler received Unsupported request ： {}", uri));
        response.send(Pistache::Http::Code::Bad_Request, fmt::format("Unsupported request : {}", uri));
    }

    LocalGatewayEndpoint *e;
};


class LocalGatewayEndpoint : public wukong::endpoint::Endpoint {

public:

    explicit LocalGatewayEndpoint(const std::string &name = "local-gateway-endpoint",
                                  const std::shared_ptr<LocalGatewayHandler> &handler = std::make_shared<LocalGatewayHandler>())
            : Endpoint(name, handler) {
        handler->associateEndpoint(this);
    }

    void start() override {
        /// start Client Server
        auto opts = Pistache::Http::Client::options().
                threads(wukong::utils::Config::ClientNumThreads()).
                maxConnectionsPerHost(wukong::utils::Config::ClientMaxConnectionsPerHost());
        cs.setHandler(std::make_shared<LocalGatewayClientHandler>(&cs));
        SPDLOG_INFO("Startup LocalGatewayClientServer with {} threads", wukong::utils::Config::ClientNumThreads());
        cs.start(opts);
        Endpoint::start();
    }

    void stop() override {
        Endpoint::stop();
        cs.shutdown();
    }

    wukong::client::ClientServer cs;

};


#endif //WUKONG_LOCAL_GATEWAY_ENDPOINT_H
