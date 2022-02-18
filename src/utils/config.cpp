//
// Created by 14408 on 2022/2/18.
//

#include <wukong/utils/config.h>
#include <wukong/utils/env.h>
#include <wukong/utils/log.h>

wukong::util::Config conf;

namespace wukong::util {

    Config::Config() {
        // System
        serialisation = getEnvVar("SERIALISATION", "json");
        logLevel = getEnvVar("LOG_LEVEL", "info");
        logFile = getEnvVar("LOG_FILE", "off");
        stateMode = getEnvVar("STATE_MODE", "inmemory");
        deltaSnapshotEncoding =
                getEnvVar("DELTA_SNAPSHOT_ENCODING", "pages=4096;xor;zstd=1");

        // Redis
        redisStateHost = getEnvVar("REDIS_STATE_HOST", "localhost");
        redisQueueHost = getEnvVar("REDIS_QUEUE_HOST", "localhost");
        redisPort = getEnvVar("REDIS_PORT", "6379");

        // Scheduling
        noScheduler = getIntEnvVar("NO_SCHEDULER", "0");
        overrideCpuCount = getIntEnvVar("OVERRIDE_CPU_COUNT", "0");
        noTopologyHints = getEnvVar("NO_TOPOLOGY_HINTS", "off");

        // Worker-related timeouts (all in seconds)
        globalMessageTimeout =
                getIntEnvVar("GLOBAL_MESSAGE_TIMEOUT", "60000");
        boundTimeout = getIntEnvVar("BOUND_TIMEOUT", "30000");

        // MPI
        defaultMpiWorldSize =
                getIntEnvVar("DEFAULT_MPI_WORLD_SIZE", "5");
        mpiBasePort = getIntEnvVar("MPI_BASE_PORT", "10800");

        // Endpoint
        endpointInterface = getEnvVar("ENDPOINT_INTERFACE", "");
        endpointHost = getEnvVar("ENDPOINT_HOST", "");
        endpointPort = getIntEnvVar("ENDPOINT_PORT", "8080");
        endpointNumThreads =
                getIntEnvVar("ENDPOINT_NUM_THREADS", "4");

//        if (endpointHost.empty()) {
//            // Get the IP for this host
//            endpointHost =
//                    wukong::util::getPrimaryIPForThisHost(endpointInterface);
//        }

        // Transport
        functionServerThreads =
                getIntEnvVar("FUNCTION_SERVER_THREADS", "2");
        stateServerThreads =
                getIntEnvVar("STATE_SERVER_THREADS", "2");
        snapshotServerThreads =
                getIntEnvVar("SNAPSHOT_SERVER_THREADS", "2");
        pointToPointServerThreads =
                getIntEnvVar("POINT_TO_POINT_SERVER_THREADS", "2");

        // Dirty tracking
        dirtyTrackingMode = getEnvVar("DIRTY_TRACKING_MODE", "segfault");
        diffingMode = getEnvVar("DIFFING_MODE", "xor");
    }

    void Config::print() {

        SPDLOG_INFO("--- System ---");
        SPDLOG_INFO("SERIALISATION              {}", serialisation);
        SPDLOG_INFO("LOG_LEVEL                  {}", logLevel);
        SPDLOG_INFO("LOG_FILE                   {}", logFile);
        SPDLOG_INFO("STATE_MODE                 {}", stateMode);
        SPDLOG_INFO("DELTA_SNAPSHOT_ENCODING    {}", deltaSnapshotEncoding);

        SPDLOG_INFO("--- Redis ---");
        SPDLOG_INFO("REDIS_STATE_HOST           {}", redisStateHost);
        SPDLOG_INFO("REDIS_QUEUE_HOST           {}", redisQueueHost);
        SPDLOG_INFO("REDIS_PORT                 {}", redisPort);

        SPDLOG_INFO("--- Scheduling ---");
        SPDLOG_INFO("NO_SCHEDULER               {}", noScheduler);
        SPDLOG_INFO("OVERRIDE_CPU_COUNT         {}", overrideCpuCount);
        SPDLOG_INFO("NO_TOPOLOGY_HINTS         {}", noTopologyHints);

        SPDLOG_INFO("--- Timeouts ---");
        SPDLOG_INFO("GLOBAL_MESSAGE_TIMEOUT     {}", globalMessageTimeout);
        SPDLOG_INFO("BOUND_TIMEOUT              {}", boundTimeout);

        SPDLOG_INFO("--- MPI ---");
        SPDLOG_INFO("DEFAULT_MPI_WORLD_SIZE  {}", defaultMpiWorldSize);
        SPDLOG_INFO("MPI_BASE_PORT  {}", mpiBasePort);

        SPDLOG_INFO("--- Endpoint ---");
        SPDLOG_INFO("ENDPOINT_INTERFACE         {}", endpointInterface);
        SPDLOG_INFO("ENDPOINT_HOST              {}", endpointHost);
        SPDLOG_INFO("ENDPOINT_PORT              {}", endpointPort);
        SPDLOG_INFO("ENDPOINT_NUM_THREADS       {}", endpointNumThreads);
    }
}