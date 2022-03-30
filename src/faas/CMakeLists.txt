file(GLOB FUNC_INTERFACE_SOURCES_FILE *.cpp *.cc *.h)

add_library(function-interface ${FUNC_INTERFACE_SOURCES_FILE})
target_link_libraries(function-interface PUBLIC
        wukong::proto
        wukong::utils
        )
target_include_directories(function-interface PRIVATE ${WUKONG_INCLUDE_DIR})
add_library(wukong::function::interface ALIAS function-interface)