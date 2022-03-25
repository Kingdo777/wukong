//
// Created by 14408 on 2022/2/18.
//

#include <wukong/utils/config.h>
#include <wukong/utils/log.h>
#include <wukong/utils/os.h>
#include <wukong/utils/uuid.h>

namespace wukong::utils {

    const std::string Config::logLevel = getEnvVar(    /* NOLINT */
            "LOG_LEVEL",
            "trace");
    const int Config::endpointPort = getIntEnvVar(       /* NOLINT */
            "ENDPOINT_PORT",
            DEFAULT_ENDPOINT_PORT);
    const int Config::endpointNumThreads = getIntEnvVar( /* NOLINT */
            "ENDPOINT_NUM_THREADS",
            hardware_concurrency());
    const uint64_t Config::headerTimeout = getIntEnvVar(    /* NOLINT */
            "HEADER_TIMEOUT",
            DEFAULT_HEADER_TIMEOUT);
    const std::chrono::seconds Config::endpointHeaderTimeout = std::chrono::seconds(headerTimeout);     /* NOLINT */
    const uint64_t Config::bodyTimeout = getIntEnvVar(      /* NOLINT */
            "BODY_TIMEOUT",
            DEFAULT_BODY_TIMEOUT);
    const std::chrono::seconds Config::endpointBodyTimeout = std::chrono::seconds(bodyTimeout);         /* NOLINT */
    const uint64_t Config::requestTimeout = getIntEnvVar(   /* NOLINT */
            "REQUEST_TIMEOUT",
            DEFAULT_REQUEST_TIMEOUT);
    const std::chrono::seconds Config::endpointRequestTimeout = std::chrono::seconds(requestTimeout);   /* NOLINT */

    const int Config::clientNumThreads = getIntEnvVar( /* NOLINT */
            "CLIENT_NUM_THREADS",
            hardware_concurrency());
    const int Config::clientMaxConnectionsPerHost = getIntEnvVar( /* NOLINT */
            "CLIENT_MAX_CONNECTS_PER_HOST",
            DEFAULT_CLIENT_MAX_CONNECTS_PER_HOST);

    const int Config::redisPort = getIntEnvVar( /* NOLINT */
            "REDIS_PORT",
            6379);

    const std::string Config::redisHostName = getEnvVar(    /* NOLINT */
            "REDIS_HOSTNAME",
            "localhost");

    const std::string Config::lbHost = getEnvVar(    /* NOLINT */
            "LB_HOST",
            "localhost");

    const int Config::lbPort = getIntEnvVar( /* NOLINT */
            "LB_PORT",
            8080);

    const std::string Config::invokerHost = getEnvVar(    /* NOLINT */
            "LB_HOST",
            "localhost");

    const int Config::invokerPort = getIntEnvVar( /* NOLINT */
            "LB_PORT",
            8081);

    const std::string Config::invokerInitID = getEnvVar(    /* NOLINT */
            "INVOKER_INIT_ID",
            "invoker-" + std::to_string(uuid()));

    const int Config::invokerCPU = getIntEnvVar( /* NOLINT */
            "INVOKER_CPU",
            10 * 1000); ///10个core

    const int Config::invokerMemory = getIntEnvVar( /* NOLINT */
            "INVOKER_MEMORY",
            64 * 1024); /// 64G

    const uint64_t Config::pauseTimeout = getIntEnvVar( /* NOLINT */
            "PAUSE_TIMEOUT",
            INSTANCE_PAUSE_TIMEOUT); /// 1h

    void Config::print() {
        SPDLOG_INFO("--- Log ---");
        SPDLOG_INFO("logLevel                       {}", logLevel);

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

        SPDLOG_INFO("------------------------------------------------------------");
    }
}