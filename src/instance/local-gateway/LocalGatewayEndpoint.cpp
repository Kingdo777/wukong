//
// Created by kingdo on 2022/3/23.
//

#include "LocalGatewayEndpoint.h"
#include "LocalGateway.h"

void LocalGatewayHandler::onRequest(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response) {
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

void LocalGatewayHandler::onTimeout(const Pistache::Http::Request &, Pistache::Http::ResponseWriter response) {
    response
            .send(Pistache::Http::Code::Request_Timeout, "Timeout")
            .then([=](ssize_t) {}, PrintException());
}

void
LocalGatewayHandler::handleGetReq(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response) {
    const std::string &uri = request.resource();
    if (uri == "/ping") {
        SPDLOG_DEBUG("LocalGatewayHandler received ping request");
        response.send(Pistache::Http::Code::Ok, "PONG");
        return;
    }
    SPDLOG_WARN(fmt::format("LocalGatewayHandler received Unsupported request ： {}", uri));
    response.send(Pistache::Http::Code::Bad_Request, fmt::format("Unsupported request : {}", uri));
}

void
LocalGatewayHandler::handlePostReq(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response) {
    const std::string &uri = request.resource();
    if (uri == "/ping") {
        SPDLOG_DEBUG("LocalGatewayHandler received ping request");
        response.send(Pistache::Http::Code::Ok, "PONG");
        return;
    }

    if (uri == "/init") {
        SPDLOG_DEBUG("LocalGatewayHandler received /init request");

        std::string username = request.cookies().get("username").value;
        std::string appname = request.cookies().get("appname").value;

        auto res = endpoint()->lg->initApp(username, appname);
        if (!res.first) {
            response.send(Pistache::Http::Code::Internal_Server_Error, fmt::format("Init App `{}#{}` Failed : {}",
                                                                                   username, appname, res.second));
            return;
        }
        response.send(Pistache::Http::Code::Ok, "ok");
        return;
    }

    if (uri.starts_with("/function")) {
        if (uri.starts_with("/function/call")) {
            SPDLOG_DEBUG("LocalGatewayHandler received /function/call request");
            const auto &msg = wukong::proto::jsonToMessage(request.body());
            const auto &username = msg.user();
            const auto &appname = msg.application();
            if (!endpoint()->lg->checkUser(username) || !endpoint()->lg->checkApp(appname)) {
                response.send(Pistache::Http::Code::Internal_Server_Error, "username or appname isn't match Instance");
                return;
            }
            endpoint()->lg->callFunc(msg, std::move(response));
            return;
        }
    }

    SPDLOG_WARN(fmt::format("LocalGatewayHandler received Unsupported request ： {}", uri));
    response.send(Pistache::Http::Code::Bad_Request, fmt::format("Unsupported request : {}", uri));
}
