add_subdirectory(instance-proxy)

file(GLOB INVOKER_SOURCES_FILES *.cpp *.cc *.h)

add_executable(invoker ${INVOKER_SOURCES_FILES})
target_include_directories(invoker PRIVATE ${WUKONG_INCLUDE_DIR})
target_link_libraries(invoker
        wukong::endpoint
        wukong::client-server
        wukong::utils
        wukong::proto
        wukong::invoker::proxy
        )
add_dependencies(invoker wukong::proto wukong::invoker::proxy)