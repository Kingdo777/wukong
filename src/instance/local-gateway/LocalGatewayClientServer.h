//
// Created by kingdo on 2022/3/23.
//

#ifndef WUKONG_LOCAL_GATEWAY_CLIENTSERVER_H
#define WUKONG_LOCAL_GATEWAY_CLIENTSERVER_H

#include <wukong/client/client-server.h>

class LocalGatewayClientHandler : public wukong::client::ClientHandler {
PROTOTYPE_OF(Pistache::Aio::Handler, LocalGatewayClientHandler);

public:
    typedef wukong::client::ClientHandler Base;

    explicit LocalGatewayClientHandler(wukong::client::ClientServer *client) : ClientHandler(client) {}

    LocalGatewayClientHandler(const LocalGatewayClientHandler &handler) = default;;

    void onReady(const Pistache::Aio::FdSet &fds) override {
        Base::onReady(fds);
    }

    void registerPoller(Pistache::Polling::Epoll &poller) override {
        Base::registerPoller(poller);

    }

private:


};


#endif //WUKONG_LOCAL_GATEWAY_CLIENTSERVER_H
