cmake_minimum_required(VERSION 3.16)
project(imglib CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Qt viewer (Stage 3)
set(CMAKE_AUTOMOC ON)              # for signal-slot
set(CMAKE_AUTORCC ON)              # for source files
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(imglib_test src/main.cpp src/ppm.cpp src/filters.cpp)
add_executable(unit_tests tests/tests.cpp src/ppm.cpp src/filters.cpp)
add_executable(viewer apps/viewer/main.cpp apps/viewer/viewer_window.cpp src/ppm.cpp src/filters.cpp)
target_link_libraries(viewer PRIVATE Qt5::Widgets)
target_include_directories(viewer PRIVATE include)
target_include_directories(unit_tests PRIVATE include tests)
target_include_directories(imglib_test PRIVATE include)