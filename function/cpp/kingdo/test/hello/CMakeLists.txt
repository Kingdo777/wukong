add_library(func_hello MODULE main.cpp)

target_link_libraries(func_hello PRIVATE
        wukong::function::interface
        wukong::utils
        wukong::common_dependencies
        )
target_include_directories(func_hello PRIVATE ${WUKONG_INCLUDE_DIR})