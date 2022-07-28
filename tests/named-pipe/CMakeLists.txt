add_executable(test-named-pipe main.cpp)
target_link_libraries(test-named-pipe common_dependencies pthread wukong::utils)
target_include_directories(test-named-pipe PUBLIC ${WUKONG_INCLUDE_DIR})
