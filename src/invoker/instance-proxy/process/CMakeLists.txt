file(GLOB SOURCES_FILES *.cpp *.cc *.h)

SET(INVOKER_PROXY_SOURCES_FILES "${INVOKER_PROXY_SOURCES_FILES};${SOURCES_FILES}")

add_library(invoker-proxy ${INVOKER_PROXY_SOURCES_FILES})

target_link_libraries(invoker-proxy wukong::utils)