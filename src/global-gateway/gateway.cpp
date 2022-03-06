#include <wukong/utils/timing.h>
#include <wukong/utils/signal-tool.h>
#include <wukong/proto/proto.h>
#include "endpoint.h"
#include "load-balance.h"

int main() {
    wukong::utils::initLog();
    wukong::utils::Config::print();
    SIGNAL_HANDLER()
    GlobalGatewayEndpoint e;
    e.start();
    SIGNAL_WAIT()
    e.stop();
    wukong::utils::Timing::printTimerTotals();
    return 0;
}