//
// Created by kingdo on 2022/3/4.
//

#ifndef WUKONG_LOCKS_H
#define WUKONG_LOCKS_H

#include <shared_mutex>

namespace wukong::utils {
    typedef std::unique_lock<std::mutex>            UniqueLock;         /// 互斥锁
    typedef std::unique_lock<std::recursive_mutex>  RecursiveLock;      /// 递归互斥锁
    typedef std::unique_lock<std::shared_mutex>     WriteLock;          /// 写锁
    typedef std::shared_lock<std::shared_mutex>     ReadLock;           /// 读锁
}
#endif //WUKONG_LOCKS_H
