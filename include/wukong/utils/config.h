//
// Created by 14408 on 2022/2/18.
//

#ifndef WUKONG_CONFIG_H
#define WUKONG_CONFIG_H

#include <wukong/utils/env.h>
#include <string>
#include <chrono>

#define DEFAULT_ENDPOINT_PORT       8080
#define DEFAULT_HEADER_TIMEOUT      300
#define DEFAULT_BODY_TIMEOUT        300
#define DEFAULT_REQUEST_TIMEOUT     300

#define DEFAULT_CLIENT_MAX_CONNECTS_PER_HOST    8


namespace wukong::utils {
    class Config {
    public:
        static std::string LogLevel() { return logLevel; };

        static int EndpointPort() { return endpointPort; };

        static int EndpointNumThreads() { return endpointNumThreads; };

        static auto EndpointHeaderTimeout() { return endpointHeaderTimeout; };

        static auto EndpointBodyTimeout() { return endpointBodyTimeout; };

        static auto EndpointRequestTimeout() { return endpointRequestTimeout; };

        static int ClientNumThreads() { return clientNumThreads; };

        static int ClientMaxConnectionsPerHost() { return clientMaxConnectionsPerHost; };

        static int RedisPort() { return redisPort; };

        static std::string RedisHostname() { return redisHostName; };

        static std::string InvokerLBHost() { return invokerLBHost; };

        static int InvokerLBPort() { return invokerLBPort; };

        static std::string InvokerInitID() { return invokerInitID; };

        static int InvokerCPU() { return invokerCPU; };

        static int InvokerMemory() { return invokerMemory; };


        static void print();

    private:
        /// Log
        const static std::string logLevel;

        /// Endpoint
        // gateway的默认端口，默认为8080
        const static int endpointPort;
        // gateway的工作线程数，默认为4
        const static int endpointNumThreads;
        // 每个HTTP请求发送header的时间，默认5min
        const static uint64_t headerTimeout;
        const static std::chrono::seconds endpointHeaderTimeout;
        // 每个HTTP请求发送body的时间,和HeaderTimeout共同计时请求的发送过程,如果超过这个时间没有HTTP请求,将断开TCP连接
        const static uint64_t bodyTimeout;
        const static std::chrono::seconds endpointBodyTimeout;
        // 每个请求的响应处理时间,注意,在处理请求的过程中以上两个timeout是无效的,请求处理完毕后会重置连接,以上两个timeout生效,因此此设置不能低于上两者
        const static uint64_t requestTimeout;
        const static std::chrono::seconds endpointRequestTimeout;

        /// Client-Server
        const static int clientNumThreads;
        const static int clientMaxConnectionsPerHost;

        /// Redis
        const static int redisPort;
        const static std::string redisHostName;

        /// Invoker
        const static std::string invokerLBHost;
        const static int invokerLBPort;
        const static std::string invokerInitID;
        const static int invokerCPU;
        const static int invokerMemory;
    };
}

#endif //WUKONG_CONFIG_H
