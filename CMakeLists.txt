project(Ash)
cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

# initialize conan libs
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(NO_OUTPUT_DIRS KEEP_RPATHS)

option(BUILD_ASH_TESTS "Build unit tests (default OFF)" OFF)

add_subdirectory(src)

if (BUILD_ASH_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif (BUILD_ASH_TESTS)