
set(SOURCE_FILES
    AshLogger.cpp
    Block.cpp
    Blockchain.cpp
    ChainDatabase.cpp
    main.cpp
    MinerApp.cpp
    PeerManager.cpp
    Settings.cpp
    sha256.cpp
    utils.cpp
)

set(HEADER_FILES
    AshLogger.h
    Block.h
    Blockchain.h
    ChainDatabase.h
    ComputerUUID.h
    core.h
    Miner.h
    MinerApp.h
    PeerManager.h
    Settings.h
    sha256.h
    utils.h
)

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "/NODEFAULTLIB:library")
    list(APPEND SOURCE_FILES
        ComputerUUIDWindows.h
    )
endif()

if(APPLE)
    # utils.h/.cpp needs these two libraries if curl
    # is not included
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(CARBON_LIBRARY Carbon)
endif()

if (APPLE OR UNIX)
    list(APPEND SOURCE_FILES
        ComputerUUIDPosix.cpp
    )
endif()

z_compile_resources(RESOURCE_FILES
    html/index.html
)

add_executable(ash
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

target_link_libraries(ash
    PUBLIC
        ${CONAN_LIBS}
        ${COREFOUNDATION_LIBRARY}
        ${CARBON_LIBRARY}
)
