//
// Created by kingdo on 2022/3/4.
//

#include <wukong/proto/proto.h>
#include <wukong/utils/timing.h>
#include <wukong/utils/uuid.h>

#include "endpoint.h"
#include <utility>

void GlobalGatewayHandler::onRequest(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter response)
{
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

    switch (request.method())
    {
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

void GlobalGatewayHandler::onTimeout(const Pistache::Http::Request&, Pistache::Http::ResponseWriter response)
{
    response
        .send(Pistache::Http::Code::Request_Timeout, "Timeout")
        .then([=](ssize_t) {}, PrintException());
}

void GlobalGatewayHandler::handleGetReq(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter response)
{

    if (request.resource() == "/ping")
    {
        SPDLOG_DEBUG("GlobalGatewayHandler received ping request");
        response.send(Pistache::Http::Code::Ok, "PONG");
    }
    else
    {
        wukong::proto::Message msg;
        msg.set_id(wukong::utils::uuid());

        msg.set_user("kingdo");
        msg.set_application("test");
        if (request.resource() == "/python")
        {
            msg.set_function("py_hello");
        }
        else
        {
            msg.set_function("hello");
        }
        msg.set_type(wukong::proto::Message_MessageType_FUNCTION);
        msg.set_inputdata("wukong");
        msg.set_isasync(false);
        msg.set_resultkey(fmt::format("{}#{}#{}-{}",
                                      msg.user(),
                                      msg.application(),
                                      msg.function(),
                                      msg.id()));
        if (msg.isasync())
            response.send(Pistache::Http::Code::Ok, fmt::format("Result Key : <{}>", msg.resultkey()));
        msg.set_timestamp(wukong::utils::getMillsTimestamp());
        endpoint()->lb->dispatch(std::move(msg), std::move(response));
    }
}

void GlobalGatewayHandler::handlePostReq(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter response)
{

    const std::string& uri = request.resource();
    if (uri.starts_with("/invoker"))
    {
        if (uri == "/invoker/register")
        {
            SPDLOG_DEBUG("GlobalGatewayHandler received /invoker/register request");
            endpoint()->lb->handleInvokerRegister(request.address().host(), request.body(), std::move(response));
            return;
        }
        if (request.resource() == "/invoker/info")
        {
            SPDLOG_DEBUG("GlobalGatewayHandler received get_invokers_info request");
            endpoint()->lb->handleInvokerInfo(std::move(response));
            return;
        }
    }
    else if (uri.starts_with("/user"))
    {
        if (uri == "/user/register")
        {
            SPDLOG_DEBUG("GlobalGatewayHandler received /user/register request");
            wukong::proto::User user = wukong::proto::jsonToUser(request.body());
            endpoint()->lb->handleUserRegister(user, std::move(response));
            return;
        }
        if (uri == "/user/delete")
        {
            SPDLOG_DEBUG("GlobalGatewayHandler received /user/delete request");
            wukong::proto::User user = wukong::proto::jsonToUser(request.body());
            endpoint()->lb->handleUserDelete(user, std::move(response));
            return;
        }
        if (uri == "/user/info")
        {
            SPDLOG_DEBUG("GlobalGatewayHandler received /user/info request");
            const auto& cookies = request.cookies();
            auto username       = cookies.get("user").value;
            endpoint()->lb->handleUserInfo(username, std::move(response));
            return;
        }
    }
    else if (uri.starts_with("/application"))
    {
        if (uri == "/application/create")
        {
            SPDLOG_DEBUG("GlobalGatewayHandler received /application/create request");
            wukong::proto::Application application = wukong::proto::jsonToApplication(request.body());
            endpoint()->lb->handleAppCreate(application, std::move(response));
            return;
        }
        if (uri == "/application/delete")
        {
            SPDLOG_DEBUG("GlobalGatewayHandler received /application/delete request");
            wukong::proto::Application application = wukong::proto::jsonToApplication(request.body());
            endpoint()->lb->handleAppDelete(application, std::move(response));
            return;
        }
        if (uri == "/application/info")
        {
            SPDLOG_DEBUG("GlobalGatewayHandler received /application/info request");
            const auto& cookies = request.cookies();
            auto username       = cookies.get("user").value;
            auto appname        = cookies.get("application").value;
            endpoint()->lb->handleAppInfo(username, appname, std::move(response));
            return;
        }
    }
    else if (uri.starts_with("/function"))
    {
        if (uri == "/function/register")
        {
            SPDLOG_DEBUG("GlobalGatewayHandler received /function/register request");
            wukong::proto::Function function;
            const auto& cookies = request.cookies();
            function.set_user(cookies.get("user").value);
            function.set_application(cookies.get("application").value);
            function.set_functionname(cookies.get("function").value);
            function.set_concurrency(strtol(cookies.get("concurrency").value.c_str(), nullptr, 10));
            function.set_memory(strtol(cookies.get("memory").value.c_str(), nullptr, 10));
            function.set_cpu(strtol(cookies.get("cpu").value.c_str(), nullptr, 10));
            auto type = cookies.get("type").value;
            std::transform(type.begin(), type.end(), type.begin(), ::tolower);
            function.set_type(wukong::proto::FunctionTypeNameMAP.at(type));
            endpoint()->lb->handleFuncRegister(function, request.body(), std::move(response));
            return;
        }
        if (uri == "/function/delete")
        {
            const auto& cookies = request.cookies();
            auto username       = cookies.get("user").value;
            auto appname        = cookies.get("application").value;
            auto funcname       = cookies.get("function").value;
            endpoint()->lb->handleFuncDelete(username, appname, funcname, std::move(response));
            return;
        }
        if (uri == "/function/info")
        {
            SPDLOG_DEBUG("GlobalGatewayHandler received /function/info request");
            const auto& cookies = request.cookies();
            auto username       = cookies.get("user").value;
            auto appname        = cookies.get("application").value;
            auto funcname       = cookies.get("function").value;
            endpoint()->lb->handleFuncInfo(username, appname, funcname, std::move(response));
            return;
        }
    }
    SPDLOG_WARN(fmt::format("GlobalGatewayHandler received Unsupported request ： {}", uri));
    response.send(Pistache::Http::Code::Bad_Request, fmt::format("Unsupported request : {}", uri));
}