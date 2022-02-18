add_executable(gateway gateway.cpp)
target_link_libraries(gateway PRIVATE
        wukong::endpoint
        )