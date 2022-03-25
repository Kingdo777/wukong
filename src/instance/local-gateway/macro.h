//
// Created by kingdo on 2022/3/24.
//

#ifndef WUKONG_MACRO_H
#define WUKONG_MACRO_H

#include <wukong/utils/os.h>

#define RETURN_ENDPOINT_PORT(endpoint)    \
do{                                       \
    if (wukong::utils::getIntEnvVar("NEED_RETURN_PORT", 0) == 1) {\
        SPDLOG_DEBUG("NEED_RETURN_PORT");       \
        wukong::utils::write_int(3, endpoint.port());  \
    }                               \
}while(false);

#endif //WUKONG_MACRO_H
