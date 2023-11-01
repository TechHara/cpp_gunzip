cmake_minimum_required(VERSION 3.10)
project(cpp_gunzip)

find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_executable(gunzip gunzip.cc Crc32.cc)
target_link_libraries(gunzip ${CMAKE_THREAD_LIBS_INIT})
