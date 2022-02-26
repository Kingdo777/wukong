//
// Created by 14408 on 2022/2/18.
//

#ifndef WUKONG_LOG_H
#define WUKONG_LOG_H

/// SPDLOG_ACTIVE_LEVEL 是spdlog自己宏, 指定了使用spflog的宏函数输出的时候, 显示那一级别的日志信息
#ifdef NO_DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO   // 不显示DEBUG的信息
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE  // 显示全部信息
#endif
/// 对于SPDLOG_ACTIVE_LEVEL的配置需要在spdlog头文件之前, 这样才能覆盖其配置
#include <spdlog/spdlog.h>

#define CHECK_MIN_LEVEL(level)                                          \
    if (SPDLOG_ACTIVE_LEVEL > (level) )                                  \
        SPDLOG_WARN(                                                    \
        "Logging set to trace but minimum log level set too high ({})", \
        SPDLOG_ACTIVE_LEVEL);

namespace wukong::utils {
    void initLog();
}

#endif //WUKONG_LOG_H
