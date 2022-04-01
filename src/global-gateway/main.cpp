#include "endpoint.h"
#include "load-balance.h"
#include <wukong/utils/signal-tool.h>
#include <wukong/utils/timing.h>

int main()
{
    wukong::utils::initLog();
    SPDLOG_INFO("-------------------global-gateway config--------------------");
    wukong::utils::Config::print();
    SIGNAL_HANDLER()
    GlobalGatewayEndpoint e;
    e.start();
    sleep(1);
    SIGNAL_WAIT()
    e.stop();
    wukong::utils::Timing::printTimerTotals();
    return 0;
}