file(GLOB INSTANCE_STORAGE_FUNC_SOURCES_FILE *.cpp *.cc *.h)

add_executable(instance-storage-func ${INSTANCE_STORAGE_FUNC_SOURCES_FILE})
target_link_libraries(instance-storage-func
        wukong::instance::common
        )
add_dependencies(instance-storage-func wukong::proto wukong::instance::common)