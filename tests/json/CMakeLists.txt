add_executable(test-json json.cpp)
target_link_libraries(test-json common_dependencies pthread wukong::utils)
target_include_directories(test-json PUBLIC ${WUKONG_INCLUDE_DIR})
