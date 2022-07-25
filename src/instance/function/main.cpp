//
// Created by kingdo on 2022/3/23.
//

#include "FunctionPool.h"
#include <wukong/utils/config.h>
#include <wukong/utils/signal-tool.h>
#include <wukong/utils/timing.h>

int main()
{
    wukong::utils::initLog("FunctionPool");
    SPDLOG_INFO("-------------------worker func config---------------------");
    wukong::utils::Config::print();
    SIGNAL_HANDLER()
    FunctionPool fp;
    auto opts = FunctionPool::Options::options();
    fp.init(opts);
    fp.set_handler(std::make_shared<FunctionPoolHandler>(&fp));
    fp.run();
    SIGNAL_WAIT()
    fp.shutdown();
    wukong::utils::Timing::printTimerTotals();
}