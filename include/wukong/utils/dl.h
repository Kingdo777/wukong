//
// Created by kingdo on 2022/3/16.
//

#ifndef WUKONG_DL_H
#define WUKONG_DL_H

#include <wukong/utils/radom.h>
#include <dlfcn.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>

namespace wukong::utils {

    /// 以下代码，参考自libuv的`src/unix/dl.c`

    char *wukong_strdup(const char *s);

    void wukong_free(void *ptr);

    class Lib {
    public:

        Lib() = default;

        int open(const std::string &code);

        int open(const char *filename);

        void close();

        int sym(const char *name, void **ptr);

        [[nodiscard]] const char *errors() const;

        int error();

        void *handle = nullptr;

        char *errmsg = nullptr;
    };

}

#endif //WUKONG_DL_H
