file(GLOB FUNC_INTERFACE_PYTHON_SOURCES_FILE *.cpp)

add_library(function-interface-python STATIC ${FUNC_INTERFACE_PYTHON_SOURCES_FILE})
target_link_libraries(function-interface-python PUBLIC
        wukong::function::interface
        )
#target_include_directories(function-interface PRIVATE ${WUKONG_INCLUDE_DIR})
add_library(wukong::function::interface::python ALIAS function-interface-python)