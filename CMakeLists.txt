cmake_minimum_required(VERSION 3.29)
project(mapreduce)

set(CMAKE_CXX_STANDARD 20)
option(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc)
else()
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)
if(CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

get_filename_component(task_proto "protos/task.proto" ABSOLUTE)
get_filename_component(task_proto_path "${task_proto}" PATH)

# Generated sources
set(task_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/task.pb.cc")
set(task_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/task.pb.h")
set(task_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/task.grpc.pb.cc")
set(task_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/task.grpc.pb.h")
add_custom_command(
        OUTPUT "${task_proto_srcs}" "${task_proto_hdrs}" "${task_grpc_srcs}" "${task_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${task_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${task_proto}"
        DEPENDS "${task_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# hw_grpc_proto
add_library(task_grpc_proto
        ${task_grpc_srcs}
        ${task_grpc_hdrs}
        ${task_proto_srcs}
        ${task_proto_hdrs})
target_link_libraries(task_grpc_proto
        absl::check
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})

add_executable(worker worker.cpp)
target_link_libraries(worker task_grpc_proto absl::check
        absl::flags
        absl::flags_parse
        absl::log
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})

add_executable(coordinator coordinator.cpp)
target_link_libraries(coordinator task_grpc_proto absl::check
        absl::flags
        absl::flags_parse
        absl::log
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})

add_executable(client client.cpp)
target_link_libraries(client task_grpc_proto absl::check
        absl::flags
        absl::flags_parse
        absl::log
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})