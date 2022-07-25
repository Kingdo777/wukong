add_executable(test-uuid main.cpp)
target_link_libraries(test-uuid common_dependencies pthread wukong::utils)
target_include_directories(test-uuid PUBLIC ${WUKONG_INCLUDE_DIR})
