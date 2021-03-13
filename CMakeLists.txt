project(Ash)
cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)
include(ZCompileResource)

# initialize conan libs
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(NO_OUTPUT_DIRS KEEP_RPATHS)

option(BUILD_ASH_TESTS "Build unit tests (default OFF)" OFF)

# Add a RELEASE definition that we can count on regardless
# of platform
string(APPEND CMAKE_CXX_FLAGS_RELEASE " -D_RELEASE")
string(APPEND CMAKE_CXX_FLAGS_MINSIZEREL " -D_RELEASE")
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_DEBUG")
string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -D_DEBUG")

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

        /wd5105
        /wd5104
        /permissive-
        /Zc:preprocessor
        # /experimental:preprocessor
    )
endif(MSVC)

if (UNIX)
    if (CMAKE_COMPILER_IS_GNUCC)
        # https://stackoverflow.com/questions/55406770/gcc-undefined-references-with-abicxx11
        add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
    endif(CMAKE_COMPILER_IS_GNUCC)
endif()

add_subdirectory(src)

if (BUILD_ASH_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif (BUILD_ASH_TESTS)