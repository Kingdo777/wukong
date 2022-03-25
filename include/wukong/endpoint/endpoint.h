//
// Created by 14408 on 2022/2/18.
//

#ifndef WUKONG_ENDPOINT_H
#define WUKONG_ENDPOINT_H

#include <wukong/utils/config.h>
#include <wukong/utils/log.h>
#include <pistache/endpoint.h>
#include <pistache/http.h>

namespace wukong::endpoint {

    class DefaultHandler : public Pistache::Http::Handler {

    HTTP_PROTOTYPE(DefaultHandler)

        void onRequest(
                const Pistache::Http::Request &req,
                Pistache::Http::ResponseWriter response) override;

        void onTimeout(
                const Pistache::Http::Request & /*req*/,
                Pistache::Http::ResponseWriter response) override;
    };


    class Endpoint {
    public:
        explicit Endpoint(const std::string &name,
                          const std::shared_ptr<Pistache::Http::Handler> &handler_,
                          int port = utils::Config::EndpointPort());

        virtual void start();

        virtual void stop();

        int port() {
            return endpointPort;
        }

    private:
        int endpointPort = utils::Config::EndpointPort();
        int endpointThreadCount = utils::Config::EndpointNumThreads();

        std::chrono::seconds endpointHeaderTimeout = utils::Config::EndpointHeaderTimeout();
        std::chrono::seconds endpointBodyTimeout = utils::Config::EndpointBodyTimeout();

        std::string endpointName;
        Pistache::Http::Endpoint endpoint;
        std::shared_ptr<Pistache::Http::Handler> endpointHandler = std::make_shared<DefaultHandler>();
    };
}

#endif //WUKONG_ENDPOINT_H
