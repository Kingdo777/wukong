#include "pistache/endpoint.h"
#include <wukong/proto/proto.h>

using namespace Pistache;

class HelloHandler : public Http::Handler {
public:
HTTP_PROTOTYPE(HelloHandler)

    void onRequest(const Http::Request & /*request*/, Http::ResponseWriter response) override {
        wukong::proto::ReplyRegisterInvoker msg;
        msg.set_success(true);
        msg.set_invokerid("invoker1");
        msg.set_msg("OK");
        response.send(Pistache::Http::Code::Ok, wukong::proto::messageToJson(msg));
    }
};

int main() {
    Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(9080));
    auto opts = Pistache::Http::Endpoint::options()
            .threads(1);

    Http::Endpoint server(addr);
    server.init(opts);
    server.setHandler(Http::make_handler<HelloHandler>());
    server.serve();
}
