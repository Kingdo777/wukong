//
// Created by kingdo on 2022/2/20.
//

#ifndef WUKONG_HANDLER_H
#define WUKONG_HANDLER_H

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

    class WukongHandler : public Pistache::Http::Handler {
    public:
    HTTP_PROTOTYPE(WukongHandler)

        void onRequest(const Pistache::Http::Request & /*request*/, Pistache::Http::ResponseWriter response) override;

        void onTimeout(const Pistache::Http::Request & /*req*/,
                       Pistache::Http::ResponseWriter response) override;
    };

}

#endif //WUKONG_HANDLER_H
