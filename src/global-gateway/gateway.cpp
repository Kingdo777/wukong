#include <wukong/utils/timing.h>
#include <wukong/proto/proto.h>
#include "endpoint.h"

int main() {
    wukong::utils::initLog();
    wukong::utils::Config::print();
    GlobalGatewayEndpoint e;
    e.start();
    wukong::utils::Timing::printTimerTotals();
    return 0;
}

