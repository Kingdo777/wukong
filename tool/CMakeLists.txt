file(GLOB GLOBAL_GATEWAY_SOURCES_FILE *.cpp *.cc *.h)

add_executable(tool-hello_server ${GLOBAL_GATEWAY_SOURCES_FILE})
target_include_directories(tool-hello_server PUBLIC ${WUKONG_INCLUDE_DIR})
target_link_libraries(tool-hello_server common_dependencies wukong::proto)