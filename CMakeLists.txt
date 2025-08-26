cmake_minimum_required(VERSION 3.26)

project(DualCube C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)


set(DLL_SOURCE_FILES 
    ./src/DualCube.c
    ./src/DualCube.h
)


include_directories(src)
include_directories(Corin)

add_link_options(-static)
add_compile_options(-O3)
add_compile_options(-mbmi2)
add_compile_options(-march=native)

add_library(DualCubeDLL SHARED  ${DLL_SOURCE_FILES})

aux_source_directory(./src SRC_LIST)
aux_source_directory(./Corin CORIN_SRC)

add_executable(DualCubeCLI main.c ${SRC_LIST})
target_link_libraries(DualCubeCLI iphlpapi.lib winhttp.lib pthread.lib ws2_32.lib)

add_executable(DualCubeGUI WIN32 winMain.c ${SRC_LIST} ${CORIN_SRC})
target_link_libraries(DualCubeGUI iphlpapi.lib winhttp.lib pthread.lib ws2_32.lib comctl32.lib)
