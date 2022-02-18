//
// Created by 14408 on 2022/2/18.
//

#ifndef WUKONG_ENV_H
#define WUKONG_ENV_H

#include <string>

namespace wukong::util {
    std::string getEnvVar(const std::string &key, const std::string &deflt);

    int getIntEnvVar(const std::string &key, const std::string &deflt);
}
#endif //WUKONG_ENV_H
