//
// Created by kingdo on 2022/3/17.
//

#include <wukong/utils/config.h>
#include <wukong/utils/env.h>
#include <wukong/utils/signal-tool.h>
#include <wukong/utils/timing.h>

#include "LocalGateway.h"

int main()
{
    wukong::utils::initLog("local-gateway");
    SPDLOG_INFO("-------------------local-gateway config---------------------");
    wukong::utils::Config::print();
    SIGNAL_HANDLER()
    LocalGateway lg;
    lg.start();
    SIGNAL_WAIT()
    lg.shutdown();
    wukong::utils::Timing::printTimerTotals();
    return 0;
}