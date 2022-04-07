add_executable(test-cgroup CGroup.cpp)
target_link_libraries(test-cgroup common_dependencies pthread wukong::utils)
target_include_directories(test-cgroup PUBLIC ${WUKONG_INCLUDE_DIR})

add_executable(test-cgroup-subprocess sub-process.cpp)
target_link_libraries(test-cgroup-subprocess common_dependencies pthread wukong::utils)
target_include_directories(test-cgroup-subprocess PUBLIC ${WUKONG_INCLUDE_DIR})