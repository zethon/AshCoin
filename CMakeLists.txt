cmake_minimum_required(VERSION 3.0)
project(Ash)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MACOSX_RPATH 1)

# initialize conan libs
include(CMake/conan.cmake)
conan_cmake_run(REQUIRES
    boost/1.77.0
    nlohmann_json/3.10.5
    spdlog/1.8.5
    cryptopp/8.6.0
    range-v3/0.12.0
    leveldb/1.23
    BASIC_SETUP CMAKE_TARGETS
    GENERATORS cmake_find_package cmake_paths
    BUILD missing
    OPTIONS 
        boost:shared=False 
        boost:without_test=False 
        boost:without_program_options=False 
        boost:without_filesystem=False 
        boost:without_system=False 
        boost:without_exception=False
        boost:without_contract=False
)
include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(NO_OUTPUT_DIRS KEEP_RPATHS)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)
include(ZCompileResource)

option(BUILD_ASH_TESTS "Build unit tests (default OFF)" OFF)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# Add a RELEASE definition that we can count on regardless
# of platform
string(APPEND CMAKE_CXX_FLAGS_RELEASE " -D_RELEASE")
string(APPEND CMAKE_CXX_FLAGS_MINSIZEREL " -D_RELEASE")
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_DEBUG")
string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -D_DEBUG")

# global definitions
# add_definitions(-DBOOST_ALLOW_DEPRECATED_HEADERS)
add_compile_definitions(
    BOOST_ALLOW_DEPRECATED_HEADERS
    BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
    # BOOST_SPIRIT_THREADSAFE=1
    # BOOST_SPIRIT_UNICODE
    # BOOST_ASIO_SEPARATE_COMPILATION
    BOOST_SYSTEM_NO_DEPRECATED
    # AMQP_STATIC
    FMT_HEADER_ONLY
)

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

add_subdirectory(Simple-Web-Server)
include_directories(Simple-Web-Server)

add_subdirectory(src)

if (BUILD_ASH_TESTS)
    enable_testing()
    add_subdirectory(tests)
    configure_file(test-config.h.in test-config.h)
endif (BUILD_ASH_TESTS)