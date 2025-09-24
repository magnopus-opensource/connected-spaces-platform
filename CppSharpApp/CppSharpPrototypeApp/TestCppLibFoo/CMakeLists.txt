cmake_minimum_required(VERSION 3.20)

# Project definition
project(FooProject LANGUAGES CXX)

# Library target
add_library(Foo SHARED
    Foo.cpp
    Foo.h
)

# Use C++17 (CppSharp assumes a modern standard)
target_compile_features(Foo PUBLIC cxx_std_17)

# Define FOO_EXPORTS when building the DLL, so the macro expands to __declspec(dllexport)
target_compile_definitions(Foo PRIVATE FOO_EXPORTS)

# Public include directory (so consumers can #include "Foo.h")
target_include_directories(Foo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# MSVC-specific settings (important for ABI stability with STL)
if (MSVC)
    # Always build with /MD (multi-threaded DLL CRT), not /MT
    target_compile_options(Foo PRIVATE /EHsc /permissive-)
endif()