//
// Created by kingdo on 2022/4/6.
//

#include <wukong/utils/config.h>
#include <wukong/utils/env.h>
#include <wukong/utils/errors.h>
#include <wukong/utils/macro.h>
#include <wukong/utils/os.h>
#include <wukong/utils/signal-tool.h>
#include <wukong/utils/timing.h>

#include "../src/instance/local-gateway/LocalGateway.h"

class GlobalGatewayEndpoint;

class LoadBalanceClientHandler : public wukong::client::ClientHandler
{
    PROTOTYPE_OF(Pistache::Aio::Handler, LoadBalanceClientHandler);

public:
    typedef wukong::client::ClientHandler Base;

    explicit LoadBalanceClientHandler(wukong::client::ClientServer* client)
        : wukong::client::ClientHandler(client)
    { }

    LoadBalanceClientHandler(const LoadBalanceClientHandler& handler)
        : ClientHandler(handler)
    { }

    void onReady(const Pistache::Aio::FdSet& fds) override
    {
        for (auto fd : fds)
        {
            if (fd.getTag() == functionCallQueue.tag())
            {
                handleFunctionCallQueue();
            }
            else if (fd.getTag() == responseQueue.tag())
            {
                handleResponseQueue();
            }
        }
        Base::onReady(fds);
    }

    void registerPoller(Pistache::Polling::Epoll& poller) override
    {
        functionCallQueue.bind(poller);
        responseQueue.bind(poller);
        Base::registerPoller(poller);
    }

    struct FunctionCallEntry
    {

        explicit FunctionCallEntry(std::string host_,
                                   std::string port_,
                                   std::string uri_,
                                   bool is_async_,
                                   std::string resultKey_,
                                   std::string data_,
                                   Pistache::Http::ResponseWriter response_)
            : instanceHost(std::move(host_))
            , instancePort(std::move(port_))
            , funcname(std::move(uri_))
            , data(std::move(data_))
            , resultKey(std::move(resultKey_))
            , is_async(is_async_)
            , response(std::move(response_))
        { }

        /// curl -X POST -d "data" http://ip:port/uri
        std::string instanceHost;
        std::string instancePort;
        std::string funcname;
        std::string data; /// proto::Message的json序列
        std::string resultKey;
        bool is_async;

        Pistache::Http::ResponseWriter response;
    };

    struct ResponseEntry
    {
        ResponseEntry(Pistache::Http::Code code_,
                      std::string result_,
                      std::string index_)
            : code(code_)
            , result(std::move(result_))
            , index(std::move(index_))
        { }

        Pistache::Http::Code code;
        std::string result;
        std::string index;
    };
    void callFunction(FunctionCallEntry entry)
    {
        // TODO in right thread
        functionCallQueue.push(std::move(entry));
    }

private:
    Pistache::PollableQueue<FunctionCallEntry> functionCallQueue;
    std::map<std::string, FunctionCallEntry> functionCallMap;
    std::mutex functionCallMapMutex;

    Pistache::PollableQueue<ResponseEntry> responseQueue;

    void handleFunctionCallQueue()
    {
        for (;;)
        {
            auto entry = functionCallQueue.popSafe();
            if (!entry)
                break;
            asyncCallFunction(std::move(*entry));
        }
    }

    void asyncCallFunction(LoadBalanceClientHandler::FunctionCallEntry&& entry)
    {
        auto uri = fmt::format("http://{}:{}/function/call/{}", entry.instanceHost, entry.instancePort, entry.funcname);
        auto res = post(uri, entry.data);
        if (entry.is_async)
            return;
        std::string funcEntryIndex = wukong::utils::randomString(15);
        wukong::utils::UniqueLock lock(functionCallMapMutex);
        functionCallMap.insert(std::make_pair(funcEntryIndex, std::move(entry)));
        lock.unlock(); /// 必须要在这里解锁，不然responseFunctionCall如果被同一线程执行，会导致死锁，虽然这种可能性几乎不存在
        res.then(
            [funcEntryIndex, this](Pistache::Http::Response response) {
                auto code          = response.code();
                std::string result = response.body();
                responseFunctionCall(code, result, this, funcEntryIndex);
            },
            [funcEntryIndex, this](std::exception_ptr exc) {
                try
                {
                    std::rethrow_exception(std::move(exc));
                }
                catch (const std::exception& e)
                {
                    auto code          = Pistache::Http::Code::Internal_Server_Error;
                    std::string result = e.what();
                    responseFunctionCall(code, result, this, funcEntryIndex);
                }
            });
    }

    void handleResponseQueue()
    {
        for (;;)
        {
            auto entry = responseQueue.popSafe();
            if (!entry)
                break;
            responseFunctionCall(entry->code, entry->result, this, entry->index);
        }
    }

    void responseFunctionCall(Pistache::Http::Code code, const std::string& result,
                              LoadBalanceClientHandler* h,
                              const std::string& funcEntryIndex)
    {
        if (h != this)
        {
            /// 这里是我想多了， 虽然很难想，但是基本上确定，虽然post返回后更换了线程但是依然会执行原来的handler，很难理解的地方！
            assert(false);
            ResponseEntry entry(code, result, funcEntryIndex);
            return;
        }
        wukong::utils::UniqueLock lock(functionCallMapMutex);
        auto iter = functionCallMap.find(funcEntryIndex);
        if (iter == functionCallMap.end())
        {
            SPDLOG_ERROR(fmt::format("functionCall not Find in functionCallMap, with funcEntryIndex `{}`",
                                     funcEntryIndex));
            return;
        }
        if (code != Pistache::Http::Code::Ok)
        {
            auto msg = fmt::format("Call Function Failed : {}", result);
            SPDLOG_ERROR(msg);
        }
        iter->second.response.send(code, result);
        functionCallMap.erase(iter);
    }
};

class GlobalGatewayHandler : public Pistache::Http::Handler
{
public:
    HTTP_PROTOTYPE(GlobalGatewayHandler)

    void onRequest(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter response) override
    {
        response.headers().add<Pistache::Http::Header::AccessControlAllowOrigin>("*");
        response.headers().add<Pistache::Http::Header::AccessControlAllowMethods>("GET,POST,PUT,OPTIONS");
        response.headers().add<Pistache::Http::Header::AccessControlAllowHeaders>("User-Agent,Content-Type");
        response.headers().add<Pistache::Http::Header::ContentType>(Pistache::Http::Mime::MediaType("text/plain"));
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

    void associateEndpoint(GlobalGatewayEndpoint* e, wukong::client::ClientServer* cs_, int instance_port_)
    {
        this->endpoint = e;
        this->cs       = cs_;
        instancePost   = instance_port_;
    }

private:
    std::shared_ptr<LoadBalanceClientHandler> pickOneHandler()
    {
        return std::static_pointer_cast<LoadBalanceClientHandler>(cs->pickOneHandler());
    }

    void handleGetReq(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter response)
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
            msg.set_type(wukong::proto::Message_MessageType_FUNCTION);

            msg.set_user("kingdo");
            msg.set_application("test");
            const std::string& func = request.resource();
            if (func == "/")
                msg.set_function("py_hello");
            else
                msg.set_function(func.substr(1));
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

            LoadBalanceClientHandler::FunctionCallEntry functionCallEntry(
                "localhost",
                std::to_string(instancePost),
                msg.function(),
                msg.isasync(),
                msg.resultkey(),
                wukong::proto::messageToJson(msg),
                std::move(response));

            pickOneHandler()->callFunction(std::move(functionCallEntry));
        }
    }

    void handlePostReq(const Pistache::Http::Request&, Pistache::Http::ResponseWriter response)
    {
        SPDLOG_WARN("handlePostReq");
        response.send(Pistache::Http::Code::Ok, "POST Request");
    }

    GlobalGatewayEndpoint* endpoint {};
    wukong::client::ClientServer* cs {};
    int instancePost {};
};

class GlobalGatewayEndpoint : public wukong::endpoint::Endpoint
{
public:
    typedef wukong::endpoint::Endpoint BASE;

    explicit GlobalGatewayEndpoint(wukong::client::ClientServer* cs,
                                   int instancePost,
                                   const std::string& name                              = "global-gateway-endpoint",
                                   const std::shared_ptr<GlobalGatewayHandler>& handler = std::make_shared<GlobalGatewayHandler>())
        : Endpoint(name, handler)
    {
        handler->associateEndpoint(this, cs, instancePost);
    }
};

int main()
{
    SPDLOG_ERROR("记得，需要配置NEED_RETURN_PORT=1 和 ENDPOINT_PORT=0 两个环境变量")
    int fds[2][2];
    for (auto& fd : fds)
    {
        wukong::utils::socket_pair(fd);
    }
    if (fork() == 0)
    {
        wukong::utils::initLog("global-lw");
        SPDLOG_INFO("-------------------global config---------------------");
        wukong::utils::Config::print();
        SIGNAL_HANDLER()

        int write_fd = fds[0][0];
        int read_fd  = fds[1][0];

        std::string username = "kingdo";
        std::string appname  = "test";

        int instancePort = 0;
        wukong::utils::nonblock_ioctl(read_fd, 0);
        wukong::utils::read_from_fd(read_fd, &instancePort);
        WK_CHECK_WITH_EXIT(instancePort > 0, fmt::format("can't get Instance Port"));

        wukong::client::ClientServer cs;
        auto opts = Pistache::Http::Client::options().threads(wukong::utils::Config::ClientNumThreads()).maxConnectionsPerHost(wukong::utils::Config::ClientMaxConnectionsPerHost());
        cs.setHandler(std::make_shared<LoadBalanceClientHandler>(&cs));
        cs.start(opts);

        bool success    = false;
        std::string msg = "ok";

        std::string uri = fmt::format("http://localhost:{}/ping", instancePort);
        auto rsp        = cs.get(uri).send();
        while (rsp.isPending())
            ;
        rsp.then(
            [&](Pistache::Http::Response response) {
                if (response.code() == Pistache::Http::Code::Ok && response.body() == "PONG")
                {
                    success = true;
                }
                else
                {
                    msg = "Status Code Wrong, " + response.body();
                }
            },
            [&](const std::exception_ptr& exc) {
                try
                {
                    std::rethrow_exception(exc);
                }
                catch (const std::exception& e)
                {
                    msg = e.what();
                }
            });
        WK_CHECK_WITH_EXIT(success, fmt::format("Ping Failed, {}", msg));

        uri = fmt::format("http://localhost:{}/init", instancePort);
        Pistache::Http::Cookie cookie1("username", username);
        Pistache::Http::Cookie cookie2("appname", appname);
        rsp = cs.post(uri).cookie(cookie1).cookie(cookie2).send();
        while (rsp.isPending())
            ;
        rsp.then(
            [&](Pistache::Http::Response response) {
                if (response.code() == Pistache::Http::Code::Ok && response.body() == "PONG")
                {
                    success = true;
                }
                else
                {
                    msg = "Status Code Wrong, " + response.body();
                }
            },
            [&](const std::exception_ptr& exc) {
                try
                {
                    std::rethrow_exception(exc);
                }
                catch (const std::exception& e)
                {
                    msg = e.what();
                }
            });
        WK_CHECK_WITH_EXIT(success, fmt::format("Init Failed, {}", msg));

        GlobalGatewayEndpoint endpoint(&cs, instancePort);
        endpoint.start();
        SIGNAL_WAIT()
        endpoint.stop();
        wukong::utils::Timing::printTimerTotals();
    }
    wukong::utils::initLog("local-gw");
    SPDLOG_INFO("-------------------local config---------------------");
    wukong::utils::Config::print();
    SIGNAL_HANDLER()
    ::dup2(fds[0][1], 3);
    ::dup2(fds[1][1], 4);
    LocalGateway lg;
    lg.start();
    SIGNAL_WAIT()
    lg.shutdown();
    wukong::utils::Timing::printTimerTotals();
    return 0;
}