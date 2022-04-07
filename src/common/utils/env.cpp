//
// Created by 14408 on 2022/2/18.
//
#include <wukong/utils/env.h>

namespace wukong::utils
{
    void setEnv(std::string const& key, std::string const& val, bool overwrite)
    {
        setenv(key.c_str(), val.c_str(), overwrite);
    }

    void setEnv(std::string const& key, int val, bool overwrite)
    {
        setEnv(key, std::to_string(val), overwrite);
    }

    std::string getEnvVar(std::string const& key, std::string const& deflt)
    {
        char const* val = getenv(key.c_str());

        if (val == nullptr)
        {
            return deflt;
        }

        std::string retVal(val);

        if (retVal.length() == 0)
        {
            return deflt;
        }
        else
        {
            return retVal;
        }
    }

    int getIntEnvVar(const std::string& key, int deflt)
    {
        std::string result = getEnvVar(key, "QWERTYU");
        if ("QWERTYU" == result)
            return deflt;
        else
            return std::stoi(result);
    }
}