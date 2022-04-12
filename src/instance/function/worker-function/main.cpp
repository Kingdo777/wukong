//
// Created by kingdo on 2022/3/23.
//

#include "WorkerFuncAgent.h"
#include <wukong/utils/signal-tool.h>

int main()
{
    std::string who = wukong::utils::getEnvVar("WHO_AM_I", "");
    wukong::utils::initLog("worker-func-" + who);
    SPDLOG_INFO("-------------------worker func config---------------------");
    wukong::utils::Config::print();
    SIGNAL_HANDLER()
    WorkerFuncAgent agent;
    auto opts = WorkerFuncAgent::Options::options();
    agent.init(opts);
    agent.set_handler(std::make_shared<AgentHandler>(&agent));
    agent.run();
    SIGNAL_WAIT()
    agent.shutdown();
    wukong::utils::Timing::printTimerTotals();
    return 0;
}