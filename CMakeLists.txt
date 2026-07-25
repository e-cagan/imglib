cmake_minimum_required(VERSION 3.16)
project(imglib CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(imglib_test src/main.cpp src/ppm.cpp)
target_include_directories(imglib_test PRIVATE include)