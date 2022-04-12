add_library(func_toupper MODULE main.cpp)

target_link_libraries(func_toupper PRIVATE
        wukong::function::interface
        )
target_include_directories(func_toupper PRIVATE ${WUKONG_INCLUDE_DIR})