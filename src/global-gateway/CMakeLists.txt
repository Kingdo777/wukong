file(GLOB GLOBAL_GATEWAY_SOURCES_FILE *.cpp *.cc *.h)

add_executable(global-gateway ${GLOBAL_GATEWAY_SOURCES_FILE})
target_link_libraries(global-gateway
        wukong::endpoint
        wukong::client-server
        wukong::utils
        wukong::proto
        wukong::common_dependencies
        )
add_dependencies(global-gateway wukong::proto)