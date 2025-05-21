cmake_minimum_required(VERSION 3.26)
project(DualCube C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


set(CMAKE_EXE_LINKER_FLAGS "-static")
add_compile_options(-O3)
# add_compile_options(-static)
# add_compile_options(-std=c11)


add_executable(DualCube main.c)

target_link_libraries(DualCube iphlpapi.lib winhttp.lib pthread.lib ws2_32.lib)
