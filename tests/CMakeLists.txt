add_executable(test-dl_test dl.cpp)
target_include_directories(test-dl_test PUBLIC ${WUKONG_INCLUDE_DIR})
target_link_libraries(test-dl_test utils dl)

add_executable(test-test test_cpp.cpp)
target_link_libraries(test-test common_dependencies pthread wukong::utils)
target_link_libraries(test-test pthread)


add_executable(test-process subProcess.cpp)
target_link_libraries(test-process common_dependencies pthread wukong::utils)
target_include_directories(test-process PUBLIC ${WUKONG_INCLUDE_DIR})

add_subdirectory(cgroup)
add_subdirectory(local-gateway)
add_subdirectory(shm)
add_subdirectory(json)