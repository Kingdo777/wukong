wukong_lib(endpoint)
target_link_libraries(endpoint PRIVATE
        wukong::utils
        wukong::proto
        )
add_dependencies(endpoint wukong::proto)