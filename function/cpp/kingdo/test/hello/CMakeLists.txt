add_library(func_hello SHARED main.cpp)

target_link_libraries(func_hello PRIVATE
        wukong::function::interface
        )
target_include_directories(func_hello PRIVATE ${WUKONG_INCLUDE_DIR})