//
// Created by kingdo on 2022/3/9.
//

#include <wukong/proto/proto.h>
#include <wukong/utils/env.h>
#include <wukong/utils/config.h>
#include "invokerClientServer.h"

std::pair<bool, std::string> InvokerClientServer::register2LB(const std::string &invokerJson) {
    bool status = false;
    std::string msg = "Invoker Registered";
    if (!registered) {
        std::string LBHost = wukong::utils::Config::InvokerLBHost();
        int LBPort = wukong::utils::Config::InvokerLBPort();
        std::string uri = LBHost + ":" + std::to_string(LBPort) + "/registerInvoker";
        auto rsp = this->post(uri).body(invokerJson).timeout(std::chrono::seconds(5)).send();
        while (rsp.isPending());
        rsp.then(
                [&](Pistache::Http::Response response) {
                    if (response.code() == Pistache::Http::Code::Ok) {
                        msg = response.body();
                        status = true;
                    } else {
                        msg = "Status Code Wrong, " + response.body();
                    }
                },
                [&](const std::exception_ptr &exc) {
                    try {
                        std::rethrow_exception(exc);
                    }
                    catch (const std::exception &e) {
                        msg = e.what();
                    }
                });
        if (!status)
            this->shutdown();
    }
    return std::make_pair(status, msg);
}
