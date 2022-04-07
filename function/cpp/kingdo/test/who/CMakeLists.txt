add_library(func_who MODULE main.cpp)

target_link_libraries(func_who PRIVATE
        wukong::function::interface
        )
target_include_directories(func_who PRIVATE ${WUKONG_INCLUDE_DIR})