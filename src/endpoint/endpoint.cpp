//
// Created by 14408 on 2022/2/18.
//

#include <wukong/endpoint/endpoint.h>
#include <wukong/utils/log.h>
#include <csignal>

namespace wukong::endpoint {
    class MyHandler : public Pistache::Http::Handler {

    HTTP_PROTOTYPE(MyHandler)

        void onRequest(
                const Pistache::Http::Request &req,
                Pistache::Http::ResponseWriter response) override {

            if (req.resource() == "/ping") {
                if (req.method() == Pistache::Http::Method::Get) {

                    using namespace Pistache::Http;

                    const auto &query = req.query();
                    if (query.has("chunked")) {
                        std::cout << "Using chunked encoding" << std::endl;

                        response.headers()
                                .add<Header::Server>("pistache/0.1")
                                .add<Header::ContentType>(MIME(Text, Plain));

                        response.cookies()
                                .add(Cookie("lang", "en-US"));

                        auto stream = response.stream(Pistache::Http::Code::Ok);
                        stream << "PO";
                        stream << "NG";
                        stream << ends;
                    } else {
                        response.send(Pistache::Http::Code::Ok, "PONG").then(
                                [](ssize_t bytes) { std::cout << bytes << " bytes have been sent\n"; },
                                Pistache::Async::NoExcept
                        );
                    }
                }
            } else if (req.resource() == "/echo") {
                if (req.method() == Pistache::Http::Method::Post) {
                    response.send(Pistache::Http::Code::Ok, req.body(), MIME(Text, Plain));
                } else {
                    response.send(Pistache::Http::Code::Method_Not_Allowed);
                }
            } else if (req.resource() == "/stream_binary") {
                auto stream = response.stream(Pistache::Http::Code::Ok);
                char binary_data[] = "some \0\r\n data\n";
                size_t chunk_size = 14;
                for (size_t i = 0; i < 10; ++i) {
                    stream.write(binary_data, chunk_size);
                    stream.flush();
                }
                stream.ends();
            } else if (req.resource() == "/exception") {
                throw std::runtime_error("Exception thrown in the handler");
            } else if (req.resource() == "/timeout") {
                response.timeoutAfter(std::chrono::seconds(2));
            } else if (req.resource() == "/static") {
                if (req.method() == Pistache::Http::Method::Get) {
                    Pistache::Http::serveFile(response, "README.md").then([](ssize_t bytes) {
                                                                              std::cout << "Sent " << bytes << " bytes" << std::endl;
                                                                          },
                                                                          Pistache::Async::NoExcept);
                }
            } else {
                response.send(Pistache::Http::Code::Not_Found);
            }
        }

        void onTimeout(
                const Pistache::Http::Request & /*req*/,
                Pistache::Http::ResponseWriter response) override {
            response
                    .send(Pistache::Http::Code::Request_Timeout, "Timeout")
                    .then([=](ssize_t) {}, PrintException());
        }
    };


    wukong::endpoint::Endpoint::Endpoint(int port, int threadCount, const std::string &name) :
            endpointPort(port),
            endpointThreadCount(threadCount),
            endpoint(Pistache::Address(Pistache::Ipv4::any(), Pistache::Port(port))) {}

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
                .backlog(256)
                .flags(Pistache::Tcp::Options::ReuseAddr);

        endpoint.init(opts);

        // Configure and start endpoint
        endpoint.setHandler(Pistache::Http::make_handler<MyHandler>());
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

        endpoint.shutdown();
    }

    void Endpoint::stop() {
        SPDLOG_INFO("Shutting down {} endpoint ", endpointName);
        endpoint.shutdown();
    }
}