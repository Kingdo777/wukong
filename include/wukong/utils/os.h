//
// Created by kingdo on 2022/2/26.
//

#ifndef WUKONG_OS_H
#define WUKONG_OS_H

#include <wukong/utils/errors.h>
#include <wukong/utils/macro.h>
#include <wukong/utils/string-tool.h>

#include <arpa/inet.h>
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <grp.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

namespace wukong::utils
{

    int hardware_concurrency();

    ///################## NetWork ############################
    ///#######################################################

    std::string getIPFromHostname(const std::string& hostname);

    /**
     * Returns the IP for the given interface, or picks one based on
     * an "appropriate" interface name.
     */
    std::string getPrimaryIPForThisHost(const std::string& interface);

    int nonblock_ioctl(int fd, int set);

    int cloexec_ioctl(int fd, int set);

    int nonblock_fcntl(int fd, int set);

    int cloexec_fcntl(int fd, int set);

    int socket_pair(int fds[2]);

    int make_pipe(int fds[2], int flags);

    int kill(pid_t pid, int signum);

    template <typename T>
    ssize_t write_2_fd(int fd, T val)
    {
        ssize_t n;
        do
            n = ::write(fd, &val, sizeof(T));
        while (n == -1 && errno == EINTR);
        return n;
    }

#define WRITE_2_FD(fd, val)                                                                \
    do                                                                                     \
    {                                                                                      \
        WK_CHECK_WITH_EXIT((fd) != -1, "fd == -1");                                        \
        auto size = wukong::utils::write_2_fd((fd), (val));                                \
        if (size == -1)                                                                    \
        {                                                                                  \
            SPDLOG_ERROR("write fd is wrong : {}", wukong::utils::errors());               \
            assert(false);                                                                 \
        }                                                                                  \
        else                                                                               \
            WK_CHECK(size == sizeof(val), "read_from_fd failed, read-size is not expect"); \
    } while (false)

#define WRITE_2_FD_goto(fd, val)                                                           \
    do                                                                                     \
    {                                                                                      \
        WK_CHECK_WITH_EXIT((fd) != -1, "fd == -1");                                        \
        auto size = wukong::utils::write_2_fd((fd), (val));                                \
        if (size == -1)                                                                    \
        {                                                                                  \
            if (errno == EAGAIN || errno == EWOULDBLOCK)                                   \
                goto write_fd_EAGAIN;                                                      \
            SPDLOG_ERROR("write fd is wrong : {}", wukong::utils::errors());               \
            assert(false);                                                                 \
        }                                                                                  \
        else                                                                               \
            WK_CHECK(size == sizeof(val), "read_from_fd failed, read-size is not expect"); \
    } while (false)

#define WRITE_2_FD_original(fd, buf_address, buf_size)                       \
    do                                                                       \
    {                                                                        \
        WK_CHECK_WITH_EXIT((fd) != -1, "fd == -1");                          \
        auto size = ::write((fd), (buf_address), (buf_size));                \
        if (size == -1)                                                      \
        {                                                                    \
            SPDLOG_ERROR("write fd is wrong : {}", wukong::utils::errors()); \
            assert(false);                                                   \
        }                                                                    \
    } while (false)

    template <typename T>
    ssize_t read_from_fd(int fd, T* val)
    {
        // TODO 应该结合epoll实现timeout机制
        ssize_t n;
        do
            n = ::read(fd, val, sizeof(T));
        while (n == -1 && errno == EINTR);
        return n;
    }

#define READ_FROM_FD(fd, val)                                                                                         \
    do                                                                                                                \
    {                                                                                                                 \
        auto size = wukong::utils::read_from_fd((fd), (val));                                                         \
        if (size == -1)                                                                                               \
        {                                                                                                             \
            SPDLOG_ERROR("read fd is wrong : {}", wukong::utils::errors());                                           \
            assert(false);                                                                                            \
        }                                                                                                             \
        else                                                                                                          \
            WK_CHECK(size == sizeof(*(val)), fmt::format("read_from_fd `{}` failed, read-size is not expect", size)); \
    } while (false)

#define READ_FROM_FD_original(fd, buf_address, buf_size)                    \
    do                                                                      \
    {                                                                       \
        auto size = ::read((fd), (buf_address), (buf_size));                \
        if (size == -1)                                                     \
        {                                                                   \
            SPDLOG_ERROR("read fd is wrong : {}", wukong::utils::errors()); \
        }                                                                   \
    } while (false)

#define READ_FROM_FD_goto(fd, val)                                                                                    \
    do                                                                                                                \
    {                                                                                                                 \
        auto size = wukong::utils::read_from_fd((fd), (val));                                                         \
        if (size == -1)                                                                                               \
        {                                                                                                             \
            if (errno == EAGAIN || errno == EWOULDBLOCK)                                                              \
                goto read_fd_EAGAIN;                                                                                  \
            SPDLOG_ERROR("read fd is wrong : {}", wukong::utils::errors());                                           \
            assert(false);                                                                                            \
        }                                                                                                             \
        else                                                                                                          \
            WK_CHECK(size == sizeof(*(val)), fmt::format("read_from_fd `{}` failed, read-size is not expect", size)); \
    } while (false)

#define READ_FROM_FD_original_goto(fd, buf_address, buf_size)               \
    do                                                                      \
    {                                                                       \
        auto size = ::read((fd), (buf_address), (buf_size));                \
        if (size == -1)                                                     \
        {                                                                   \
            if (errno == EAGAIN || errno == EWOULDBLOCK)                    \
                goto read_fd_EAGAIN;                                        \
            SPDLOG_ERROR("read fd is wrong : {}", wukong::utils::errors()); \
            assert(false);                                                  \
        }                                                                   \
    } while (false)

}
#endif // WUKONG_OS_H
