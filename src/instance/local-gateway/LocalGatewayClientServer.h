//
// Created by kingdo on 2022/3/23.
//

#ifndef WUKONG_LOCAL_GATEWAY_CLIENT_SERVER_H
#define WUKONG_LOCAL_GATEWAY_CLIENT_SERVER_H

#include <wukong/client/client-server.h>
#include <wukong/proto/proto.h>
#include <wukong/utils/struct.h>
#include <wukong/utils/errors.h>


class LocalGateway;

class LocalGatewayClientHandler : public wukong::client::ClientHandler
{
    PROTOTYPE_OF(Pistache::Aio::Handler, LocalGatewayClientHandler);

    friend class LocalGateway;

public:
    typedef wukong::client::ClientHandler Base;

    explicit LocalGatewayClientHandler(wukong::client::ClientServer* client, LocalGateway* lg_)
        : ClientHandler(client)
        , lg(lg_)
    { }

    LocalGatewayClientHandler(const LocalGatewayClientHandler& handler)
        : ClientHandler(handler)
        , lg(handler.lg) {};

    void onReady(const Pistache::Aio::FdSet& fds) override;

    void registerPoller(Pistache::Polling::Epoll& poller) override;

    struct InternalRequestEntry
    {
        InternalRequestEntry(wukong::proto::Message msg_, int internalResponseFD_)
            : msg(std::move(msg_))
            , internalResponseFD(internalResponseFD_)
        { }
        wukong::proto::Message msg;
        int internalResponseFD;
    };

    struct InternalResponseEntry
    {
        InternalResponseEntry(void* process_, std::string funcname_, int responseFD_)
            : process(process_)
            , funcname(std::move(funcname_))
            , responseFD(responseFD_)
        { }
        void* process;
        std::string funcname;
        int responseFD;
    };

    struct ExternalRequestEntry
    {
        ExternalRequestEntry(wukong::proto::Message msg_, Pistache::Http::ResponseWriter response_)
            : msg(std::move(msg_))
            , response(std::move(response_))
        { }

        wukong::proto::Message msg;
        Pistache::Http::ResponseWriter response;
    };

    struct ExternalResponseEntry
    {
    public:
        ExternalResponseEntry(void* process_, std::string funcname_, Pistache::Http::ResponseWriter response_)
            : process(process_)
            , funcname(std::move(funcname_))
            , response(std::move(response_))
        { }
        void* process;
        std::string funcname;
        Pistache::Http::ResponseWriter response;
    };

private:
    void handleStorageFunctionInternalRequest(const wukong::proto::Message& msg, int responseFD);
    void handleWorkerFunctionInternalRequest(const wukong::proto::Message& msg, int responseFD);

    void handleInternalReadyQueue();
    void handleInternalWaitQueue();
    void handleExternalReadyQueue();
    void handleExternalWaitQueue();

    void handleExternalResult(const FuncResult& result);
    void handleInternalWorkerResult(const FuncResult& result);
    void handleInternalStorageResult(const FuncResult& result);
    void handleResult(int fd);
    void handleInternalRequest(int fd);

    Pistache::PollableQueue<InternalRequestEntry> internalReadyQueue;
    Pistache::PollableQueue<InternalRequestEntry> internalWaitQueue;
    Pistache::PollableQueue<ExternalRequestEntry> externalReadyQueue;
    Pistache::PollableQueue<ExternalRequestEntry> externalWaitQueue;

    std::mutex externalResponseMapMutex;
    std::unordered_map<uint64_t, ExternalResponseEntry> externalResponseMap;

    std::mutex internalWorkerResponseMapMutex;
    std::unordered_map<uint64_t, InternalResponseEntry> internalWorkerResponseMap;

    std::mutex internalStorageResponseMapMutex;
    std::unordered_map<uint64_t, InternalResponseEntry> internalStorageResponseMap;

    enum ResultType {
        ExternalResponse,
        InternalWorkerResponse,
        InternalStorageResponse
    };
    std::mutex responseTypeMapMutex;
    std::unordered_map<uint64_t, ResultType> responseTypeMap;

    LocalGateway* lg;
};

#endif // WUKONG_LOCAL_GATEWAY_CLIENT_SERVER_H
