cmake_minimum_required(VERSION 3.31)

# Populate our version variables
include(cmake/get_version_from_git.cmake)
get_version_from_git()

set (CSP_VERSION_STRING "${CSP_VERSION}")

project(CSP
    DESCRIPTION "Connected Spaces Platform" VERSION ${CSP_VERSION_STRING}
    LANGUAGES CXX)

# ------------------------------------------------------------------------------------------------------------------
# First populate variables with our files
set(CSP_ROOT        ${CMAKE_CURRENT_SOURCE_DIR}/Library)
set(CSP_INCLUDE_DIR ${CSP_ROOT}/include)
set(CSP_SOURCE_DIR  ${CSP_ROOT}/src)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/common_files.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/core_files.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/multiplayer_files.cmake)

set(CSP_PUBLIC_SOURCES
  ${CSP_COMMON_PUBLIC_INCLUDES}
  ${CSP_CORE_PUBLIC_INCLUDES}
  ${CSP_MULTIPLAYER_PUBLIC_INCLUDES}
)

set(CSP_PRIVATE_SOURCES
  ${CSP_COMMON_SOURCES}
  ${CSP_COMMON_PRIVATE_INCLUDES}
  ${CSP_CORE_SOURCES}
  ${CSP_CORE_PRIVATE_INCLUDES}
  ${CSP_MULTIPLAYER_SOURCES}
  ${CSP_MULTIPLAYER_PRIVATE_INCLUDES}
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

# Vendored libs
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
# Create csp-lib

option(CSP_BUILD_SHARED "Build csp as a shared library" OFF)

if(CSP_BUILD_SHARED)
  set(CSP_LIB_TYPE SHARED)
else()
  set(CSP_LIB_TYPE STATIC)
endif()

add_library(csp-lib ${CSP_LIB_TYPE})

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
target_include_directories(csp-lib
  SYSTEM PRIVATE
    ${CSP_SERVICES_DIR}
)

target_link_libraries(csp-lib
  PRIVATE
    rapidjson
    fmt::fmt
    Async++
    Poco::Poco
    Poco::Net
    csp-signalr
    tinyspline::tinyspline
    glm::glm
    csp-quickjs
)

set(TINYSPLINE_BUILD_TESTS OFF)
set(TINYSPLINE_BUILD_EXAMPLES OFF)
set(TINYSPLINE_BUILD_DOCS OFF)

# Public as it is a requiremnt that consumers also be >= c++17
target_compile_features(csp-lib PUBLIC cxx_std_17)

target_compile_definitions(csp-lib
  PRIVATE
    # Setup dll export macros if we're building as shared
    $<$<BOOL:${CSP_BUILD_SHARED}>:BUILD_CSP_DLL>
    # Platform macros
    $<$<PLATFORM_ID:Windows>:CSP_WINDOWS>
    $<$<PLATFORM_ID:Darwin>:CSP_MACOSX>

    # We consume libasync statically
    LIBASYNC_STATIC
    # We have our own default scheduler to remove the need to additional params
    LIBASYNC_CUSTOM_DEFAULT_SCHEDULER

    # Remove codecvt deprecation warnings for msvc
    $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING>
    # Disable unused POCO features
    $<$<NOT:$<CXX_COMPILER_ID:Emscripten>>:POCO_NO_INOTIFY>
    $<$<NOT:$<CXX_COMPILER_ID:Emscripten>>:POCO_NO_FILECHANNEL>
    $<$<NOT:$<CXX_COMPILER_ID:Emscripten>>:POCO_NO_SPLITTERCHANNEL>
    $<$<NOT:$<CXX_COMPILER_ID:Emscripten>>:POCO_NO_SYSLOGCHANNEL>
    $<$<NOT:$<CXX_COMPILER_ID:Emscripten>>:POCO_UTIL_NO_INIFILECONFIGURATION>
    $<$<NOT:$<CXX_COMPILER_ID:Emscripten>>:POCO_UTIL_NO_JSONCONFIGURATION>
    $<$<NOT:$<CXX_COMPILER_ID:Emscripten>>:POCO_UTIL_NO_XMLCONFIGURATION>

    # Android + MacOS + iOS
    $<$<OR:$<PLATFORM_ID:Android>,$<PLATFORM_ID:Darwin>>:POCO_NO_WINDOWS_H>
    $<$<OR:$<PLATFORM_ID:Android>,$<PLATFORM_ID:Darwin>>:POCO_NO_FPENVIRONMENT>
    $<$<OR:$<PLATFORM_ID:Android>,$<PLATFORM_ID:Darwin>>:POCO_NO_WSTRING>
    $<$<OR:$<PLATFORM_ID:Android>,$<PLATFORM_ID:Darwin>>:POCO_NO_SHAREDMEMORY>
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

    $<$<CXX_COMPILER_ID:AppleClang,Clang>:
	    -Wextra
	    # --Don't error on deprecation warnings, this is because we use Uri a lot in our services generated code,
	    # which has deprecation warnings for some unused but still generated endpoints.
	    -Wno-error=deprecated-declarations
	    # Don't warn against doing stuff like `return {0}`, which we do in the interop output.
	    -Wno-braced-scalar-init
	    # Don't warn against missing field initializers, e.g. uninitialized fields in structs (because of the wrapper generator)
	    -Wno-missing-field-initializers
	    # Don't error on ignored qualifiers, such as returning const objects. These should be simple to remove at some point.
	    -Wno-ignored-qualifiers
    >
)

# Generate version header
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/version.h.in
    # When we remove support for the old build system, we should probably move this into the binary dir
    ${CSP_INCLUDE_DIR}/CSP/version.h
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

  target_sources(csp-tests PRIVATE ${CSP_TESTS_SOURCES})

  # Preserve folder structure for files in IDE
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
    ${CSP_TESTS_SOURCES}
  )

  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/MultiplayerTestRunner)

  target_link_libraries(csp-tests
    PRIVATE 
      csp-lib
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
      sole::sole
      tiny-process-library::tiny-process-library

      # We need to link a lot of our third party libs here, as well, 
      # becase we either test these directly, or expose them through private headers.
      # This way we don't have to make them public libs for regular consumers.

      # Our tests directly use these libs
      csp-quickjs
      rapidjson
      glm::glm
      fmt::fmt
      Async++
      csp-signalr

      # Libs exposed through private includes
      # We should find a way of hiding these from headers ideally
      Poco::Poco
      Poco::Net
  )

  # Set include directories
  target_include_directories(csp-tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/Tests/src
      ${CMAKE_CURRENT_SOURCE_DIR}/MultiplayerTestRunner/include
      # We need to access private includes in our tests
      ${CSP_SOURCE_DIR}
      ${CSP_SERVICES_DIR}
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
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
  )

  # Ensure csp and MultiplayerTestRunner are built first
  add_dependencies(csp-tests
    csp-lib
    MultiplayerTestRunner
  )

  # Copy out assets folder next to the test executable
  add_custom_command(TARGET csp-tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Tests/assets"
            "$<TARGET_FILE_DIR:csp-tests>/assets"
    COMMENT "Copying assets to output folder"
)

endif()
