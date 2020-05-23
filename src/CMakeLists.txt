
set(SOURCE_FILES
    Block.cpp
    BlockChain.cpp
    main.cpp
    sha256.cpp
)

set(HEADER_FILES
    Block.h
    BlockChain.h
    sha256.h
)

add_executable(ash
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

target_link_libraries(ash
    PUBLIC
        ${CONAN_LIBS}
)
