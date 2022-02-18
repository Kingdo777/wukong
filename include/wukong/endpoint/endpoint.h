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
    class Endpoint {
    public:
        Endpoint(int port, int threadCount, const std::string &name);

        void start();

        void stop();

    private:
        int endpointPort;
        int endpointThreadCount;
        std::string endpointName;
        Pistache::Http::Endpoint endpoint;
    };
}

#endif //WUKONG_ENDPOINT_H
