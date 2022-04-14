file(GLOB INSTANCE_WORKER_FUNC_SOURCES_FILE *.cpp *.cc *.h)

add_executable(instance-worker-func ${INSTANCE_WORKER_FUNC_SOURCES_FILE})
target_link_libraries(instance-worker-func
        wukong::common_dependencies
        wukong::utils
        wukong::proto
        wukong::function::interface::python
        )