# A custom CMakeLists for the QuickJS library.
# The full intent for copying this library into our repo instead of using the repository as-is has been lost.
# However, QuickJS can not be built on Windows-msvc using it's own build system, which is likely a large reason why it was done this way.
# Secondly, custom CSP logging code has been added to the library, along with a custom filesystem implementation for CSP_ANDROID.
# We also can't be certain this came from the main repo here: https://github.com/bellard/quickjs, or which commit it was branched from.

cmake_minimum_required(VERSION 3.31)

project(QuickJS VERSION 1.0
    DESCRIPTION "CSP QuickJS"
    LANGUAGES C)

add_library(csp-quickjs STATIC)

target_compile_features(csp-quickjs PUBLIC cxx_std_17)

file(GLOB_RECURSE QUICKJS_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

target_sources(csp-quickjs PRIVATE
    ${QUICKJS_SOURCES}
)

target_include_directories(csp-quickjs
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(csp-quickjs
  PRIVATE
    _HAS_EXCEPTIONS=0
  PUBLIC
    JS_STRICT_NAN_BOXING
)

target_compile_options(csp-quickjs PRIVATE
  $<$<PLATFORM_ID:Emscripten>:-pthread;-fwasm-exceptions>
)

target_link_options(csp-quickjs PUBLIC
  $<$<PLATFORM_ID:Emscripten>:-pthread;-fwasm-exceptions>
)