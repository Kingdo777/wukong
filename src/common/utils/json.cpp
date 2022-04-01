//
// Created by kingdo on 2022/2/26.
//
#include <wukong/utils/json.h>

namespace wukong::utils
{

    bool getBoolFromJson(rapidjson::Document& doc, const std::string& key, bool dflt)
    {
        rapidjson::Value::MemberIterator it = doc.FindMember(key.c_str());
        if (it == doc.MemberEnd())
        {
            return dflt;
        }
        return it->value.GetBool();
    }

    std::vector<uint32_t> getUintArrayFromJson(rapidjson::Document& doc, const std::string& key)
    {
        rapidjson::Value::MemberIterator it = doc.FindMember(key.c_str());
        std::vector<uint32_t> result;
        if (it == doc.MemberEnd())
        {
            return result;
        }

        for (const auto& i : it->value.GetArray())
        {
            result.emplace_back(i.GetUint());
        }

        return result;
    }

    int getIntFromJson(rapidjson::Document& doc, const std::string& key, int dflt)
    {
        rapidjson::Value::MemberIterator it = doc.FindMember(key.c_str());
        if (it == doc.MemberEnd())
        {
            return dflt;
        }

        return it->value.GetInt();
    }

    int64_t getInt64FromJson(rapidjson::Document& doc, const std::string& key, int dflt)
    {
        rapidjson::Value::MemberIterator it = doc.FindMember(key.c_str());
        if (it == doc.MemberEnd())
        {
            return dflt;
        }

        return it->value.GetInt64();
    }

    uint64_t getUInt64FromJson(rapidjson::Document& doc, const std::string& key, int dflt)
    {
        rapidjson::Value::MemberIterator it = doc.FindMember(key.c_str());
        if (it == doc.MemberEnd())
        {
            return dflt;
        }

        return it->value.GetUint64();
    }

    std::string getStringFromJson(rapidjson::Document& doc, const std::string& key, const std::string& dflt)
    {
        rapidjson::Value::MemberIterator it = doc.FindMember(key.c_str());
        if (it == doc.MemberEnd())
        {
            return dflt;
        }

        const char* valuePtr = it->value.GetString();
        return std::string { valuePtr, valuePtr + it->value.GetStringLength() };
    }

    std::map<std::string, std::string> getStringStringMapFromJson(rapidjson::Document& doc, const std::string& key)
    {
        std::map<std::string, std::string> map;

        rapidjson::Value::MemberIterator it = doc.FindMember(key.c_str());
        if (it == doc.MemberEnd())
        {
            return map;
        }

        const char* valuePtr = it->value.GetString();
        std::stringstream ss(
            std::string(valuePtr, valuePtr + it->value.GetStringLength()));
        std::string keyVal;
        while (std::getline(ss, keyVal, ','))
        {
            auto pos        = keyVal.find(':');
            std::string key = keyVal.substr(0, pos);
            map[key]        = keyVal.erase(0, pos + sizeof(char));
        }

        return map;
    }

    std::map<std::string, int> getStringIntMapFromJson(rapidjson::Document& doc, const std::string& key)
    {
        std::map<std::string, int> map;

        rapidjson::Value::MemberIterator it = doc.FindMember(key.c_str());
        if (it == doc.MemberEnd())
        {
            return map;
        }

        const char* valuePtr = it->value.GetString();
        std::stringstream ss(
            std::string(valuePtr, valuePtr + it->value.GetStringLength()));
        std::string keyVal;
        while (std::getline(ss, keyVal, ','))
        {
            auto pos         = keyVal.find(':');
            std::string key_ = keyVal.substr(0, pos);
            int val          = std::stoi(keyVal.erase(0, pos + sizeof(char)));
            map[key_]        = val;
        }

        return map;
    }

    std::string getValueFromJsonString(const std::string& key, const std::string& jsonIn)
    {
        rapidjson::MemoryStream ms(jsonIn.c_str(), jsonIn.size());
        rapidjson::Document d;
        d.ParseStream(ms);

        std::string result = getStringFromJson(d, key, "");
        return result;
    }
}