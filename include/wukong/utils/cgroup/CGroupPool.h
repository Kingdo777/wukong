//
// Created by kingdo on 2022/4/1.
//

#ifndef WUKONG_CGROUPPOOL_H
#define WUKONG_CGROUPPOOL_H

#include "CGroup.h"
#include <queue>
#include <unordered_map>

class CGroupPool
{
public:
    static std::shared_ptr<CGroupPool> GetCGroupPool()
    {
        return cgPool;
    }

private:
    CGroupPool() = default;

    std::mutex cgroup_pool_mutex;

    std::unordered_map<int64_t, std::queue<std::shared_ptr<MemoryCGroup>>> memory_cgroup_map;
    std::unordered_map<int64_t, std::queue<std::shared_ptr<CpuCGroup>>> cpu_cgroup_map;

    static std::shared_ptr<CGroupPool> cgPool;
};

#endif // WUKONG_CGROUPPOOL_H
