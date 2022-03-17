//
// Created by kingdo on 2022/3/16.
//

#include <wukong/utils/dl.h>
#include <wukong/utils/log.h>

int main() {
    wukong::utils::Lib lib;
    lib.open("/home/kingdo/CLionProjects/wukong/cmake-build-debug/out/lib/libfunc_hello.so");
    std::string (*f)();
    if (lib.sym("_Z9faas_pingB5cxx11v", (void **) &f))
        SPDLOG_ERROR(lib.errors());
    else {
        SPDLOG_INFO(f());
    }
    return 0;
}