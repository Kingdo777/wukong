file(GLOB LOCAL_GATEWAY_SOURCES_FILE *.cpp *.cc *.h)

add_executable(instance-local-gateway ${LOCAL_GATEWAY_SOURCES_FILE})
target_link_libraries(instance-local-gateway
        wukong::endpoint
        wukong::client-server
        wukong::utils
        wukong::proto
        )
add_dependencies(instance-local-gateway wukong::proto)