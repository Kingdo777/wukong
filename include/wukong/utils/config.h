//
// Created by 14408 on 2022/2/18.
//

#ifndef WUKONG_CONFIG_H
#define WUKONG_CONFIG_H

#include <string>

#define MPI_HOST_STATE_LEN 20

#define DEFAULT_TIMEOUT 60000
#define RESULT_KEY_EXPIRY 30000
#define STATUS_KEY_EXPIRY 300000

namespace wukong::util {
    class Config {

    public:
        // System
        std::string serialisation;
        std::string logLevel;
        std::string logFile;
        std::string stateMode;
        std::string deltaSnapshotEncoding;

        // Redis
        std::string redisStateHost;
        std::string redisQueueHost;
        std::string redisPort;

        // Scheduling
        int noScheduler;
        int overrideCpuCount;
        std::string noTopologyHints;

        // Worker-related timeouts
        int globalMessageTimeout;
        int boundTimeout;

        // MPI
        int defaultMpiWorldSize;
        int mpiBasePort;

        // Endpoint
        std::string endpointInterface;
        std::string endpointHost;
        int endpointPort;
        int endpointNumThreads;

        // Transport
        int functionServerThreads;
        int stateServerThreads;
        int snapshotServerThreads;
        int pointToPointServerThreads;

        // Dirty tracking
        std::string dirtyTrackingMode;
        std::string diffingMode;

        Config();

        void print();
    };
}

extern wukong::util::Config conf;

#endif //WUKONG_CONFIG_H
