include(FindGit)
find_package(Git REQUIRED)
include(FetchContent)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

#------------- 将所有的第三方包, 全部打包成一个依赖库 ------------------------
# 创建一个INTERFACE库，一个INTERFACE库不会直接创建编译目标文件, 与下面的INTERFACE并不是同一概念
add_library(common_dependencies INTERFACE)

#----------------------- RapidJSON ------------------------------------
find_package(RapidJSON QUIET)
if (NOT RapidJSON_FOUND)
    message("RapidJSON not found. Downloading it from source...")
    FetchContent_Declare(
            RapidJSON
            GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
            GIT_TAG v1.1.0
            GIT_SHALLOW true
    )
    set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "")
    set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "")
    set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "")
    FetchContent_MakeAvailable(RapidJSON)
    target_include_directories(common_dependencies INTERFACE ${rapidjson_SOURCE_DIR}/include)
else ()
    target_include_directories(common_dependencies INTERFACE ${RAPIDJSON_INCLUDE_DIRS})
endif ()

#----------------------- SpdLog ------------------------------------
find_package(spdlog QUIET)
if (NOT spdlog_FOUND)
    message("SpdLog not found. Downloading it from source...")
    FetchContent_Declare(
            spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG v1.9.2
            GIT_SHALLOW true
    )
    set(SPDLOG_BUILD_ALL OFF CACHE BOOL "")
    set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "")
    set(SPDLOG_ENABLE_PCH OFF CACHE BOOL "")
    set(SPDLOG_BUILD_EXAMPLE_HO OFF CACHE BOOL "")
    set(PISTACHE_BUILD_TESTS OFF CACHE BOOL "")
    set(SPDLOG_BUILD_TESTS_HO OFF CACHE BOOL "")
    set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "")
    set(SPDLOG_SANITIZE_ADDRESS OFF CACHE BOOL "")
    set(SPDLOG_BUILD_WARNINGS OFF CACHE BOOL "")

    FetchContent_MakeAvailable(spdlog)
endif ()
# INTERFACE 表示, wukong::common_dependencies本身不会链接后面一系列的库及其头文件,
# 而链接wukong::common_dependencies的库或者可执行程序会链接这些库及其头文件,
# INTERFACE与PRIVATE的效果刚好相反. 而PUBLIC则是两者都会链接库及其头文件.
# 详见https://blog.csdn.net/weixin_43862847/article/details/119762230
target_link_libraries(common_dependencies INTERFACE spdlog)

#----------------------- pistache ------------------------------------
find_package(PkgConfig REQUIRED)
set(ENV{PKG_CONFIG_PATH}
        /usr/local/lib64/pkgconfig:$ENV{PKG_CONFIG_PATH})
pkg_check_modules(Pistache QUIET IMPORTED_TARGET libpistache)
if (NOT Pistache_FOUND)
    message("pistache not found.  Downloading it from source...")
    FetchContent_Declare(
            pistache
            GIT_REPOSITORY https://github.com/Kingdo777/pistache
            GIT_SHALLOW true
    )
    set(PISTACHE_BUILD_TESTS OFF CACHE BOOL "")
    set(PISTACHE_BUILD_FUZZ OFF CACHE BOOL "")
    set(PISTACHE_BUILD_EXAMPLES OFF CACHE BOOL "")
    set(DPISTACHE_BUILD_DOCS OFF CACHE BOOL "")

    FetchContent_MakeAvailable(pistache)
    target_link_libraries(common_dependencies INTERFACE pistache_static)
else ()
    target_link_libraries(common_dependencies INTERFACE PkgConfig::Pistache)
endif ()

#----------------------- hiredis ------------------------------------
find_package(hiredis QUIET)
if (NOT hiredis_FOUND)
    message("hiredis not found.  Downloading it from source...")
    FetchContent_Declare(
            hiredis
            GIT_REPOSITORY https://github.com/redis/hiredis.git
            GIT_TAG v1.0.2
            GIT_SHALLOW true
    )
    set(ENABLE_SSL OFF CACHE BOOL "")
    set(DISABLE_TESTS ON CACHE BOOL "")
    set(ENABLE_SSL_TESTS OFF CACHE BOOL "")

    FetchContent_MakeAvailable(hiredis)
else ()
    # 这里是因为，通过make install之后，头文件将会被安装到 PREFIX/hiredis/hiredis.h
    # 但是通过FetchContent获取时，头文件是没有前缀hiredis的，因此为了保证一致，在find_package成功时，添加以下头文件引入目录
    target_include_directories(common_dependencies INTERFACE ${hiredis_INCLUDE_DIRS}/hiredis)
endif ()
target_link_libraries(common_dependencies INTERFACE hiredis)

#----------------------- fmt ------------------------------------
find_package(fmt QUIET)
if (NOT fmt_FOUND)
    message("fmt not found.  Downloading it from source...")
    FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG        8.1.1
            GIT_SHALLOW true
    )

    FetchContent_MakeAvailable(fmt)
    target_link_libraries(common_dependencies INTERFACE fmt)
else ()
    target_link_libraries(common_dependencies INTERFACE fmt)
endif ()

#----------------------- libuv ------------------------------------
find_package(libuv QUIET)
if (NOT libuv_FOUND)
    message("Libuv not found. Downloading it from source...")
    FetchContent_Declare(
            Libuv
            GIT_REPOSITORY https://github.com/libuv/libuv.git
            GIT_TAG v1.44.1
            GIT_SHALLOW true
    )
    set(DBUILD_TESTING OFF CACHE BOOL "")
    set(LIBUV_BUILD_TESTS OFF CACHE BOOL "")
    set(LIBUV_BUILD_BENCH OFF CACHE BOOL "")
    FetchContent_MakeAvailable(Libuv)
endif ()
target_link_libraries(common_dependencies INTERFACE uv)

#---------------------------其他库-------------------------------------
target_link_libraries(common_dependencies INTERFACE
        pthread
        dl
        ${Protobuf_LIBRARIES}
        )

#--------------------------------------------------------------------
# 起一个别名, 因为当前CMAKE不支持直接生成wukong::格式的lib
add_library(wukong::common_dependencies ALIAS common_dependencies)