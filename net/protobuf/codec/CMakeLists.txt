add_library(protobuf_codec codec.cc) # codec.cc包含了dispatcher.h
# 看清每个库和可执行文件需要依赖的其他库
target_link_libraries(protobuf_codec protobuf web_server_net z)

set_source_files_properties(addr_book.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")
include_directories(${PROJECT_BINARY_DIR})

add_library(addr_book_proto ./addr_book.pb.cc)
target_link_libraries(addr_book_proto protobuf pthread)


add_executable(protobuf_server ./addr_book_server.cc)
set_target_properties(protobuf_server PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(protobuf_server protobuf_codec addr_book_proto)

add_executable(protobuf_client ./addr_book_client.cc)
set_target_properties(protobuf_client PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(protobuf_client protobuf_codec addr_book_proto)
