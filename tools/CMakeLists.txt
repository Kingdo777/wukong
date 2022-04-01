
add_executable(tool-hello_server hello_server.cpp)
target_include_directories(tool-hello_server PUBLIC ${WUKONG_INCLUDE_DIR})
target_link_libraries(tool-hello_server PUBLIC
        common_dependencies
        wukong::endpoint
        wukong::client-server
        )

#add_executable(tool-dl_test dl.cpp)
#target_include_directories(tool-dl_test PUBLIC ${WUKONG_INCLUDE_DIR})
#target_link_libraries(tool-dl_test utils dl)
#
#add_executable(tool-test test_cpp.cpp)
#target_link_libraries(tool-test pthread)
#
#add_executable(tool-process subProcess.cpp)
#target_link_libraries(tool-process common_dependencies pthread wukong::utils)
