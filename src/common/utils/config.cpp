//
// Created by 14408 on 2022/2/18.
//

#include <wukong/utils/config.h>
#include <wukong/utils/log.h>
#include <wukong/utils/macro.h>
#include <wukong/utils/os.h>
#include <wukong/utils/uuid.h>

namespace wukong::utils
{

    const std::string Config::logLevel                        = getEnvVar("LOG_LEVEL", "trace");
    const std::string Config::enableLogFile                   = getEnvVar("ENABLE_LOG_FILE", "off");
    const std::string Config::logFileBaseDir                  = getEnvVar("LOG_FILE_BASE_DIR", "/tmp/wukong/var/logs/");
    const int Config::endpointPort                            = getIntEnvVar("ENDPOINT_PORT", DEFAULT_ENDPOINT_PORT);
    const int Config::endpointNumThreads                      = getIntEnvVar("ENDPOINT_NUM_THREADS", hardware_concurrency());
    const uint64_t Config::headerTimeout                      = getIntEnvVar("HEADER_TIMEOUT", DEFAULT_HEADER_TIMEOUT);
    const std::chrono::seconds Config::endpointHeaderTimeout  = std::chrono::seconds(headerTimeout);
    const uint64_t Config::bodyTimeout                        = getIntEnvVar("BODY_TIMEOUT", DEFAULT_BODY_TIMEOUT);
    const std::chrono::seconds Config::endpointBodyTimeout    = std::chrono::seconds(bodyTimeout);
    const uint64_t Config::requestTimeout                     = getIntEnvVar("REQUEST_TIMEOUT", DEFAULT_REQUEST_TIMEOUT);
    const std::chrono::seconds Config::endpointRequestTimeout = std::chrono::seconds(requestTimeout);
    const int Config::clientNumThreads                        = getIntEnvVar("CLIENT_NUM_THREADS", hardware_concurrency());
    const int Config::clientMaxConnectionsPerHost             = getIntEnvVar("CLIENT_MAX_CONNECTS_PER_HOST", DEFAULT_CLIENT_MAX_CONNECTS_PER_HOST);
    const int Config::redisPort                               = getIntEnvVar("REDIS_PORT", 6379);
    const std::string Config::redisHostName                   = getEnvVar("REDIS_HOSTNAME", "localhost");
    const std::string Config::lbHost                          = getEnvVar("LB_HOST", "localhost");
    const int Config::lbPort                                  = getIntEnvVar("LB_PORT", 8080);
    const std::string Config::invokerHost                     = getEnvVar("LB_HOST", "localhost");
    const int Config::invokerPort                             = getIntEnvVar("LB_PORT", 8081);
    const std::string Config::invokerInitID                   = getEnvVar("INVOKER_INIT_ID", "invoker-" + std::to_string(uuid()));
    const int Config::invokerCPU                              = getIntEnvVar("INVOKER_CPU", 10 * 1000); /// 10个core
    const int Config::invokerMemory                           = getIntEnvVar("INVOKER_MEMORY", 64 * 1024); /// 64G
    const uint64_t Config::pauseTimeout                       = getIntEnvVar("PAUSE_TIMEOUT", INSTANCE_PAUSE_TIMEOUT); /// 1h
    const int Config::insFuncReadFD                           = getIntEnvVar("INSTANCE_FUNCTION_DEFAULT_READ_FD", 3); /// 3
    const uint64_t Config::insFuncReadBufferSize              = getIntEnvVar("INSTANCE_FUNCTION_DEFAULT_READ_BUFFER_SIZE", WUKONG_MESSAGE_SIZE); /// 3
    const int Config::insFuncWriteFD                          = getIntEnvVar("INSTANCE_FUNCTION_DEFAULT_WRITE_FD", 4); /// 4
    const int Config::insFuncInternalRequestFD                = getIntEnvVar("INSTANCE_FUNCTION_DEFAULT_WRITE_FD", 5); /// 5
    const int Config::insFuncInternalResponseFD               = getIntEnvVar("INSTANCE_FUNCTION_DEFAULT_WRITE_FD", 6); /// 6
    const int Config::sfNumThreads                            = getIntEnvVar("SF_NUM_THREADS", 1);
    const int Config::sfNumWorkers                            = getIntEnvVar("SF_NUM_WORKERS", 1);
    const int Config::sfCores                                 = getIntEnvVar("SF_CORES", 1000);
    const int Config::sfMemory                                = getIntEnvVar("SF_MEMORY", 64);

    void Config::print()
    {
        SPDLOG_INFO("--- Log ---");
        SPDLOG_INFO("logLevel                       {}", logLevel);
        SPDLOG_INFO("enableLogFile                  {}", enableLogFile);
        SPDLOG_INFO("logFileBaseDir                 {}", logFileBaseDir);

        SPDLOG_INFO("--- Endpoint ---");
        SPDLOG_INFO("endpointPort                   {}", endpointPort);
        SPDLOG_INFO("endpointNumThreads             {}", endpointNumThreads);
        SPDLOG_INFO("endpointHeaderTimeout          {}s", headerTimeout);
        SPDLOG_INFO("endpointBodyTimeout            {}s", bodyTimeout);
        SPDLOG_INFO("endpointRequestTimeout         {}s", requestTimeout);

        SPDLOG_INFO("--- Client ---");
        SPDLOG_INFO("clientNumThreads               {}", clientNumThreads);
        SPDLOG_INFO("clientMaxConnectionsPerHost    {}", clientMaxConnectionsPerHost);

        SPDLOG_INFO("--- Redis ---");
        SPDLOG_INFO("redisPort                      {}", redisPort);
        SPDLOG_INFO("redisHostname                  {}", redisHostName);

        SPDLOG_INFO("--- LB ---");
        SPDLOG_INFO("LBPort                         {}", lbPort);
        SPDLOG_INFO("LBHost                         {}", lbHost);

        SPDLOG_INFO("--- Invoker ---");
        SPDLOG_INFO("invokerPort                    {}", invokerPort);
        SPDLOG_INFO("invokerHost                    {}", invokerHost);
        SPDLOG_INFO("invokerInitID                  {}", invokerInitID);
        SPDLOG_INFO("invokerCPU                     {} cores", invokerCPU / 1000.0);
        SPDLOG_INFO("invokerMemory                  {:.1f}MB", invokerMemory / 1024.0);
        SPDLOG_INFO("pauseTimeout                   {}s", pauseTimeout);

        SPDLOG_INFO("--- Instance ---");
        SPDLOG_INFO("insFuncDefaultReadFD           {}", insFuncReadFD);
        SPDLOG_INFO("insFuncReadBufferSize          {}", insFuncReadBufferSize);
        SPDLOG_INFO("insFuncDefaultWriteFD          {}", insFuncWriteFD);
        SPDLOG_INFO("insFuncDefaultRequestFD        {}", insFuncInternalRequestFD);
        SPDLOG_INFO("insFuncDefaultResponseFD       {}", insFuncInternalResponseFD);

        SPDLOG_INFO("--- Storage Function ---");
        SPDLOG_INFO("sfNumThreads                   {}", sfNumThreads);
        SPDLOG_INFO("sfNumWorkers                   {}", sfNumWorkers);
        SPDLOG_INFO("sfCores                        {}", sfCores);
        SPDLOG_INFO("sfMemory                       {}", sfMemory);

        SPDLOG_INFO("------------------------------------------------------------");
    }
}