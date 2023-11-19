cmake_minimum_required(VERSION 3.10)
project(cpp_gunzip)

find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 17)

option(USE_FAST_CRC32 "Use fast crc32 library instead of libz for computing crc32 checksum" ON)

if(USE_FAST_CRC32)
    add_compile_definitions(USE_FAST_CRC32)
    add_executable(gunzip gunzip.cc Crc32.cc)
    target_link_libraries(gunzip ${CMAKE_THREAD_LIBS_INIT})
else()
    find_package(ZLIB REQUIRED)
    add_executable(gunzip gunzip.cc)
    target_link_libraries(gunzip ${CMAKE_THREAD_LIBS_INIT} ZLIB::ZLIB)
endif()