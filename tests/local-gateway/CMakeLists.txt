file(GLOB TEST_LOCAL_GATEWAY_SOURCES_FILE ../../src/instance/local-gateway/LocalGateway.cpp ../../src/instance/local-gateway/LocalGatewayClientServer.cpp ../../src/instance/local-gateway/LocalGatewayEndpoint.cpp)

add_executable(test-localgateway local-gateway-test.cpp ${TEST_LOCAL_GATEWAY_SOURCES_FILE})
target_link_libraries(test-localgateway
        wukong::endpoint
        wukong::client-server
        wukong::utils
        wukong::proto
        wukong::common_dependencies)
target_include_directories(test-localgateway PUBLIC ${WUKONG_INCLUDE_DIR})