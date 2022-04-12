//
// Created by kingdo on 2022/3/23.
//

#include "StorageFuncAgent.h"
#include <wukong/utils/config.h>
#include <wukong/utils/signal-tool.h>
#include <wukong/utils/timing.h>

int main()
{
    wukong::utils::initLog("storage-func");
    SPDLOG_INFO("-------------------storage func config---------------------");
    wukong::utils::Config::print();
    SIGNAL_HANDLER()
    StorageFuncAgent agent;
    agent.run();
    SIGNAL_WAIT()
    agent.shutdown();
    wukong::utils::Timing::printTimerTotals();
    return 0;
}