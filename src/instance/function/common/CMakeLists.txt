file(GLOB INSTANCE_COMMON_SOURCES_FILE *.cpp *.cc *.h)

add_library(instance-common ${INSTANCE_COMMON_SOURCES_FILE})
target_link_libraries(instance-common
        wukong::common_dependencies
        wukong::utils
        wukong::proto
        )
target_include_directories(instance-common PUBLIC .)
add_dependencies(instance-common wukong::utils wukong::proto)
add_library(wukong::instance::common ALIAS instance-common)