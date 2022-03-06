//
// Created by kingdo on 2022/3/4.
//

#include <wukong/utils/timing.h>
#include <wukong/proto/proto.h>
#include <wukong/utils/uuid.h>

#include <utility>
#include "endpoint.h"

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
    // 目前pistache的timeout设计上存在无法解决的Bug，首先当前代码，timeout不可能被触发，因为触发只能通过执行处理请求的线程处理其timefd，
    // 但是在当前onRequest()执行完成之前，epoll都不会感知到timefd超时，当onRequest()结束后，response会被释放，届时将同时销毁response
    // 所拥有的timeout，导致其不会执行timefd的处理函数
    // 此外，send() 被执行时，也会直接禁用掉timefd，这还算合理。
    // 并且，response的移动构造函数，设计有问题，在移动Timeout时，原对象的fd将会被设置为-1，这样在原对象释放Timeout的时候会导致Bug
    // 最简单的优化方案应该是：
    // 避免在释放response时，调用~Timeout()中的关闭计时的操作，这样即使response被释放，timefd依然存在并正确被处理。但是显然这样timeout没有任何价值
    // 真正有价值的设计应该是，当timeout超时时，直接返回处理超时，同时让所有的onRequest()后续操作停止，但是这在实现上非常难，因为正在处理的线程是不会停下的。
    // 但是至少应确保，超时后要返回超时信息，因此，我认为需要一个专门的线程来处理超时
    // response.timeoutAfter(std::chrono::milliseconds(1000));

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
    TIMING_END(handler_request)
}

void GlobalGatewayHandler::onTimeout(const Pistache::Http::Request &, Pistache::Http::ResponseWriter response) {
    response
            .send(Pistache::Http::Code::Request_Timeout, "Timeout")
            .then([=](ssize_t) {}, PrintException());
}

void
GlobalGatewayHandler::handleGetReq(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response) {


    wukong::proto::Message msg;
    msg.set_id(wukong::utils::uuid());
    msg.set_type(wukong::proto::Message_MessageType_FUNCTION);

    msg.set_application("test");
    msg.set_function("hello");

    msg.set_isasync(true);

    msg.set_inputdata("");

    msg.set_timestamp(time(nullptr));

    endpoint()->lb->dispatch(std::move(msg), std::move(response));
}

void
GlobalGatewayHandler::handlePostReq(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response) {
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