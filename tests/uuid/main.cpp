//
// Created by kingdo on 22-7-24.
//
#include <boost/uuid/uuid.hpp> // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp> // streaming operators etc.
#include <iostream>
#include <wukong/utils/log.h>
#include <wukong/utils/uuid.h>
using namespace wukong::utils;
using namespace std;


int main()
{
    SPDLOG_ERROR(uuid());
    for (int i = 0; i < 10; i++)
    {
        if (fork() == 0)
        {
            SPDLOG_ERROR(uuid());
            return 0;
        }
    }
}

// int main()
//{
//    boost::uuids::uuid uuid = boost::uuids::string_generator()("{0123456789abcdef0123456789abcdef}");
//    std::cout << uuid<<endl;
//    std::cout << uuid;
//    boost::uuids::string_generator sgen;
//    boost::uuids::uuid u1 = sgen("{0123456789abcdef0123456789abcdef}");
//     std::cout << u1 << std::endl;
//    boost::uuids::uuid u2 = sgen("01234567-89ab-cdef-0123-456789abcdef");
//     std::cout << u2 << std::endl;
//}
//
//int main()
//{
//    SPDLOG_ERROR(uuid());
//    for (int i = 0; i < 10; i++)
//    {
//        if (fork() == 0)
//        {
//            uuid_reset();
//            SPDLOG_ERROR(uuid());
//            return 0;
//        }
//    }
//}