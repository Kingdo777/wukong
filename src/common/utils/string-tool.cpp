//
// Created by kingdo on 2022/3/10.
//

#include <wukong/utils/string-tool.h>

bool wukong::utils::isAllWhitespace(const std::string& input)
{
    return std::all_of(input.begin(), input.end(), isspace);
}

bool wukong::utils::startsWith(const std::string& input, const std::string& subStr)
{
    if (subStr.empty())
    {
        return false;
    }

    return input.rfind(subStr, 0) == 0;
}

bool wukong::utils::endsWith(const std::string& value, const std::string& ending)
{
    if (ending.empty() || ending.size() > value.size())
    {
        return false;
    }
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool wukong::utils::contains(const std::string& input, const std::string& subStr)
{
    if (input.find(subStr) != std::string::npos)
    {
        return true;
    }
    else
    {
        return false;
    }
}

std::string wukong::utils::removeSubstr(const std::string& input, const std::string& toErase)
{
    std::string output = input;

    size_t pos = output.find(toErase);

    if (pos != std::string::npos)
    {
        output.erase(pos, toErase.length());
    }

    return output;
}

bool wukong::utils::stringIsInt(const std::string& input)
{
    return !input.empty() && input.find_first_not_of("0123456789") == std::string::npos;
}
