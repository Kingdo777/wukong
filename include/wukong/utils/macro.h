#pragma once

#include <climits> // for PIPE_BUF
#include <wukong/utils/log.h>

// We're always on x86_64
#define WUKONG_CACHE_LINE_SIZE 64
#define WUKONG_PAGE_SIZE 4096

#ifndef WUKONG_FILE_CREAT_MODE
#define WUKONG_FILE_CREAT_MODE 0664
#endif

#ifndef WUKONG_DIR_CREAT_MODE
#define WUKONG_DIR_CREAT_MODE 0775
#endif

#define WUKONG_CGROUP_DIR_CREAT_MODE 0755

#ifndef WUKONG_MESSAGE_SIZE
#define WUKONG_MESSAGE_SIZE 2048
#endif

#ifndef WUKONG_MEMORY_UNIT_SIZE
#define WUKONG_MEMORY_UNIT_SIZE 64 // 64MB
#endif

#ifndef WUKONG_CPU_UNIT_SIZE
#define WUKONG_CPU_UNIT_SIZE 100 // 0.1 core
#endif

static_assert(WUKONG_MESSAGE_SIZE <= PIPE_BUF,
              "WUKONG_MESSAGE_SIZE cannot exceed PIPE_BUF");
static_assert(WUKONG_MESSAGE_SIZE >= WUKONG_CACHE_LINE_SIZE * 2,
              "WUKONG_MESSAGE_SIZE is too small");

#define WK_CHECK_WITH_ASSERT(condition, msg) \
    do                                       \
    {                                        \
        bool check = (condition);            \
        if (check)                           \
            break;                           \
        WK_CHECK(check, msg);                \
        assert(false);                       \
    } while (false)

#define WK_CHECK(condition, msg) \
    do                           \
    {                            \
        if (!(condition))        \
            SPDLOG_ERROR((msg)); \
    } while (false)

typedef std::pair<bool, std::string> WK_FUNC_RETURN_TYPE;
#define WK_FUNC_RETURN()                     \
    do                                       \
    {                                        \
        return std::make_pair(success, msg); \
    } while (false)

#define WK_FUNC_START()      \
    bool success    = false; \
    std::string msg = "ok";

#define WK_FUNC_END() \
    success = true;   \
    WK_FUNC_RETURN();

#define WK_FUNC_CHECK(condition, msg_) \
    do                                 \
    {                                  \
        if (!(condition))              \
        {                              \
            msg = (msg_);              \
            WK_FUNC_RETURN();          \
        }                              \
    } while (false)

#define WK_FUNC_CHECK_RET(ret)                                \
    do                                                        \
    {                                                         \
        const auto& ret_var__ = (ret);                        \
        WK_FUNC_CHECK((ret_var__).first, (ret_var__).second); \
    } while (false)

#define WK_CHECK_FUNC_RET(ret)                           \
    do                                                   \
    {                                                    \
        const auto& ret_var__ = (ret);                   \
        WK_CHECK((ret_var__).first, (ret_var__).second); \
    } while (false)

#define WK_CHECK_FUNC_RET_WITH_ASSERT(ret)                           \
    do                                                               \
    {                                                                \
        const auto& ret_var__ = (ret);                               \
        WK_CHECK_WITH_ASSERT((ret_var__).first, (ret_var__).second); \
    } while (false)

typedef struct FunctionInfo
{
    char lib_path[256] = { 0 };
    int threads        = 0;
} FunctionInfo;

typedef struct Result
{
    bool success                   = false;
    char data[WUKONG_MESSAGE_SIZE] = { 0 };
    uint64_t msg_id                = 0;
} FuncResult;

typedef struct InternalRequest
{
    char funcname[32]              = { 0 };
    char args[WUKONG_MESSAGE_SIZE] = { 0 };
    uint64_t request_id                = 0;
} InternalRequest;

typedef struct InternalResponse
{
    bool success                   = false;
    char data[WUKONG_MESSAGE_SIZE] = { 0 };
    uint64_t request_id                = 0;
} InternalResponse;
