cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_ARCHITECTURES 61)
project(lbccs-capture LANGUAGES CUDA CXX)

find_package(CUDAToolkit REQUIRED VERSION 11.8)

add_executable(main main.cpp buffer_control.cu mem_helpers.cpp)
target_link_libraries(main d3d11.lib ${CUDAToolkit_LIBRARY_DIR}/cuda.lib dbgeng.lib )
message(STATUS ${CUDAToolkit_LIBRARY_DIR})
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libcmt.lib")