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

    class Json
    {
    public:
        Json()
        {
            doc.SetObject();
        }
        explicit Json(const std::string& string)
        {
            parse(string);
        }
        void parse(const std::string& str)
        {
            rapidjson::MemoryStream ms(str.c_str(), str.size());
            doc.ParseStream(ms);
        }
        std::string serialize()
        {
            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
            doc.Accept(writer);
            return sb.GetString();
        }
        std::string get(const std::string& key, const std::string& dflt = "")
        {
            auto it = doc.FindMember(key.c_str());
            return doc.HasMember(key.c_str()) ? std::string { it->value.GetString(), it->value.GetStringLength() } : dflt;
        }

        bool getBool(const std::string& key, bool dflt)
        {
            return doc.HasMember(key.c_str()) ? doc.FindMember(key.c_str())->value.GetBool() : dflt;
        }
        int getInt(const std::string& key, int dflt)
        {
            return doc.HasMember(key.c_str()) ? doc.FindMember(key.c_str())->value.GetInt() : dflt;
        }
        int64_t getInt64(const std::string& key, int64_t dflt)
        {
            return doc.HasMember(key.c_str()) ? doc.FindMember(key.c_str())->value.GetInt64() : dflt;
        }
        uint64_t getUInt64(const std::string& key, uint64_t dflt)
        {
            return doc.HasMember(key.c_str()) ? doc.FindMember(key.c_str())->value.GetUint64() : dflt;
        }

        void set(const std::string& key, const std::string& value)
        {
            rapidjson::Document::AllocatorType& a = doc.GetAllocator();
            doc.AddMember(rapidjson::Value(key.c_str(), key.size(), a).Move(),
                          rapidjson::Value(value.c_str(), value.size(), a).Move(), a);
        }
        void setInt(const std::string& key, int value)
        {
            rapidjson::Document::AllocatorType& a = doc.GetAllocator();
            doc.AddMember(rapidjson::Value(key.c_str(), key.size(), a).Move(),
                          value, a);
        }
        void setInt64(const std::string& key, int64_t value)
        {
            rapidjson::Document::AllocatorType& a = doc.GetAllocator();
            doc.AddMember(rapidjson::Value(key.c_str(), key.size(), a).Move(),
                          value, a);
        }
        void setUInt64(const std::string& key, uint64_t value)
        {
            rapidjson::Document::AllocatorType& a = doc.GetAllocator();
            doc.AddMember(rapidjson::Value(key.c_str(), key.size(), a).Move(),
                          value, a);
        }
        void setBool(const std::string& key, bool value)
        {
            rapidjson::Document::AllocatorType& a = doc.GetAllocator();
            doc.AddMember(rapidjson::Value(key.c_str(), key.size(), a).Move(),
                          value, a);
        }

    private:
        rapidjson::Document doc;
    };

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

#endif // WUKONG_JSON_H
