//
// Created by 14408 on 2022/2/18.
//

#include <wukong/endpoint/endpoint.h>
#include <wukong/utils/log.h>
#include <csignal>

namespace wukong::endpoint {
    Endpoint::Endpoint(const std::string &name, const std::shared_ptr<Pistache::Http::Handler> &handler) :
            endpoint(Pistache::Address(Pistache::Ipv4::any(), Pistache::Port(endpointPort))) {
        if (handler != nullptr)
            endpointHandler = handler;
    }

    void Endpoint::start() {
        SPDLOG_INFO("Starting {} endpoint on 0.0.0.0:{}, with {} threads",
                    endpointName,
                    endpointPort,
                    endpointThreadCount);

        // Set up signal handler
        sigset_t signals;
        if (sigemptyset(&signals) != 0 || sigaddset(&signals, SIGTERM) != 0 ||
            sigaddset(&signals, SIGKILL) != 0 ||
            sigaddset(&signals, SIGINT) != 0 ||
            sigaddset(&signals, SIGHUP) != 0 ||
            sigaddset(&signals, SIGQUIT) != 0 ||
            pthread_sigmask(SIG_BLOCK, &signals, nullptr) != 0) {
            throw std::runtime_error("Install signal handler failed");
        }

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

        // Wait for a signal
        SPDLOG_INFO("Awaiting signal");
        int signal = 0;
        int status = sigwait(&signals, &signal);
        if (status == 0) {
            SPDLOG_INFO("Received signal: {}", signal);
        } else {
            SPDLOG_INFO("Sigwait return value: {}", signal);
        }

        stop();
    }

    void Endpoint::stop() {
        SPDLOG_INFO("Shutting down {} endpoint ", endpointName);
        endpoint.shutdown();
    }
}