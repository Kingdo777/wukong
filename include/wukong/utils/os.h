//
// Created by kingdo on 2022/2/26.
//

#ifndef WUKONG_OS_H
#define WUKONG_OS_H

#include <wukong/utils/string-tool.h>

#include <thread>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <unordered_map>
#include <sys/wait.h>
#include <asm-generic/ioctls.h>
#include <sys/ioctl.h>
#include <grp.h>
#include <fcntl.h>
#include <unistd.h>

namespace wukong::utils {

    int hardware_concurrency();

    ///################## NetWork ############################
    ///#######################################################

    std::string getIPFromHostname(const std::string &hostname);

    /**
    * Returns the IP for the given interface, or picks one based on
    * an "appropriate" interface name.
    */
    std::string getPrimaryIPForThisHost(const std::string &interface);

    int nonblock_ioctl(int fd, int set);

    int cloexec_ioctl(int fd, int set);


    int nonblock_fcntl(int fd, int set);


    int cloexec_fcntl(int fd, int set);

    int socket_pair(int fds[2]);

    int make_pipe(int fds[2], int flags);

    void write_int(int fd, int val);

    int read_int(int fd);

}
#endif //WUKONG_OS_H
