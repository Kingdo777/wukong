//
// Created by kingdo on 2022/3/9.
//

#include "invokerEndpoint.h"

void InvokerHandler::onRequest(const Pistache::Http::Request &req, Pistache::Http::ResponseWriter response) {
    if (req.resource() == "/ping") {
        if (req.method() == Pistache::Http::Method::Get) {
            response.send(Pistache::Http::Code::Ok, "PONG");
        }
    }
}

void InvokerHandler::onTimeout(const Pistache::Http::Request &, Pistache::Http::ResponseWriter response) {
    response.send(Pistache::Http::Code::Request_Timeout, "Timeout")
            .then([=](ssize_t) {}, PrintException());
}

InvokerEndpoint::InvokerEndpoint(
        const std::string &name,
        const std::shared_ptr<InvokerHandler> &handler) :
        Endpoint(name, handler) {

}

void InvokerEndpoint::start() {
    Endpoint::start();
}

void InvokerEndpoint::stop() {
    Endpoint::stop();
}
