file(GLOB INSTANCE_STORAGE_FUNC_SOURCES_FILE *.cpp *.cc *.h)

add_executable(instance-storage-func ${INSTANCE_STORAGE_FUNC_SOURCES_FILE})
target_link_libraries(instance-storage-func
        wukong::common_dependencies
        wukong::utils
        wukong::proto
        )