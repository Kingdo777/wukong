//
// Created by kingdo on 2022/2/26.
//

#ifndef WUKONG_JSON_H
#define WUKONG_JSON_H

#include <map>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sstream>
#include <string>
#include <vector>

namespace wukong::utils
{
    bool getBoolFromJson(rapidjson::Document& doc, const std::string& key, bool dflt);

    std::vector<uint32_t> getUintArrayFromJson(rapidjson::Document& doc, const std::string& key);

    int getIntFromJson(rapidjson::Document& doc, const std::string& key, int dflt);

    int64_t getInt64FromJson(rapidjson::Document& doc, const std::string& key, int dflt);

    uint64_t getUInt64FromJson(rapidjson::Document& doc, const std::string& key, int dflt);

    std::string getStringFromJson(rapidjson::Document& doc,
                                  const std::string& key,
                                  const std::string& dflt);

    std::map<std::string, std::string> getStringStringMapFromJson(
        rapidjson::Document& doc,
        const std::string& key);

    std::map<std::string, int> getStringIntMapFromJson(rapidjson::Document& doc,
                                                       const std::string& key);

    std::string getValueFromJsonString(const std::string& key,
                                       const std::string& jsonIn);
}

#endif //WUKONG_JSON_H
