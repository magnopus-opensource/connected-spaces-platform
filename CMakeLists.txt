cmake_minimum_required(VERSION 3.31)

project(CSP VERSION 1.0
    DESCRIPTION "Connected Spaces Platform"
    LANGUAGES CXX)

add_library(csp-lib)

set(CSP_ROOT        ${CMAKE_CURRENT_SOURCE_DIR}/Library)
set(CSP_INCLUDE_DIR ${CSP_ROOT}/include)
set(CSP_SOURCE_DIR  ${CSP_ROOT}/src)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/shared_files.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/core_files.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/multiplayer_files.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/misc_files.cmake)

# Preserve folder structure for files in IDE
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
  ${CSP_SHARED_PUBLIC_INCLUDES}
  ${CSP_SHARED_SOURCES}
  ${CSP_SHARED_PRIVATE_INCLUDES}

  ${CSP_CORE_PUBLIC_INCLUDES}
  ${CSP_CORE_SOURCES}
  ${CSP_CORE_PRIVATE_INCLUDES}

  ${CSP_MULTIPLAYER_PUBLIC_INCLUDES}
  ${CSP_MULTIPLAYER_SOURCES}
  ${CSP_MULTIPLAYER_PRIVATE_INCLUDES}

  ${CSP_MISC_PUBLIC_INCLUDES}
  ${CSP_MISC_SOURCES}
  ${CSP_MISC_PRIVATE_INCLUDES}
)

# Find dependencies
find_package(RapidJSON REQUIRED)
find_package(fmt REQUIRED)
find_package(Async++ REQUIRED)
find_package(Poco REQUIRED)
find_package(GTest CONFIG REQUIRED)
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
# You will need to update this file if the services adds new files.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/services_files.cmake)

# Preserve folder structure for files in IDE
source_group(TREE ${CSP_SERVICES_DIR} FILES
  ${CSP_SERVICES_SOURCES}
)

# Set include directories
target_include_directories(csp-lib PUBLIC
  PUBLIC
    # Include dir when building from source
    $<BUILD_INTERFACE:${CSP_INCLUDE_DIR}>
    # Include dir after installation 
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CSP_SOURCE_DIR}
    ${CSP_SERVICES_DIR}
)

# Link dependencies
target_link_libraries(csp-lib
  PRIVATE rapidjson
  PRIVATE fmt::fmt
  PRIVATE Async++
  PRIVATE Poco::Poco
  PRIVATE Poco::Net
  PRIVATE csp-signalr
  PRIVATE tinyspline::tinyspline
  PRIVATE glm::glm
  PRIVATE csp-quickjs
)

# Project uses C++17
target_compile_features(csp-lib PUBLIC cxx_std_17)

target_compile_options(csp-lib PRIVATE
    # Needed to compile EntityScriptBindings.cpp
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)