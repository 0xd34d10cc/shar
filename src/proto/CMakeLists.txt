protobuf_generate_cpp(PROTO_SRC PROTO_HEADER protocol.proto)
add_library(proto ${PROTO_SRC} ${PROTO_HEADER})

target_include_directories(proto
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${PROTOBUF_INCLUDE_DIR}
)
target_link_libraries(proto PUBLIC ${CONAN_LIBS_PROTOBUF})