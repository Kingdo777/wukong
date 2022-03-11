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
}
#endif //WUKONG_OS_H
