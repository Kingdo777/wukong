//
// Created by kingdo on 2022/3/17.
//

#include <wukong/utils/config.h>
#include <wukong/utils/signal-tool.h>
#include <wukong/utils/timing.h>
#include <wukong/utils/env.h>
#include <wukong/utils/process/DefaultSubProcess.h>

#include "LocalGatewayEndpoint.h"
#include "macro.h"

int main() {
    wukong::utils::initLog();
    SPDLOG_INFO("-------------------local-gateway config---------------------");
    wukong::utils::Config::print();
    SIGNAL_HANDLER()
    LocalGatewayEndpoint e;
    e.start();
    RETURN_ENDPOINT_PORT(e);
    SIGNAL_WAIT()
    e.stop();
    wukong::utils::Timing::printTimerTotals();
    return 0;
}