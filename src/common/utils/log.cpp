//
// Created by 14408 on 2022/2/18.
//
#include <wukong/utils/config.h>
#include <wukong/utils/log.h>

void wukong::utils::initLog(std::string exec_name, const std::string& log_type)
{
    if (exec_name.empty())
    {
        exec_name = "unnamed/" + boost::dll::program_location().filename().string();
    }
    bool enable_file = false;

    if (log_type == "default")
        enable_file = Config::EnableLogFile() == "on";
    else if (log_type == "file")
        enable_file = true;

    if (enable_file)
    {
        auto path = boost::filesystem::path(Config::LogFileBaseDir()).append(exec_name).append("log");
        if (!exists(path))
        {
            create_directories(path.parent_path());
            creat(path.c_str(), 0600);
        }
        spdlog::set_default_logger(spdlog::basic_logger_mt(fmt::format("{}-{}", exec_name, getpid()), path.c_str()));
        spdlog::set_pattern(fmt::format("%^[%H:%M:%S] [%t] [%=6l]%$ %-60v [%@]"));
    }
    else
    {
        // Docs: https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
        // %^ %$    之间的内容将会被标上颜色
        // %H:%M:%S 是时间格式
        // [%t]     是线程ID
        // %=6l     是全写log类型，info、debug等，=6表示占6字符居中
        // %-60v    是log内容，-60表示占60字符左对齐
        // %@       是文件名和行号
        spdlog::set_pattern(fmt::format("%^ [{:^20}] [%H:%M:%S] [%t] [%=6l]%$ %-60v [%@]", exec_name));
    }

    spdlog::level::level_enum log_level;
    if (Config::LogLevel() == "trace")
    {
        CHECK_MIN_LEVEL(spdlog::level::trace)
        log_level = spdlog::level::trace;
    }
    else if (Config::LogLevel() == "debug")
    {
        CHECK_MIN_LEVEL(spdlog::level::debug)
        log_level = spdlog::level::debug;
    }
    else if (Config::LogLevel() == "info")
    {
        log_level = spdlog::level::info;
    }
    else
    {
        log_level = spdlog::level::off;
    }
    spdlog::set_level(log_level);
    spdlog::flush_on(log_level);
}

void wukong::utils::printAllENV()
{
    char** env = environ;
    while (*env != nullptr)
    {
        SPDLOG_INFO(*env);
        env++;
    }
}
