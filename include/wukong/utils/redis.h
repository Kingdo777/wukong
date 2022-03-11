//
// Created by kingdo on 2022/3/10.
//

#ifndef WUKONG_REDIS_H
#define WUKONG_REDIS_H

#include <wukong/utils/log.h>
#include <wukong/utils/string-tool.h>

#include <string>
#include <mutex>
#include <hiredis.h>
#include <memory>
#include <set>
#include <unordered_map>
#include <fmt/format.h>


/// Redis_keys

#define SET_INVOKER_ID "SET_invokerID"
#define HASH_INVOKER    "HASH_invoker"

namespace wukong::utils {

    using UniqueRedisReply =
    std::unique_ptr<redisReply, decltype(&freeReplyObject)>;

    UniqueRedisReply wrapReply(redisReply *r);

    class Redis {

    public:
        static Redis &getRedis() {
            // Hiredis requires one instance per thread
            static thread_local Redis r;
            return r;
        }

        Redis();

        ~Redis();

        /// 删除key
        void del(const std::string &key);

        /// 获取集合中的元素个数
        uint64_t scard(const std::string &key);

        /// 返回集合的所有的元素
        std::set<std::string> smembers(const std::string &key);

        /// 删除集合中的某个元素
        void srem(const std::string &key, const std::string &value);

        /// 判断集合中是否包含某一元素
        bool sismember(const std::string &key, const std::string &value);

        /// 增加一个元素到集合当中
        void sadd(const std::string &key, const std::string &value);

        /// 返回Hash中的所有字段
        std::unordered_map<std::string, std::string> hgetall(const std::string &key);

        /// 一次性设置Hash的多个字段
        void hmset(const std::string &key, const std::unordered_map<std::string, std::string> &hash);

    private:
        std::string hostname;
        std::string ip;
        int port;
        redisContext *context;

        static std::set<std::string> extractStringSetFromReply(const redisReply &reply);

        static std::unordered_map<std::string, std::string> extractStringHashFromReply(const redisReply &reply);

        std::string loadScript(std::string_view scriptBody);

        UniqueRedisReply safeRedisCommand(const char *format, ...);

    };

//    class RedisInstance {
//    public:
//        explicit RedisInstance();
//
//        std::string delifeqSha;
//        std::string schedPublishSha;
//
//        std::string ip;
//        std::string hostname;
//        int port;
//
//    private:
//
//        std::mutex scriptsLock;
//
//        std::string loadScript(redisContext *context,
//                               std::string_view scriptBody);
//
//        // Script to delete a key if it equals a given value
//        const std::string_view delifeqCmd = R"---(
//if redis.call('GET', KEYS[1]) == ARGV[1] then
//    return redis.call('DEL', KEYS[1])
//else
//    return 0
//end
//)---";
//
//        // Script to push and expire function execution results avoiding extra
//        // copies and round-trips
//        const std::string_view schedPublishCmd = R"---(
//local key = KEYS[1]
//local status_key = KEYS[2]
//local result = ARGV[1]
//local result_expiry = tonumber(ARGV[2])
//local status_expiry = tonumber(ARGV[3])
//redis.call('RPUSH', key, result)
//redis.call('EXPIRE', key, result_expiry)
//redis.call('SET', status_key, result)
//redis.call('EXPIRE', status_key, status_expiry)
//return 0
//)---";
//    };
}

#endif //WUKONG_REDIS_H
