//
// Created by kingdo on 2022/3/6.
//

#ifndef WUKONG_SIGNAL_TOOL_H
#define WUKONG_SIGNAL_TOOL_H

#include <csignal>
#include <sys/wait.h>
#include <wukong/utils/log.h>

#define SIGNAL_HANDLER()                                                                                                                                                                                                                                                         \
    sigset_t signals;                                                                                                                                                                                                                                                            \
    if (sigemptyset(&signals) != 0 || sigaddset(&signals, SIGTERM) != 0 || sigaddset(&signals, SIGKILL) != 0 || sigaddset(&signals, SIGINT) != 0 || sigaddset(&signals, SIGHUP) != 0 || sigaddset(&signals, SIGQUIT) != 0 || pthread_sigmask(SIG_BLOCK, &signals, nullptr) != 0) \
    {                                                                                                                                                                                                                                                                            \
        throw std::runtime_error("Install signal handler failed");                                                                                                                                                                                                               \
    }

#define SIGNAL_WAIT()                                    \
    SPDLOG_INFO("Awaiting signal");                      \
    int signal = 0;                                      \
    int status = sigwait(&signals, &signal);             \
    if (status == 0)                                     \
    {                                                    \
        SPDLOG_INFO("Received signal: {}", signal);      \
    }                                                    \
    else                                                 \
    {                                                    \
        SPDLOG_INFO("Sigwait return value: {}", signal); \
    }

#endif //WUKONG_SIGNAL_TOOL_H
