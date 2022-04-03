add_executable(tool-cgroup CGroup.cpp)
target_link_libraries(tool-cgroup common_dependencies pthread wukong::utils)
target_include_directories(tool-cgroup PUBLIC ${WUKONG_INCLUDE_DIR})

add_executable(tool-cgroup-subprocess sub-process.cpp)
target_link_libraries(tool-cgroup-subprocess common_dependencies pthread wukong::utils)
target_include_directories(tool-cgroup-subprocess PUBLIC ${WUKONG_INCLUDE_DIR})