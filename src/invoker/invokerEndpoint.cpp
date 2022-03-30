//
// Created by kingdo on 2022/3/9.
//

#include "invokerEndpoint.h"
#include "invoker.h"

void InvokerHandler::onRequest(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response) {
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

void InvokerHandler::onTimeout(const Pistache::Http::Request &, Pistache::Http::ResponseWriter response) {
    response.send(Pistache::Http::Code::Request_Timeout, "Timeout")
            .then([=](ssize_t) {}, PrintException());
}

void InvokerHandler::handleGetReq(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response) {
    const std::string &uri = request.resource();
    if (uri == "/ping") {
        SPDLOG_DEBUG("GlobalGatewayHandler received ping request");
        response.send(Pistache::Http::Code::Ok, "PONG");
        return;
    }
    SPDLOG_WARN(fmt::format("GlobalGatewayHandler received Unsupported request ： {}", uri));
    response.send(Pistache::Http::Code::Bad_Request, fmt::format("Unsupported request : {}", uri));
}

void InvokerHandler::handlePostReq(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response) {
    const std::string &uri = request.resource();
    if (uri == "/ping") {
        SPDLOG_DEBUG("GlobalGatewayHandler received ping request");
        response.send(Pistache::Http::Code::Ok, "PONG");
        return;
    }
    if (uri.starts_with("/instance")) {
        if (uri == "/instance/startup") {
            SPDLOG_DEBUG("GlobalGatewayHandler received /function/startup request");
            wukong::proto::Application app = wukong::proto::jsonToApplication(request.body());
            endpoint()->invoker()->startupInstance(app, std::move(response));
            return;
        }
        if (uri == "/instance/shutdown") {
            SPDLOG_DEBUG("GlobalGatewayHandler received /function/shutdown request");
            wukong::proto::Application app = wukong::proto::jsonToApplication(request.body());
            endpoint()->invoker()->shutdownInstance(app, std::move(response));
            return;
        }
    }

    SPDLOG_WARN(fmt::format("GlobalGatewayHandler received Unsupported request ： {}", uri));
    response.send(Pistache::Http::Code::Bad_Request, fmt::format("Unsupported request : {}", uri));
}

InvokerEndpoint::InvokerEndpoint(
        const std::string &name,
        const std::shared_ptr<InvokerHandler> &handler) :
        Endpoint(name, handler) {
    handler->associateEndpoint(this);
}

void InvokerEndpoint::start() {
    Endpoint::start();
}

void InvokerEndpoint::stop() {
    Endpoint::stop();
}
