
set(SOURCE_FILES
    Block.cpp
    BlockChain.cpp
    main.cpp
    Settings.cpp
    sha256.cpp
    utils.cpp
)

set(HEADER_FILES
    Block.h
    BlockChain.h
    Settings.h
    sha256.h
    utils.h
)

add_executable(ash
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

target_link_libraries(ash
    PUBLIC
        ${CONAN_LIBS}
)
