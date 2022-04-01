cmake_minimum_required(VERSION 3.16)
project(wukong)

# 给编译输出的结果, 添加颜色高亮效果 (https://zhuanlan.zhihu.com/p/368359203)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
endif ()

# -Wall 显示所有警告信息(https://blog.csdn.net/cjtstrive/article/details/85375477)
# -fPIC 作用于编译阶段，告诉编译器产生与位置无关代码(Position-Independent Code)（http://blog.sina.com.cn/s/blog_54f82cc201011op1.html）
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall")
# -g 使可执行文件便于调试(https://blog.csdn.net/andrewgithub/article/details/108962057)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_STANDARD 20)

option(ENABLE_TRACE "Statistics execution time" ON)
option(USE_FIRECRACKER_PROXY "use firecracker" OFF)
option(USE_DOCKER_PROXY "use docker" OFF)
option(USE_PROCESS_PROXY "use process" ON)

# 设置自定义的include目录
set(WUKONG_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include)

# 设置bin，lib的输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/bin)

# 引入第三方的module
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")
include(ExternalProjects)

if (${ENABLE_TRACE})
    add_definitions(-DENABLE_TRACE)
endif ()

# 配置使用那种Instance Proxy，数字取自圆周率
add_definitions(-DFIRECRACKER=314156)
add_definitions(-DDOCKER=265358)
add_definitions(-DPROCESS=979324)
if (${USE_FIRECRACKER_PROXY})
    message(STATUS "Using FIRECRACKER Instance Proxy")
    add_definitions(-DINSTANCE_PROXY=FIRECRACKER)
elseif (${USE_DOCKER_PROXY})
    message(STATUS "Using DOCKER Instance Proxy")
    add_definitions(-DINSTANCE_PROXY=DOCKER)
else ()
    message(STATUS "Using PROCESS Instance Proxy")
    add_definitions(-DINSTANCE_PROXY=PROCESS)
endif ()


function(wukong_lib lib_name)
    # 获取源文件
    file(GLOB SOURCES_FILE */*.cpp *.cpp *.cc)
    file(GLOB HEADERS_FILE ${WUKONG_INCLUDE_DIR}/wukong/${lib_name}/*.h)
    # 生成库文件, 注意, 如果自定义各个lib之间存在引用关系, 需要手动添加
    add_library(${lib_name} ${SOURCES_FILE} ${HEADERS_FILE})
    # 在这里如果不用PUBLIC, 那么使用这些库的可执行程序, 将无法引用第三方库的头文件. 因为我们自己的库的头文件中会引用这些头文件,
    # 这样当其他可执行程序如果引用了我们的自己的库的头文件, 就会提示找不到第三方库的头文件
    target_link_libraries(${lib_name} PRIVATE wukong::common_dependencies)
    add_library(wukong::${lib_name} ALIAS ${lib_name})

    # Ensure library generates readable stack traces
    target_compile_options(${lib_name} PUBLIC -fno-omit-frame-pointer)
    target_link_options(${lib_name} PUBLIC -Wl,--export-dynamic)
endfunction()

add_subdirectory(src/common)
add_subdirectory(src/global-gateway)
add_subdirectory(src/invoker)
add_subdirectory(src/instance)
add_subdirectory(src/faas)


add_subdirectory(tools)
add_subdirectory(function/cpp/kingdo/test/hello)
add_subdirectory(function/cpp/kingdo/test/hello1)
add_subdirectory(function/cpp/kingdo/test/hello2)


# format target
# 格式化工程文件，蛮有意思的，第一次知道clang-format命令
add_custom_target(wukong-format
        COMMAND
        ./tools/format.sh
        WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
        )