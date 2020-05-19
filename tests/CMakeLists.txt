project(tests)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

set(ASH_FILES
    ../src/sha256.cpp
    ../src/Block.cpp
    ../src/BlockChain.cpp
)

set(UTILS_FILES
    TestBlock.cpp
)

add_executable(TestBlock
    main.cpp
    ${ASH_FILES}
    ${UTILS_FILES}
)

target_link_libraries(TestBlock
    PUBLIC
    ${CONAN_LIBS}
)

add_test(NAME TestBlock
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/TestBlock
)
