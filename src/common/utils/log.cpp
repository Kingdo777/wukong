//
// Created by 14408 on 2022/2/18.
//
#include <wukong/utils/log.h>
#include <wukong/utils/config.h>


void wukong::utils::initLog() {
    // Docs: https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
    // %^ %$    之间的内容将会被标上颜色
    // %H:%M:%S 是时间格式
    // [%t]     是线程ID
    // %=6l     是全写log类型，info、debug等，=6表示占6字符居中
    // %-60v    是log内容，-60表示占60字符左对齐
    // %@       是文件名和行号
    spdlog::set_pattern("%^[%H:%M:%S] [%t] [%=6l]%$ %-60v [%@]");

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

void wukong::utils::printAllENV() {
    char **env = environ;
    while (*env != nullptr) {
        SPDLOG_INFO(*env);
        env++;
    }
}
