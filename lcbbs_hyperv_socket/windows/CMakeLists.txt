cmake_minimum_required(VERSION 3.10)

project(af_hyperv_server)

add_executable(af_hyperv_server af_hyperv_server.cpp)

target_link_libraries(af_hyperv_server wsock32 ws2_32)