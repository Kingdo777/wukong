//
// Created by 14408 on 2022/2/18.
//

#include <wukong/endpoint/endpoint.h>
#include <wukong/utils/log.h>

namespace wukong::endpoint {
    Endpoint::Endpoint(const std::string &name, const std::shared_ptr<Pistache::Http::Handler> &handler) :
            endpoint(Pistache::Address(Pistache::Ipv4::any(), Pistache::Port(endpointPort))) {
        if (handler != nullptr)
            endpointHandler = handler;
        endpointName = name;
    }

    void Endpoint::start() {
        SPDLOG_INFO("Starting {} endpoint on 0.0.0.0:{}, with {} threads",
                    endpointName,
                    endpointPort,
                    endpointThreadCount);

        // Configure endpoint
        auto opts = Pistache::Http::Endpoint::options()
                .threads(endpointThreadCount)
                .bodyTimeout(endpointBodyTimeout)
                .headerTimeout(endpointHeaderTimeout)
                .backlog(256)
                .flags(Pistache::Tcp::Options::ReuseAddr);

        endpoint.init(opts);

        // Configure and start endpoint
        endpoint.setHandler(endpointHandler);
        endpoint.serveThreaded();
    }

    void Endpoint::stop() {
        SPDLOG_INFO("Shutting down {} endpoint ", endpointName);
        endpoint.shutdown();
    }
}