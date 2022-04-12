//
// Created by kingdo on 2022/3/10.
//

#include <wukong/utils/config.h>
#include <wukong/utils/os.h>
#include <wukong/utils/redis.h>

namespace wukong::utils
{

    UniqueRedisReply wrapReply(redisReply* r)
    {
        return UniqueRedisReply { r, &freeReplyObject };
    }

    Redis::Redis()
        : hostname(Config::RedisHostname())
        , ip(getIPFromHostname(hostname))
        , port(Config::RedisPort())
    {
        context = redisConnect(ip.c_str(), port);
        if (context == nullptr || context->err)
        {
            if (context)
            {
                printf("Error connecting to redis at %s: %s\n",
                       ip.c_str(),
                       context->errstr);
            }
            else
            {
                printf("Error allocating redis context\n");
            }
            throw std::runtime_error("Failed to connect to redis");
        }
    }

    Redis::~Redis()
    {
        redisFree(context);
    }

    std::string Redis::loadScript(const std::string_view scriptBody)
    {
        auto reply = safeRedisCommand("SCRIPT LOAD %b", scriptBody.data(), scriptBody.size());

        if (reply == nullptr)
        {
            throw std::runtime_error("Error loading script from Redis");
        }

        if (reply->type == REDIS_REPLY_ERROR)
        {
            throw std::runtime_error(reply->str);
        }

        std::string scriptSha = reply->str;

        return scriptSha;
    }

    UniqueRedisReply Redis::safeRedisCommand(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        UniqueRedisReply reply = wrapReply((redisReply*)redisvCommand(context, format, args));
        va_end(args);
        return reply;
    }

    std::set<std::string> Redis::extractStringSetFromReply(const redisReply& reply)
    {
        std::set<std::string> retValue;
        for (size_t i = 0; i < reply.elements; i++)
        {
            retValue.insert(reply.element[i]->str);
        }
        return retValue;
    }

    std::unordered_map<std::string, std::string> Redis::extractStringHashFromReply(const redisReply& reply)
    {
        std::unordered_map<std::string, std::string> retValue;
        for (size_t i = 0; i < reply.elements; i += 2)
        {
            retValue[reply.element[i]->str] = reply.element[i + 1]->str;
        }
        return retValue;
    }

    uint64_t Redis::scard(const std::string& key)
    {
        auto reply   = safeRedisCommand("SCARD %s", key.c_str());
        uint64_t res = reply->integer;
        return res;
    }

    std::set<std::string> Redis::smembers(const std::string& key)
    {
        auto reply = safeRedisCommand("SMEMBERS %s", key.c_str());
        std::set<std::string> result(extractStringSetFromReply(*reply));
        return result;
    }

    void Redis::srem(const std::string& key, const std::string& value)
    {

        safeRedisCommand("SREM %s %s", key.c_str(), value.c_str());
    }

    void Redis::del(const std::string& key)
    {
        safeRedisCommand("DEL %s", key.c_str());
    }

    bool Redis::sismember(const std::string& key, const std::string& value)
    {
        auto reply = safeRedisCommand("SISMEMBER %s %s", key.c_str(), value.c_str());

        bool res = reply->integer == 1;

        return res;
    }

    void Redis::sadd(const std::string& key, const std::string& value)
    {
        auto reply = safeRedisCommand("SADD %s %s", key.c_str(), value.c_str());
        if (reply->type == REDIS_REPLY_ERROR)
        {
            SPDLOG_ERROR("Failed to add {} to set {}", value, key);
            throw std::runtime_error("Failed to add element to set");
        }
    }

    std::unordered_map<std::string, std::string> Redis::hgetall(const std::string& key)
    {
        auto reply = safeRedisCommand("HGETALL %s", key.c_str());
        return extractStringHashFromReply(*reply);
    }

    void Redis::hmset(const std::string& key, const std::unordered_map<std::string, std::string>& hash)
    {
        std::string cmd = fmt::format("HMSET {} ", key);
        for (const auto& item : hash)
        {
            cmd += fmt::format(" {} {} ", item.first, item.second);
        }
        auto reply = safeRedisCommand(cmd.c_str());
        if (reply->type == REDIS_REPLY_ERROR)
        {
            SPDLOG_ERROR("Failed to add to hash {}", key);
            throw std::runtime_error("Failed to add element to set");
        }
    }

    std::string Redis::get(const std::string& key)
    {
        auto reply = safeRedisCommand("GET %s", key.c_str());
        return std::string { reply->str, reply->len };
    }

    void Redis::get_to_file(const std::string& key, const boost::filesystem::path& file_path)
    {
        WK_CHECK_WITH_EXIT(exists(file_path.parent_path()), "file_path is not exists");
        auto reply = safeRedisCommand("GET %s", key.c_str());
        std::ofstream f;
        f.open(file_path.c_str(), std::ios::out | std::ios::binary);
        f.write(reply->str, reply->len);
        f.flush();
        f.close();
    }

    void Redis::set(const std::string& key, const std::string& value)
    {
        auto reply = safeRedisCommand("SET %s %b", key.c_str(), value.c_str(), value.size());

        if (reply->type == REDIS_REPLY_ERROR)
        {
            SPDLOG_ERROR("Failed to SET {} - {}", key.c_str(), reply->str);
        }
    }
}