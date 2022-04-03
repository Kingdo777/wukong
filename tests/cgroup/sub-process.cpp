#include "wukong/utils/cgroup/CGroup.h"
#include "wukong/utils/log.h"
#include "wukong/utils/signal-tool.h"
//
// Created by kingdo on 2022/4/3.
//
int main()
{
    wukong::utils::initLog();
    SIGNAL_HANDLER()
    SPDLOG_INFO("SUB Process ， PID = {}", getpid());
    std::vector<std::thread> ts;
    ts.reserve(6);
    for (int i = 0; i < 6; ++i)
    {
        ts.emplace_back(std::thread([]() {
            while (1)
                ;
        }));
    }
    SIGNAL_WAIT()
    SPDLOG_INFO("SUB Process {} Shutdown", getpid());
}