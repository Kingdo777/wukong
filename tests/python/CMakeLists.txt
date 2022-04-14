add_executable(test-python python.cpp ../../src/instance/function/worker-function/WorkerFuncAgent.cpp)
target_link_libraries(test-python
        wukong::function::interface::python
        )
target_include_directories(test-python PUBLIC ${WUKONG_INCLUDE_DIR})