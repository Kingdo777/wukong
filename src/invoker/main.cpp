//
// Created by kingdo on 2022/3/9.
//

#include <wukong/utils/config.h>
#include <wukong/utils/signal-tool.h>
#include <wukong/utils/timing.h>

#include "invoker.h"

int main() {
    wukong::utils::initLog();
    wukong::utils::Config::print();
    SIGNAL_HANDLER()
    Invoker invoker;
    invoker.start();
    SIGNAL_WAIT()
    invoker.stop();
    wukong::utils::Timing::printTimerTotals();
    return 0;
}