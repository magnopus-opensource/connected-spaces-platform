cmake_minimum_required(VERSION 3.31)

project(MultiplayerTestRunner LANGUAGES CXX)

add_executable(MultiplayerTestRunner)

target_sources(MultiplayerTestRunner
    PRIVATE
        src/CLIArgs.cpp
        src/CLIArgs.h
        src/LoginRAII.cpp
        src/LoginRAII.h
        src/main.cpp
        src/SpaceRAII.cpp
        src/SpaceRAII.h
        src/Utils.cpp
        src/Utils.h

        src/InternalUnitTests/Test_CLI.cpp
        src/InternalUnitTests/Test_LoginRAII.cpp
        src/InternalUnitTests/Test_RunnableTests.cpp
        src/InternalUnitTests/Test_SpaceRAII.cpp

        src/RunnableTests/CreateAvatar.cpp
        src/RunnableTests/CreateAvatar.h
        src/RunnableTests/CreateConversation.cpp
        src/RunnableTests/CreateConversation.h
        src/RunnableTests/EventBusPing.cpp
        src/RunnableTests/EventBusPing.h
        src/RunnableTests/LeaderElection.cpp
        src/RunnableTests/LeaderElection.h
        src/RunnableTests/LeaderElectionEvent.cpp
        src/RunnableTests/LeaderElectionEvent.h
        src/RunnableTests/LeaderElectionTick.cpp
        src/RunnableTests/LeaderElectionTick.h
)

target_include_directories(MultiplayerTestRunner
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty"
)

target_link_libraries(MultiplayerTestRunner
    PRIVATE
        csp-lib
        csp-lib-internal
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        sole::sole
)

if(MSVC)
    target_link_options(MultiplayerTestRunner PRIVATE /ignore:4099)

    set_property(TARGET MultiplayerTestRunner PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
endif()

target_compile_options(MultiplayerTestRunner PRIVATE
    # Needed to compile EntityScriptBindings.cpp
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
)

set_target_properties(MultiplayerTestRunner PROPERTIES
    OUTPUT_NAME MultiplayerTestRunner
)