protobuf_generate_cpp(PB_SRC PB_HEADER wukong.proto)

file(GLOB PROTO_SOURCES_FILE *.cpp *.cc)

add_library(proto ${PB_SRC} ${PROTO_SOURCES_FILE})
target_include_directories(proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(proto PRIVATE ${WUKONG_INCLUDE_DIR})
target_link_libraries(proto PRIVATE wukong::utils)

add_library(wukong::proto ALIAS proto)