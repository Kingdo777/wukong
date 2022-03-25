//
// Created by 14408 on 2022/2/18.
//

#include <wukong/endpoint/endpoint.h>
#include <wukong/utils/log.h>

namespace wukong::endpoint {
    Endpoint::Endpoint(const std::string &name, const std::shared_ptr<Pistache::Http::Handler> &handler, int port) :
            endpointPort(port),
            endpoint(Pistache::Address(Pistache::Ipv4::any(), Pistache::Port(endpointPort))) {
        if (handler != nullptr)
            endpointHandler = handler;
        endpointName = name;
    }

    void Endpoint::start() {
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
        if (0 == endpointPort)
            endpointPort = endpoint.getPort();
        SPDLOG_INFO("Startup {} endpoint on 0.0.0.0:{}, with {} threads",
                    endpointName,
                    endpointPort,
                    endpointThreadCount);
    }

    void Endpoint::stop() {
        SPDLOG_INFO("Shutting down {} endpoint ", endpointName);
        endpoint.shutdown();
    }
}