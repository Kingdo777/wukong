//
// Created by kingdo on 2022/2/27.
//

#include <wukong/utils/timing.h>
#include <wukong/proto/proto.h>
#include "global-gw-handler.h"

void GlobalGatewayHandler::onRequest(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response) {
    SPDLOG_DEBUG("GlobalGatewayHandler received request..");
    TIMING_START(handler_request)
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
    response.timeoutAfter(std::chrono::milliseconds(wukong::utils::Config::EndpointRequestTimeout()));

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


    // Parse message from JSON in reque


    TIMING_END(handler_request)
}

void GlobalGatewayHandler::onTimeout(const Pistache::Http::Request &, Pistache::Http::ResponseWriter response) {
    response
            .send(Pistache::Http::Code::Request_Timeout, "Timeout")
            .then([=](ssize_t) {}, PrintException());
}

void GlobalGatewayHandler::handleGetReq(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response) {
    Pistache::Http::Code code = Pistache::Http::Code::Ok;
    std::string result = "OK\n";

    response.send(code, result);
}

void GlobalGatewayHandler::handlePostReq(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response) {
    const std::string &requestStr = request.body();

    Pistache::Http::Code code = Pistache::Http::Code::Ok;
    std::string result;

    if (requestStr.empty()) {
        SPDLOG_ERROR("Wukong handler received empty request");
        code = Pistache::Http::Code::Internal_Server_Error;
        result = "Empty request";
    } else {
        wukong::proto::Message msg = wukong::proto::jsonToMessage(requestStr);
    }

    response.send(code, result);
}

