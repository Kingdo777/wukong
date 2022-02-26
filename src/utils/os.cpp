//
// Created by kingdo on 2022/2/26.
//

#include <wukong/utils/os.h>
#include <thread>

int hardware_concurrency() { return std::thread::hardware_concurrency(); }