file(GLOB FUNCTION_POOL_SOURCES_FILE *.cpp *.h storage-function/*.cpp worker-function/*.cpp storage-function/*.h worker-function/*.h)

add_executable(instance-function-pool ${FUNCTION_POOL_SOURCES_FILE})
target_link_libraries(instance-function-pool
        wukong::endpoint
        wukong::utils
        wukong::proto
        wukong::common_dependencies
        function-interface-python
        function-interface
        )
add_dependencies(instance-function-pool wukong::proto)
