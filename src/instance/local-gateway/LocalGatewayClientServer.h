//
// Created by kingdo on 2022/3/23.
//

#ifndef WUKONG_LOCAL_GATEWAY_CLIENTSERVER_H
#define WUKONG_LOCAL_GATEWAY_CLIENTSERVER_H

#include <wukong/client/client-server.h>
#include <wukong/proto/proto.h>

class LocalGateway;

class LocalGatewayClientHandler : public wukong::client::ClientHandler {
PROTOTYPE_OF(Pistache::Aio::Handler, LocalGatewayClientHandler);

public:
    typedef wukong::client::ClientHandler Base;

    explicit LocalGatewayClientHandler(wukong::client::ClientServer *client, LocalGateway *lg_) : ClientHandler(client),
                                                                                                  lg(lg_) {}

    LocalGatewayClientHandler(const LocalGatewayClientHandler &handler) : ClientHandler(handler),
                                                                          lg(handler.lg) {};

    void onReady(const Pistache::Aio::FdSet &fds) override;

    void registerPoller(Pistache::Polling::Epoll &poller) override;


    void
    callFunc(int write_fd_, int read_fd_, const wukong::proto::Message &msg, Pistache::Http::ResponseWriter response);

private:

    struct FunctionCallEntry {
        FunctionCallEntry(int write_fd_, int read_fd_, wukong::proto::Message msg,
                          Pistache::Http::ResponseWriter response_) :
                write_fd(write_fd_),
                read_fd(read_fd_),
                msg(std::move(msg)),
                response(std::move(response_)) {}

        int write_fd;
        int read_fd;
        wukong::proto::Message msg;
        Pistache::Http::ResponseWriter response;
    };

    void handleFunctionCallQueue();

    void handleResult(int fd);

    Pistache::PollableQueue<FunctionCallEntry> functionCallQueue;

    std::mutex msgResponseMapMutex;
    std::unordered_map<uint64_t, Pistache::Http::ResponseWriter> msgResponseMap;

    LocalGateway *lg;

};


#endif //WUKONG_LOCAL_GATEWAY_CLIENTSERVER_H
