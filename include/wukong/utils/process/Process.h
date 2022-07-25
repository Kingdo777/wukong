//
// Created by kingdo on 22-7-19.
//

#ifndef WUKONG_PROCESS_H
#define WUKONG_PROCESS_H

#include "wukong/utils/os.h"
#include <csignal>
#include <cstdint>

namespace wukong::utils
{
    class Process
    {
    public:
        struct ProcessResource
        {
            uint64_t cpus; /// 指定使用cpu的core数，每个core划分为1000份
            uint64_t memory; /// 指定使用memory的多少，单位为MB
        };

        [[nodiscard]] pid_t getPid() const;

        [[nodiscard]] int send(int signum) const;

        bool kill();

        int pid            = -1;
        int64_t exitStatus = 0;
        /// 用于配置进程的CPU/Memory资源
        ProcessResource resource { 0, 0 };
    };
}
#endif // WUKONG_PROCESS_H
