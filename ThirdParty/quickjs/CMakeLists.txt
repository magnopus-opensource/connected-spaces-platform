# A custom CMakeLists for the QuickJS library.
# The full intent for copying this library into our repo instead of using the repository as-is has been lost.
# However, QuickJS can not be built on Windows-msvc using it's own build system, which is likely a large reason why it was done this way.
# Secondly, custom CSP logging code has been added to the library, along with a custom filesystem implementation for CSP_ANDROID.
# We also can't be certain this came from the main repo here: https://github.com/bellard/quickjs, or which commit it was branched from.

cmake_minimum_required(VERSION 3.31)

project(QuickJS VERSION 1.0
    DESCRIPTION "CSP QuickJS"
    LANGUAGES C)

include(GNUInstallDirs)

add_library(csp-quickjs STATIC)

target_compile_features(csp-quickjs PUBLIC cxx_std_17)

file(GLOB_RECURSE QUICKJS_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

file(GLOB_RECURSE QUICKJS_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

target_sources(csp-quickjs
  PRIVATE
    ${QUICKJS_SOURCES}
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
    FILES ${QUICKJS_PUBLIC_HEADERS}
)

target_include_directories(csp-quickjs
  SYSTEM PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# CSP and its internal static libraries may be linked into a shared library,
# so build library targets with PIC on non-Windows platforms.
if(NOT WIN32)
  set_target_properties(
    csp-quickjs
      PROPERTIES
        POSITION_INDEPENDENT_CODE ON
  )
endif()

target_compile_definitions(csp-quickjs
  PRIVATE
    _HAS_EXCEPTIONS=0
    # Needed for glibc to expose sighandler_t in <signal.h>
    $<$<COMPILE_LANG_AND_ID:C,GNU>:_GNU_SOURCE>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:_GNU_SOURCE>

    # This is a CSP macro that was added to our version of the QuickJS library
    # which prevents QuickJS from defining methods that do stack checking.
    # This macro only applies to non-Emscripten and non-MSVC builds, although we have only
    # observed this issue on Linux builds.
    # The stack checking was causing invalid stack overflow exceptions to be thrown.
    # Related to this error: https://forum.juce.com/t/stack-overflow-error-with-quickjs/67427/12
    $<$<CXX_COMPILER_ID:GNU>:CSP_DISABLE_STACK_CHECK_QJS>

  PUBLIC
    JS_STRICT_NAN_BOXING
)

target_compile_options(csp-quickjs PRIVATE
  $<$<PLATFORM_ID:Emscripten>:-pthread;-fwasm-exceptions>
)

target_link_options(csp-quickjs PUBLIC
  $<$<PLATFORM_ID:Emscripten>:-pthread;-fwasm-exceptions>
)

install(TARGETS csp-quickjs
  EXPORT CSPTargets
  FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)