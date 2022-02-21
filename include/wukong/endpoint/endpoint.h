//
// Created by 14408 on 2022/2/18.
//

#ifndef WUKONG_ENDPOINT_H
#define WUKONG_ENDPOINT_H

#include <wukong/utils/config.h>
#include <wukong/utils/log.h>
#include <wukong/endpoint//handler.h>
#include <pistache/endpoint.h>
#include <pistache/http.h>

namespace wukong::endpoint {
    class Endpoint {
    public:
        Endpoint(int port, int threadCount, const std::string &name,
                 const std::shared_ptr<Pistache::Http::Handler> &handler_ = nullptr);

        void start();

        void stop();

        void set_handler(const std::shared_ptr<Pistache::Http::Handler> &handler_) {
            endpointHandler = handler_;
        }

    private:
        int endpointPort;
        int endpointThreadCount;
        std::string endpointName;
        Pistache::Http::Endpoint endpoint;
        std::shared_ptr<Pistache::Http::Handler> endpointHandler = std::make_shared<DefaultHandler>();
    };
}

#endif //WUKONG_ENDPOINT_H
