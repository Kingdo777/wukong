//
// Created by kingdo on 2022/4/10.
//
#include <wukong/utils/json.h>
#include <wukong/utils/log.h>
using namespace wukong::utils;
int main()
{
    Json json;
    json.parse(R"({"k1":"v1"})");
    SPDLOG_INFO(json.serialize());
    SPDLOG_INFO("k1 = {}", json.get("k1", "NONE"));
    SPDLOG_INFO("k2 = {}", json.get("k2", "NONE"));
    json.set("k2", "v2");
    SPDLOG_INFO("k2 = {}", json.get("k2", "NONE"));
    SPDLOG_INFO(json.serialize());

    json.setInt("Int", 999);
    json.setInt("Int2", -999);

    json.setInt64("Int64", 9999999);
    json.setInt64("Int64_2", -99999999);
    json.setUInt64("UInt64", 99999999999999);
    json.setUInt64("UInt64", 88888888888888);

    json.setBool("bool", true);
    SPDLOG_INFO(json.serialize());

    Json json1(R"({"k1":"v1","k2":"v2","Int":999,"Int2":-999,"Int64":9999999,"Int64_2":-99999999,"UInt64":99999999999999,"UInt64":88888888888888,"bool":true})");
    SPDLOG_INFO(json1.serialize());

    return 0;
}