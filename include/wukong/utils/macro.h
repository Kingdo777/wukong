#pragma once

#include <climits> // for PIPE_BUF
#include <wukong/utils/log.h>

#define MAGIC_NUMBER_WUKONG (0x424E4F44474E494B)
#define magic_t uint64_t
#define MAGIC_NUMBER_CHECK(n) (!((n) ^ MAGIC_NUMBER_WUKONG))

#define STORAGE_FUNCTION_NAME "__wukong_storage_internal_function__"

#define STORAGE_FUNCTION_DEFAULT_SIZE (64 * 1024 * 1024) // 64MB

#define NAMED_PIPE_PATH "/tmp/wukong/named-pipe"

#define WUKONG_UUID_SIZE 512

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

#define WUKONG_FUNC_NAME_SIZE 256
#define WUKONG_NAMED_PIPE_SIZE 1024

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

#define WK_CHECK_WITH_ERROR_HANDLE_and_RETURN_VALUR(condition, msg, ret, onFiled) \
    do                                                                            \
    {                                                                             \
        bool check = (condition);                                                 \
        if (check)                                                                \
            break;                                                                \
        WK_CHECK(check, msg);                                                     \
        (onFiled)(msg);                                                           \
        return (ret);                                                             \
    } while (false)

#define WK_CHECK_WITH_ERROR_HANDLE_and_RETURN(condition, msg, onFiled) \
    do                                                                 \
    {                                                                  \
        bool check = (condition);                                      \
        if (check)                                                     \
            break;                                                     \
        WK_CHECK(check, msg);                                          \
        (onFiled)(msg);                                                \
        return;                                                        \
    } while (false)

#define WK_CHECK_WITH_ERROR_HANDLE(condition, msg, onFiled) \
    do                                                      \
    {                                                       \
        bool check = (condition);                           \
        if (check)                                          \
            break;                                          \
        WK_CHECK(check, msg);                               \
        onFiled(msg);                                       \
    } while (false)

#define WK_CHECK_WITH_EXIT(condition, msg) \
    do                                     \
    {                                      \
        bool check = (condition);          \
        if (check)                         \
            break;                         \
        WK_CHECK(check, msg);              \
        exit(EXIT_FAILURE);                \
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

#define WK_FUNC_CHECK_WITH_ERROR_HANDLE(condition, msg_, onFiled) \
    do                                                            \
    {                                                             \
        if (!(condition))                                         \
        {                                                         \
            msg = (msg_);                                         \
            onFiled();                                            \
            WK_FUNC_RETURN();                                     \
        }                                                         \
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

#define WK_CHECK_FUNC_RET_WITH_EXIT(ret)                           \
    do                                                             \
    {                                                              \
        const auto& ret_var__ = (ret);                             \
        WK_CHECK_WITH_EXIT((ret_var__).first, (ret_var__).second); \
    } while (false)

#define WK_CHECK_STATE(s1)                                              \
    do                                                                  \
    {                                                                   \
        WK_CHECK((state == (s1)), "SubProcess Status is Not Expected"); \
    } while (false)
#define WK_CHECK_STATE_2(s1, s2)                                                         \
    do                                                                                   \
    {                                                                                    \
        WK_CHECK((state == (s1) || state == (s2)), "SubProcess Status is Not Expected"); \
    } while (false)
#define WK_CHECK_STATE_3(s1, s2, s3)                                                                      \
    do                                                                                                    \
    {                                                                                                     \
        WK_CHECK((state == (s1) || state == (s2) || state == (s3)), "SubProcess Status is Not Expected"); \
    } while (false)
#define WK_CHECK_STATE_WITH_ASSERT(s1)                                              \
    do                                                                              \
    {                                                                             \
        WK_CHECK_WITH_EXIT((state == (s1)), "SubProcess Status is Not Expected"); \
    } while (false)
#define WK_CHECK_STATE_WITH_ASSERT_2(s1, s2)                                                         \
    do                                                                                               \
    {                                                                                              \
        WK_CHECK_WITH_EXIT((state == (s1) || state == (s2)), "SubProcess Status is Not Expected"); \
    } while (false)
#define WK_CHECK_STATE_WITH_ASSERT_3(s1, s2, s3)                                                                      \
    do                                                                                                                \
    {                                                                                                               \
        WK_CHECK_WITH_EXIT((state == (s1) || state == (s2) || state == (s3)), "SubProcess Status is Not Expected"); \
    } while (false)
