cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(lbccs-capture LANGUAGES CXX)


add_executable(main main.cpp mem_helpers.cpp)
target_link_libraries(main d3d11.lib dbgeng.lib windowsapp.lib )
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libcmt.lib")