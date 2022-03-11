//
// Created by kingdo on 2022/2/26.
//

#include <wukong/utils/os.h>
#include <thread>

int wukong::utils::hardware_concurrency() { return (int) std::thread::hardware_concurrency(); }

namespace wukong::utils {
    static std::unordered_map<std::string, std::string> ipMap;

    std::string getIPFromHostname(const std::string &hostname) {
        hostent *record = gethostbyname(hostname.c_str());

        if (record == nullptr) {
            std::string errorMsg = "Could not resolve host " + hostname;
            throw std::runtime_error(errorMsg);
        }

        auto address = (in_addr *) record->h_addr;
        std::string ipAddress = inet_ntoa(*address);

        return ipAddress;
    }

    std::string getPrimaryIPForThisHost(const std::string &interface) {
        if (ipMap.count(interface) > 0) {
            return ipMap[interface];
        }

        // Get interfaces and addresses
        struct ifaddrs *allAddrs = nullptr;
        ::getifaddrs(&allAddrs);

        // Iterate through results
        struct ifaddrs *ifa = nullptr;
        std::string ipAddress;
        for (ifa = allAddrs; ifa != nullptr; ifa = ifa->ifa_next) {
            // IPV4 only
            if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) {
                continue;
            }

            // Get the IP
            void *addr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            ::inet_ntop(AF_INET, addr, addressBuffer, INET_ADDRSTRLEN);

            std::string ifaceName(ifa->ifa_name);

            if (interface.empty()) {
                // If interface not specified, attempt to work it out
                if (wukong::utils::startsWith(ifaceName, "eth") ||
                    wukong::utils::startsWith(ifaceName, "wl") ||
                    wukong::utils::startsWith(ifaceName, "en")) {
                    ipAddress = addressBuffer;
                    break;
                }
            } else if (ifaceName == interface) {
                // If we have an interface specified, take that one
                ipAddress = std::string(addressBuffer);
                break;
            } else {
                continue;
            }
        }

        if (allAddrs != nullptr) {
            ::freeifaddrs(allAddrs);
        }

        if (ipAddress.empty()) {
            fprintf(stderr, "Unable to detect IP for this host");
        }

        // Cache and return the result
        ipMap[interface] = ipAddress;
        return ipAddress;
    }
}