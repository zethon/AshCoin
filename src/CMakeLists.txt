
set(SOURCE_FILES
    AshLogger.cpp
    Block.cpp
    Blockchain.cpp
    ChainDatabase.cpp
    CryptoUtils.cpp
    main.cpp
    MinerApp.cpp
    PeerManager.cpp
    Settings.cpp
    Transactions.cpp
    utils.cpp
)

set(HEADER_FILES
    AshLogger.h
    Block.h
    Blockchain.h
    ChainDatabase.h
    ComputerID.h
    CryptoUtils.h
    core.h
    Miner.h
    MinerApp.h
    PeerManager.h
    ProblemDetails.h
    Settings.h
    Transactions.h
    utils.h
)

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "/NODEFAULTLIB:library")
    list(APPEND SOURCE_FILES
        ComputerIDWindows.cpp
    )
endif()

if(APPLE)
    # utils.h/.cpp needs these two libraries if curl
    # is not included
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(CARBON_LIBRARY Carbon)

    list(APPEND SOURCE_FILES
        ComputerIDMac.cpp
    )
endif()

if (UNIX AND NOT APPLE)
    list(APPEND SOURCE_FILES
        ComputerIDPosix.cpp
    )
endif()

z_compile_resources(RESOURCE_FILES
    html/address.html
    html/block.html
    html/createtx.html
    html/index.html
    html/tx.html
    html/style.css
    html/common.js
    html/header.html
    html/footer.html
)

add_executable(ash
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

target_link_libraries(ash
    PUBLIC
        simple-web-server
        ${CONAN_LIBS}
        ${COREFOUNDATION_LIBRARY}
        ${CARBON_LIBRARY}
)
