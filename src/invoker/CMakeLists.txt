file(GLOB GLOBAL_GATEWAY_SOURCES_FILE *.cpp *.cc *.h)

add_executable(invoker ${GLOBAL_GATEWAY_SOURCES_FILE})
target_link_libraries(invoker
        wukong::endpoint
        wukong::client-server
        wukong::utils
        wukong::proto
        )
add_dependencies(invoker wukong::proto)