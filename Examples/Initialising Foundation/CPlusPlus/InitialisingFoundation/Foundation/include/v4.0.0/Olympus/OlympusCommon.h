#pragma once

// Enable this define if you are using the Olympus library in DLL form
// #define USING_OLYMPUS_DLL

// Enable the define appropriate for the target platform
// #define OLY_WINDOWS

#if defined OLY_WINDOWS
#define OLY_DLLEXPORT __declspec(dllexport)
#define OLY_DLLIMPORT __declspec(dllimport)

#define PRAGMA_WARNING_PUSH() __pragma(warning(push))
#define PRAGMA_WARNING_IGNORE_MSVC(WarningCode) __pragma(warning(disable : WarningCode))
#define PRAGMA_WARNING_IGNORE_CLANG(WarningName)
#define PRAGMA_WARNING_POP() __pragma(warning(pop))
#elif defined OLY_MACOSX || defined OLY_IOS || defined OLY_ANDROID
#define OLY_DLLEXPORT __attribute__((visibility("default")))
#define OLY_DLLIMPORT __attribute__((visibility("default")))

#define DO_PRAGMA(X) _Pragma(#X) // DO_PRAGMA stringizing trick - https://gcc.gnu.org/onlinedocs/cpp/Pragmas.html
#define PRAGMA_WARNING_PUSH() _Pragma("clang diagnostic push")
#define PRAGMA_WARNING_IGNORE_MSVC(WarningCode)
#define PRAGMA_WARNING_IGNORE_CLANG(WarningName) DO_PRAGMA(clang diagnostic ignored #WarningName)
#define PRAGMA_WARNING_POP() _Pragma("clang diagnostic pop")
#endif

#ifdef BUILD_OLYMPUS_DLL
#define OLY_API OLY_DLLEXPORT
#define OLY_C_API OLY_DLLEXPORT
#elif defined USING_OLYMPUS_DLL
#define OLY_API OLY_DLLIMPORT
#define OLY_C_API
#elif defined OLY_WASM
#define OLY_API
// The EMSCRIPTEN_KEEPALIVE keyword is the way to export a function from WASM in order to call it from the JS side.
#define OLY_C_API EMSCRIPTEN_KEEPALIVE

#define PRAGMA_WARNING_PUSH()
#define PRAGMA_WARNING_IGNORE_MSVC(WarningCode)
#define PRAGMA_WARNING_IGNORE_CLANG(WarningName)
#define PRAGMA_WARNING_POP()
#else
#define OLY_API
#define OLY_C_API
#endif

// Wrapper generator macros
#define OLY_NO_EXPORT
#define OLY_ASYNC_RESULT
#define OLY_ASYNC_RESULT_WITH_PROGRESS
#define OLY_EVENT
#define OLY_OUT
#define OLY_IN_OUT
#define OLY_START_IGNORE
#define OLY_END_IGNORE
#define OLY_INTERFACE
#define OLY_NO_DISPOSE

#include <stdint.h>
#include <stdlib.h>

#ifdef OLY_WASM
#include <emscripten/emscripten.h>
#endif
