//
// Created by kingdo on 22-7-26.
//
#include <fcntl.h>
#include <unistd.h>
#include <wukong/utils/log.h>
using namespace wukong::utils;

int main()
{
    int fds[2];
    int pipe_size;
    ::pipe(fds);

    pipe_size = ::fcntl(fds[0], F_GETPIPE_SZ);
    SPDLOG_INFO("Original Pipe-Size: {}", pipe_size);

    ::fcntl(fds[0], F_SETPIPE_SZ, 1024 * 2);
    pipe_size = ::fcntl(fds[0], F_GETPIPE_SZ);
    SPDLOG_INFO("Changed Pipe-Size : {}", pipe_size);

    ::fcntl(fds[0], F_SETPIPE_SZ, 1024 * 8);
    pipe_size = ::fcntl(fds[0], F_GETPIPE_SZ);
    SPDLOG_INFO("Changed Pipe-Size : {}", pipe_size);
}

//int main()
//{
//    int fds[2];
//    ::pipe(fds);
//    ::fcntl(fds[0], F_SETPIPE_SZ, 1024 * 4);
//    if (fork() == 0)
//    {
//        close(fds[0]);
//        int write_fd = fds[1];
//        ::fcntl(write_fd, F_SETFL, ::fcntl(write_fd, F_GETFL) | O_NONBLOCK); /// make the write-fd Non-Blocking
//        char data[3072] = {};
//        auto write_size  = ::write(write_fd, data, 3072);
//        SPDLOG_INFO("Write firstly 3072-Bytes Data Done, Totally Write {} Bytes", write_size);
//        write_size = ::write(write_fd, data, 3072);
//        SPDLOG_INFO("Write secondly 3072-Bytes Data Done, Totally Write {} Bytes", write_size);
//        write_size = ::write(write_fd, data, 1024);
//        SPDLOG_INFO("Write thirdly 1024-Bytes Data Done, Totally Write {} Bytes", write_size);
//        return 0;
//    }
//    close(fds[1]);
//    sleep(1000);
//}