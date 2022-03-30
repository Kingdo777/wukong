//
// Created by kingdo on 2022/3/23.
//

#include "Agent.h"
#include <wukong/utils/signal-tool.h>

int main() {
    wukong::utils::initLog();
    SPDLOG_INFO("-------------------worker func config---------------------");
    wukong::utils::Config::print();
    SIGNAL_HANDLER()
    Agent agent;
    auto opts = Agent::Options::options();
    agent.init(opts);
    agent.set_handler(std::make_shared<AgentHandler>(&agent));
    agent.run();
    SIGNAL_WAIT()
    agent.shutdown();
    wukong::utils::Timing::printTimerTotals();
    return 0;

}