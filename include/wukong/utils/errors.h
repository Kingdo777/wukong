//
// Created by kingdo on 2022/4/2.
//

#ifndef WUKONG_ERRORS_H
#define WUKONG_ERRORS_H

#include <fmt/format.h>
#include <string>
#include <wukong/utils/macro.h>

namespace wukong::utils
{
    std::string errors(const std::string& op = "");
}

#endif // WUKONG_ERRORS_H