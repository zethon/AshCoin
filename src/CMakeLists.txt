
set(SOURCE_FILES
    AshDb.cpp
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
    AshDb.h
    AshLogger.h
    Block.h
    Blockchain.h
    ChainDatabase.h
    core.h
    MinerApp.h
    PeerManager.h
    Settings.h
    sha256.h
    utils.h
)
if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "/NODEFAULTLIB:library")
endif()

if(APPLE)
# utils.h/.cpp needs these two libraries if curl
# is not included
find_library(COREFOUNDATION_LIBRARY CoreFoundation)
find_library(CARBON_LIBRARY Carbon)
endif()


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
