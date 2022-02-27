//
// Created by 14408 on 2022/2/18.
//
#include <wukong/utils/log.h>
#include <wukong/utils/config.h>


void wukong::utils::initLog() {
    // Docs: https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
    spdlog::set_pattern("%^[%H:%M:%S] [%t] [%L]%$ %v");

    if (Config::LogLevel() == "trace") {
        CHECK_MIN_LEVEL(spdlog::level::trace)
        spdlog::set_level(spdlog::level::trace);
    } else if (Config::LogLevel() == "debug") {
        CHECK_MIN_LEVEL(spdlog::level::debug)
        spdlog::set_level(spdlog::level::debug);
    } else {
        spdlog::set_level(spdlog::level::info);
    }
}
