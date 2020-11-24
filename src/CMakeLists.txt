
set(SOURCE_FILES
    AshDb.cpp
    Block.cpp
    Blockchain.cpp
    ChainDatabase.cpp
    main.cpp
    MinerApp.cpp
    Settings.cpp
    sha256.cpp
    utils.cpp
)

set(HEADER_FILES
    AshDb.h
    Block.h
    Blockchain.h
    ChainDatabase.h
    core.h
    MinerApp.h
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
