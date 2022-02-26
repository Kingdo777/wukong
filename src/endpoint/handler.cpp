//
// Created by kingdo on 2022/2/20.
//

#include <wukong/endpoint/handler.h>
#include <wukong/utils/log.h>
#include <wukong/utils/config.h>
#include <wukong/utils/timing.h>

namespace wukong::endpoint {
    void
    DefaultHandler::onRequest(const Pistache::Http::Request &req,
                              Pistache::Http::ResponseWriter response) {

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
                    response.send(Pistache::Http::Code::Ok, "PONG\n").then(
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
            long chunk_size = 14;
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
            response.send(Pistache::Http::Code::Ok, "Hello World\n");
        }
    }

    void
    DefaultHandler::onTimeout(const Pistache::Http::Request &, Pistache::Http::ResponseWriter response) {
        response
                .send(Pistache::Http::Code::Request_Timeout, "Timeout")
                .then([=](ssize_t) {}, PrintException());
    }
}

namespace wukong::endpoint {
    void WukongHandler::onRequest(const Pistache::Http::Request &, Pistache::Http::ResponseWriter response) {
        SPDLOG_DEBUG("WukongHandler received request..");
        TIMING_START(test)
        // Very permissive CORS
        response.headers().add<Pistache::Http::Header::AccessControlAllowOrigin>(
                "*");
        response.headers().add<Pistache::Http::Header::AccessControlAllowMethods>(
                "GET,POST,PUT,OPTIONS");
        response.headers().add<Pistache::Http::Header::AccessControlAllowHeaders>(
                "User-Agent,Content-Type");
        // Text response type
        response.headers().add<Pistache::Http::Header::ContentType>(
                Pistache::Http::Mime::MediaType("text/plain"));
        response.timeoutAfter(std::chrono::milliseconds(utils::Config::EndpointRequestTimeout()));
        sleep(3);
        response.send(Pistache::Http::Code::Ok, "OK!\n");

        TIMING_END(test)
    }

    void WukongHandler::onTimeout(const Pistache::Http::Request &, Pistache::Http::ResponseWriter response) {
        response
                .send(Pistache::Http::Code::Request_Timeout, "Timeout")
                .then([=](ssize_t) {}, PrintException());
    }
}