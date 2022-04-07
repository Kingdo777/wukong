//
// Created by 14408 on 2022/2/18.
//

#ifndef WUKONG_ENV_H
#define WUKONG_ENV_H

#include <string>

namespace wukong::utils
{
    void setEnv(std::string const& key, int val, bool overwrite = true);
    void setEnv(std::string const& key, std::string const& val, bool overwrite = true);
    std::string getEnvVar(const std::string& key, const std::string& deflt);
    int getIntEnvVar(const std::string& key, int deflt);
}
#endif // WUKONG_ENV_H
