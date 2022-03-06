//
// Created by kingdo on 2022/3/4.
//

#ifndef WUKONG_UUID_H
#define WUKONG_UUID_H

#include <cstdint>
#include <atomic>
#include <mutex>

#define UUID_LEN 20

namespace wukong::utils {
    uint32_t uuid();
}

#endif //WUKONG_UUID_H
