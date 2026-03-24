cmake_minimum_required(VERSION 3.31)

project(CSP VERSION 1.0
    DESCRIPTION "Connected Spaces Platform"
    LANGUAGES CXX)

# ------------------------------------------------------------------------------------------------------------------
# First populate variables with our files
set(CSP_ROOT        ${CMAKE_CURRENT_SOURCE_DIR}/Library)
set(CSP_INCLUDE_DIR ${CSP_ROOT}/include)
set(CSP_SOURCE_DIR  ${CSP_ROOT}/src)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/shared_files.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/core_files.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/multiplayer_files.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/misc_files.cmake)

set(CSP_PUBLIC_SOURCES
  ${CSP_SHARED_PUBLIC_INCLUDES}
  ${CSP_CORE_PUBLIC_INCLUDES}
  ${CSP_MISC_PUBLIC_INCLUDES}
  ${CSP_MULTIPLAYER_PUBLIC_INCLUDES}
)

set(CSP_PRIVATE_SOURCES
  ${CSP_SHARED_SOURCES}
  ${CSP_SHARED_PRIVATE_INCLUDES}
  ${CSP_CORE_SOURCES}
  ${CSP_CORE_PRIVATE_INCLUDES}
  ${CSP_MULTIPLAYER_SOURCES}
  ${CSP_MULTIPLAYER_PRIVATE_INCLUDES}
  ${CSP_MISC_SOURCES}
  ${CSP_MISC_PRIVATE_INCLUDES}
)

# Preserve folder structure for files in IDE
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
  ${CSP_PUBLIC_SOURCES}
  ${CSP_PRIVATE_SOURCES}
)

# ------------------------------------------------------------------------------------------------------------------
# Find dependencies
find_package(RapidJSON REQUIRED)
find_package(fmt REQUIRED)
find_package(Async++ REQUIRED)
find_package(Poco REQUIRED)
find_package(tinyspline REQUIRED)
find_package(glm REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/signalrclient)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/quickjs)

include(FetchContent)

# Fetch csp serivces
FetchContent_Declare(
  csp-services
  GIT_REPOSITORY https://github.com/magnopus-opensource/csp-services.git
  GIT_TAG        v2.0-d20260311.r94e4b446b.Release
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)

FetchContent_MakeAvailable(csp-services)

# We need to include the services files in the csp project instead of making a seperate project.
# This is due to both csp-lib and csp-services depending on each other.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/services_files.cmake)

# Preserve folder structure for files in IDE
source_group(TREE ${CSP_SERVICES_DIR} FILES
  ${CSP_SERVICES_SOURCES}
)

# ------------------------------------------------------------------------------------------------------------------
# Create an interface that exposes internal usage requirements to internal targets.
# This way our internal tests can still access them without us exposing them to clients.
add_library(csp-lib-internal INTERFACE)

# Define all of our private include dirs for our interface lib.
target_include_directories(csp-lib-internal INTERFACE
  ${CSP_SOURCE_DIR}
  ${CSP_SERVICES_DIR}
)

# Link dependencies
target_link_libraries(csp-lib-internal
  INTERFACE 
    rapidjson
    fmt::fmt
    Async++
    Poco::Poco
    Poco::Net
    csp-signalr
    tinysplinecxx::tinysplinecxx
    glm::glm
    csp-quickjs
)

# ------------------------------------------------------------------------------------------------------------------
# Now create our final target
add_library(csp-lib)

target_sources(csp-lib
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CSP_INCLUDE_DIR}
    FILES 
      ${CSP_PUBLIC_SOURCES}
  PRIVATE
    ${CSP_PRIVATE_SOURCES}
    ${CSP_SERVICES_SOURCES}
)

# Set include directories
target_include_directories(csp-lib
  PUBLIC
    # Include dir when building from source
    $<BUILD_INTERFACE:${CSP_INCLUDE_DIR}>
    # Include dir after installation 
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CSP_SOURCE_DIR}
)

# The generated services should be included as a system header for now,
# as these create warnings which manifest as errors due to /wd4100.
target_include_directories(csp-lib SYSTEM PRIVATE
  ${CSP_SERVICES_DIR}
)

# Link dependencies
target_link_libraries(csp-lib
  PRIVATE 
    csp-lib-internal
)

# Project uses C++17
target_compile_features(csp-lib PUBLIC cxx_std_17)

target_compile_definitions(csp-lib
  PRIVATE
    # We consume libasync statically
    LIBASYNC_STATIC
    # We have our own default scheduler to remove the need to additional params
    LIBASYNC_CUSTOM_DEFAULT_SCHEDULER
    # Remove codecvt deprecation warnings for msvc
    <$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING>
    # Disable min/max macros on Windows
    $<$<PLATFORM_ID:Windows>:NOMINMAX>
    # Disable unused POCO features
    $<$<NOT:$<CXX_COMPILER_ID:Emscripten>>:
      POCO_NO_INOTIFY
      POCO_NO_FILECHANNEL
      POCO_NO_SPLITTERCHANNEL
      POCO_NO_SYSLOGCHANNEL
      POCO_UTIL_NO_INIFILECONFIGURATION
      POCO_UTIL_NO_JSONCONFIGURATION
      POCO_UTIL_NO_XMLCONFIGURATION
    >
  )

# Warnings as errors flag
# Sets WX/Werror
set_target_properties(csp-lib PROPERTIES COMPILE_WARNING_AS_ERROR ON)

target_compile_options(csp-lib
  PRIVATE
    # Needed to compile EntityScriptBindings.cpp
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
    # strict standard compliance
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->

    # Set warning level and exceptions for msvc
    $<$<CXX_COMPILER_ID:MSVC>:
      /W4       # Sets warning level 4 (high)
      /utf-8    # fmt asked us for this, seems like we need to explicitly opt into unicode on windows
      /wd4251   # Allow stl members to be exported
      /wd4996   # Function maybe unsafe warnings
      /wd4200   # nonstandard extension: zero-sized array
    >

)

# ------------------------------------------------------------------------------------------------------------------
# Testing
include(CTest)

if(BUILD_TESTING)
  
  find_package(GTest CONFIG REQUIRED)
  find_package(sole REQUIRED)

  # Fetch tiny-process lib
  FetchContent_Declare(
    tiny-process
    GIT_REPOSITORY https://gitlab.com/eidheim/tiny-process-library.git
    GIT_TAG        8bbb5a211c5c9df8ee69301da9d22fb977b27dc1
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
  )

  FetchContent_MakeAvailable(tiny-process)

  add_executable(csp-tests)

  include(${CMAKE_CURRENT_SOURCE_DIR}/Tests/cmake/test_files.cmake)

  # Preserve folder structure for files in IDE
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
    ${CSP_TESTS_SOURCES}
  )

  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/MultiplayerTestRunner)

  target_link_libraries(csp-tests
    PRIVATE 
    csp-lib
    csp-lib-internal
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    sole::sole
    tiny-process-library::tiny-process-library
  )

  # Set include directories
  target_include_directories(csp-tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/Tests/src
      ${CMAKE_CURRENT_SOURCE_DIR}/MultiplayerTestRunner/include
  )

  target_compile_features(csp-tests PRIVATE cxx_std_17)

  target_compile_definitions(csp-tests
    PRIVATE
        CSP_TESTS
        # Create a macro for our test runner output path to run the process.
        # This ensures we have the correct path on all configurations
        # and makes it simpler than copying the executable like we were doing previously.
        MULTIPLAYER_TEST_RUNNER_PATH="$<TARGET_FILE:MultiplayerTestRunner>"
  )

  target_compile_options(csp-tests PRIVATE
    # Needed to compile EntityScriptBindings.cpp
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
  )  

endif()
