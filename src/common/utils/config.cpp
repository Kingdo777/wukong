//
// Created by 14408 on 2022/2/18.
//

#include <wukong/utils/config.h>
#include <wukong/utils/log.h>
#include <wukong/utils/os.h>

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

    void Config::print() {
        SPDLOG_INFO("--- Log ---");
        SPDLOG_INFO("logLevel                       {}", logLevel);

        SPDLOG_INFO("--- Endpoint ---");
        SPDLOG_INFO("endpointPort                   {}", endpointPort);
        SPDLOG_INFO("endpointNumThreads             {}", endpointNumThreads);
        SPDLOG_INFO("endpointHeaderTimeout          {}s", headerTimeout);
        SPDLOG_INFO("endpointBodyTimeout            {}s", bodyTimeout);
        SPDLOG_INFO("endpointRequestTimeout         {}s", requestTimeout);
    }
}