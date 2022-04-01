//
// Created by kingdo on 2022/3/5.
//
#include <wukong/utils/radom.h>

namespace wukong::utils
{
    std::string randomString(int len)
    {
        char result[len];

        static const char alphanum[] = "123456789"
                                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                       "abcdefghijklmnopqrstuvwxyz";

        static std::random_device rd;
        static std::mt19937 rng(rd());

        // Range cannot include last element of alphanum array as this is a null
        // terminator
        std::uniform_int_distribution<int> uni(0, sizeof(alphanum) - 2);

        for (int i = 0; i < len; ++i)
        {
            int r     = uni(rng);
            result[i] = alphanum[r];
        }

        return std::string { result, result + len };
    }
}