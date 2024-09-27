cmake_minimum_required(VERSION 3.5)

project(TinyWebServer)

set(CMAKE_CXX_FLAGS "-std=c++2a -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "-o3 -DNDEBUG -s")

aux_source_directory(src SRC)

add_executable(server main.cpp ${SRC})

target_link_options(server PRIVATE "$<$<CONFIG:Debug>:-lasan>")