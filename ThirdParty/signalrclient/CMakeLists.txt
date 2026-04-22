# A custom CMakeLists for the SignalR library. https://github.com/aspnet/SignalR-Client-Cpp
# The full intent for copying this library into our repo instead of using the repository as-is has been lost.
# However, we had to make changes to the library to get this working with csp. These changes can be found on Perforce.
# The SignaR library also has a complex build system, which uses vcpkg to fetch it's dependencies.
# This system adds the dependencies directly to the project, bypassing the need vcpkg.

# We also created a custom CSP fork here: https://github.com/magnopus/SignalR-Client-Cpp
# I assume this was supposed to contain these changes when moving away from Perforce.
# However, it looks like these changes were never added.
cmake_minimum_required(VERSION 3.31)

project(SignalR VERSION 1.0
    DESCRIPTION "CSP SignalR Client"
    LANGUAGES CXX)

find_package(msgpack-cxx REQUIRED)

add_library(csp-signalr STATIC)

set_target_properties(csp-signalr PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

file(GLOB_RECURSE SIGNALR_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

target_sources(csp-signalr PRIVATE
    ${SIGNALR_SOURCES}
)

target_include_directories(csp-signalr
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party_code/cpprestsdk
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party_code/jsoncpp
)

target_link_libraries(csp-signalr
  PRIVATE
    msgpack-cxx
)

target_compile_definitions(csp-signalr
  PUBLIC
    NO_SIGNALRCLIENT_EXPORTS
    USE_MSGPACK
)

target_compile_options(csp-signalr PRIVATE
  $<$<PLATFORM_ID:Emscripten>:-pthread;-fwasm-exceptions>
)

target_link_options(csp-signalr PUBLIC
  $<$<PLATFORM_ID:Emscripten>:
    -pthread
    -fwasm-exceptions
    -lwebsocket.js
  >
)