add_executable(test-shm-send send.cpp)
target_link_libraries(test-shm-send common_dependencies pthread wukong::utils)
target_include_directories(test-shm-send PUBLIC ${WUKONG_INCLUDE_DIR})

add_executable(test-shm-bounce bounce.cpp)
target_link_libraries(test-shm-bounce common_dependencies pthread wukong::utils)
target_include_directories(test-shm-bounce PUBLIC ${WUKONG_INCLUDE_DIR})
