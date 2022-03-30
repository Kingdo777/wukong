file(GLOB INVOKER_PROXY_SOURCES_FILES *.cpp *.cc *.h)

if (${USE_FIRECRACKER})
    add_subdirectory(firecracker)
elseif (${USE_USE_DOCKER})
    add_subdirectory(docker)
else ()
    add_subdirectory(process)
endif ()

target_link_libraries(invoker-proxy
        wukong::proto
        wukong::utils
        wukong::common_dependencies
        )

add_dependencies(invoker-proxy wukong::proto)

add_library(wukong::invoker::proxy ALIAS invoker-proxy)