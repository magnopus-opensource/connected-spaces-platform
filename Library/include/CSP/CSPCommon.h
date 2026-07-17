/*
 * Copyright 2026 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

// Enable this define if you are using the CSP library in DLL form
// #define USING_CSP_DLL

// Platform detection
#if defined(_WIN32) && !defined(CSP_WINDOWS)
    #define CSP_WINDOWS
#elif defined(__APPLE__) && !defined(CSP_MACOSX) && !defined(CSP_IOS)
    #include <TargetConditionals.h>

    #if TARGET_OS_IPHONE
        #define CSP_IOS
    #elif TARGET_OS_MAC
        #define CSP_MACOSX
    #endif
#elif defined(__EMSCRIPTEN__) && !defined(CSP_WASM)
    #define CSP_WASM
#elif defined(__ANDROID__) && !defined(CSP_ANDROID)
    #define CSP_ANDROID
#elif defined(__linux__) && !defined(CSP_LINUX)
    #define CSP_LINUX
#endif

#if defined(__has_include)
    #if __has_include(<unistd.h>)
        #define CSP_POSIX
    #endif
#endif

// DLL export/import macros
#if defined(CSP_WINDOWS)
    #define CSP_DLLEXPORT __declspec(dllexport)
    #define CSP_DLLIMPORT __declspec(dllimport)
#elif defined(__GNUC__) || defined(__clang__)
    #define CSP_DLLEXPORT __attribute__((visibility("default")))
    #define CSP_DLLIMPORT __attribute__((visibility("default")))
#else
    #define CSP_DLLEXPORT
    #define CSP_DLLIMPORT
#endif

// Warning pragmas
#if defined(_MSC_VER)
    #define PRAGMA_WARNING_PUSH() __pragma(warning(push))
    #define PRAGMA_WARNING_POP() __pragma(warning(pop))
    #define PRAGMA_WARNING_IGNORE_MSVC(WarningCode) __pragma(warning(disable : WarningCode))
    #define PRAGMA_WARNING_IGNORE_CLANG(WarningName)
    #define PRAGMA_WARNING_IGNORE_GCC(WarningName)
#elif defined(__clang__)
    #define CSP_DO_PRAGMA(X) _Pragma(#X)
    #define PRAGMA_WARNING_PUSH() _Pragma("clang diagnostic push")
    #define PRAGMA_WARNING_POP() _Pragma("clang diagnostic pop")
    #define PRAGMA_WARNING_IGNORE_MSVC(WarningCode)
    #define PRAGMA_WARNING_IGNORE_CLANG(WarningName) CSP_DO_PRAGMA(clang diagnostic ignored #WarningName)
    #define PRAGMA_WARNING_IGNORE_GCC(WarningName)
#elif defined(__GNUC__)
    #define CSP_DO_PRAGMA(X) _Pragma(#X)
    #define PRAGMA_WARNING_PUSH() _Pragma("GCC diagnostic push")
    #define PRAGMA_WARNING_POP() _Pragma("GCC diagnostic pop")
    #define PRAGMA_WARNING_IGNORE_MSVC(WarningCode)
    #define PRAGMA_WARNING_IGNORE_CLANG(WarningName)
    #define PRAGMA_WARNING_IGNORE_GCC(WarningName) CSP_DO_PRAGMA(GCC diagnostic ignored #WarningName)
#else
    #define PRAGMA_WARNING_PUSH()
    #define PRAGMA_WARNING_POP()
    #define PRAGMA_WARNING_IGNORE_MSVC(WarningCode)
    #define PRAGMA_WARNING_IGNORE_CLANG(WarningName)
    #define PRAGMA_WARNING_IGNORE_GCC(WarningName)
#endif

// Public API macros
#if defined(BUILD_CSP_DLL)
    #define CSP_API CSP_DLLEXPORT
    #define CSP_C_API CSP_DLLEXPORT
#elif defined(USING_CSP_DLL)
    #define CSP_API CSP_DLLIMPORT
    #define CSP_C_API
#elif defined(CSP_WASM)
    #define CSP_API
    // The EMSCRIPTEN_KEEPALIVE keyword is the way to export a function from WASM
    // in order to call it from the JS side.
    #define CSP_C_API EMSCRIPTEN_KEEPALIVE
#else
    #define CSP_API
    #define CSP_C_API
#endif

// Wrapper generator macros
#define CSP_NO_EXPORT
#define CSP_ASYNC_RESULT
#define CSP_ASYNC_RESULT_WITH_PROGRESS
#define CSP_EVENT
#define CSP_OUT
#define CSP_IN_OUT
#define CSP_START_IGNORE
#define CSP_END_IGNORE
#define CSP_INTERFACE
#define CSP_NO_DISPOSE
#define CSP_FLAGS

// TODO: Remove the following includes. 
// I don't think we should be default including these everywhere
#include <stdint.h>
#include <stdlib.h>

#ifdef CSP_WASM
#include <emscripten/emscripten.h>
#endif