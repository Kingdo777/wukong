add_executable(tool-hello_server hello_server.cpp)
target_include_directories(tool-hello_server PUBLIC ${WUKONG_INCLUDE_DIR})
target_link_libraries(tool-hello_server PUBLIC
        common_dependencies
        wukong::endpoint
        wukong::client-server
        )

# format target
# 格式化工程文件，蛮有意思的，第一次知道clang-format命令
add_custom_target(wukong-format
        COMMAND
        ./format.sh
        WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
        )


