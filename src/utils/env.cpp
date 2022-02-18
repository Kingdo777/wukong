//
// Created by 14408 on 2022/2/18.
//
#include <wukong/utils/env.h>

namespace wukong::util {
    std::string getEnvVar(std::string const &key, std::string const &deflt) {
        char const *val = getenv(key.c_str());

        if (val == nullptr) {
            return deflt;
        }

        std::string retVal(val);

        if (retVal.length() == 0) {
            return deflt;
        } else {
            return retVal;
        }
    }

    int getIntEnvVar(const std::string &key, const std::string &deflt) {
        return std::stoi(getEnvVar(key, deflt));
    }
}