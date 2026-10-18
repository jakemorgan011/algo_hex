cmake_minimum_required(VERSION 3.16)
project(algo_hex CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

add_executable(algo_hex src/m.cpp src/a.cpp)
target_compile_options(algo_hex PRIVATE -Wall -Wextra)
target_link_libraries(algo_hex PRIVATE Threads::Threads)
