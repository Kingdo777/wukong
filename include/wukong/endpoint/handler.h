//
// Created by kingdo on 2022/2/20.
//

#ifndef WUKONG_HANDLER_H
#define WUKONG_HANDLER_H

#include <pistache/http.h>

namespace wukong::endpoint{
    class DefaultHandler : public Pistache::Http::Handler {

        HTTP_PROTOTYPE(DefaultHandler)

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

}

#endif //WUKONG_HANDLER_H
