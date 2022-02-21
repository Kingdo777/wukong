cmake_minimum_required(VERSION 3.16)
project(wukong)

# 给编译输出的结果, 添加颜色高亮效果 (https://zhuanlan.zhihu.com/p/368359203)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
endif ()

set(CMAKE_CXX_FLAGS "-Wall")    # 显示所有警告信息(https://blog.csdn.net/cjtstrive/article/details/85375477)
set(CMAKE_CXX_FLAGS_DEBUG "-g") # 使可执行文件便于调试(https://blog.csdn.net/andrewgithub/article/details/108962057)
set(CMAKE_CXX_STANDARD 17)

# 设置自定义的include目录
set(WUKONG_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include)

# 设置bin，lib的输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/bin)

# 引入第三方的module
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")
include(ExternalProjects)

function(wukong_lib lib_name)
    # 获取源文件
    file(GLOB SOURCES_FILE *.cpp)
    file(GLOB HEADERS_FILE ${WUKONG_INCLUDE_DIR}/wukong/${lib_name}/*.h)
    # 生成库文件, 注意, 如果自定义各个lib之间存在引用关系, 需要手动添加
    add_library(${lib_name} STATIC ${SOURCES_FILE} ${HEADERS_FILE})
    # 在这里如果不用PUBLIC, 那么使用这些库的可执行程序, 将无法引用第三方库的头文件. 因为我们自己的库的头文件中会引用这些头文件,
    # 这样当其他可执行程序如果引用了我们的自己的库的头文件, 就会提示找不到第三方库的头文件
    target_include_directories(${lib_name} PUBLIC ${WUKONG_INCLUDE_DIR})
    target_link_libraries(${lib_name} PUBLIC wukong::common_dependencies)
    add_library(wukong::${lib_name} ALIAS ${lib_name})

    # Ensure library generates readable stack traces
    target_compile_options(${lib_name} PUBLIC -fno-omit-frame-pointer)
    target_link_options(${lib_name} PUBLIC -Wl,--export-dynamic)
endfunction()

add_subdirectory(src/endpoint)
add_subdirectory(src/utils)
add_subdirectory(src/bin)
