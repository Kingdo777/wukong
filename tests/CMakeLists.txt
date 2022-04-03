add_executable(tool-dl_test dl.cpp)
target_include_directories(tool-dl_test PUBLIC ${WUKONG_INCLUDE_DIR})
target_link_libraries(tool-dl_test utils dl)

add_executable(tool-test test_cpp.cpp)
target_link_libraries(tool-test pthread)


add_executable(tool-process subProcess.cpp)
target_link_libraries(tool-process common_dependencies pthread wukong::utils)
target_include_directories(tool-process PUBLIC ${WUKONG_INCLUDE_DIR})

add_subdirectory(cgroup)