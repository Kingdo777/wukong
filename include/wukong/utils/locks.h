//
// Created by kingdo on 2022/3/4.
//

#ifndef WUKONG_LOCKS_H
#define WUKONG_LOCKS_H

#include <shared_mutex>

namespace wukong::utils {
    typedef std::unique_lock<std::mutex> UniqueLock;
    typedef std::unique_lock<std::shared_mutex> FullLock;
    typedef std::shared_lock<std::shared_mutex> SharedLock;
}
#endif //WUKONG_LOCKS_H
