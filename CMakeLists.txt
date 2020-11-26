project(Ash)
cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

# initialize conan libs
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(NO_OUTPUT_DIRS KEEP_RPATHS)

option(BUILD_ASH_TESTS "Build unit tests (default OFF)" OFF)

# Add a RELEASE definition that we can count on regardless
# of platform
string(APPEND CMAKE_CXX_FLAGS_RELEASE " -D_RELEASE")
string(APPEND CMAKE_CXX_FLAGS_MINSIZEREL " -D_RELEASE")

if (MSVC)
    add_definitions(
        -DWIN32
        -D_WIN32
        -D_WINDOWS
        -DNOMINMAX
        -D_SCL_SECURE_NO_WARNINGS
        -D_WIN32_WINNT=0x0600
        -DWIN32_LEAN_AND_MEAN

        # use this to supress the boost generated "warning C4996"
        # on Windows
        -D_SILENCE_FPOS_SEEKPOS_DEPRECATION_WARNING
    )

    add_compile_options(
        # treat warnings as errors
        /WX
        /wd4996
        
        # ignore macro redefinition warnings because of PDCurses and MOUSE_MOVED
        /wd4005 
    )
endif(MSVC)

add_subdirectory(src)

if (BUILD_ASH_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif (BUILD_ASH_TESTS)