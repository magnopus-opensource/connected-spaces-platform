cmake_minimum_required(VERSION 3.15...4.2)

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

# Preserve folder structure in IDE
source_group(TREE ${CSP_ROOT} FILES
  ${CSP_SHARED_PUBLIC_INCLUDES}
  ${CSP_SHARED_SOURCES}
  ${CSP_SHARED_PRIVATE_INCLUDES}

  ${CSP_CORE_PUBLIC_INCLUDES}
  ${CSP_CORE_SOURCES}
  ${CSP_CORE_PRIVATE_INCLUDES}

  ${CSP_MULTIPLAYER_PUBLIC_INCLUDES}
  ${CSP_MULTIPLAYER_SOURCES}
  ${CSP_MULTIPLAYER_PRIVATE_INCLUDES}
)

# Generate export header that defines the TEST_LIB_EXPORT macro for exporting in shared library builds
#include(GenerateExportHeader)

#generate_export_header(test-lib
#  EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/TestLibExport.h
#)

# Disable export macro for static builds by defining TEST_LIB_STATIC_DEFINE
# This is handled inside the generated TestLibExport.h
#target_compile_definitions(test-lib PUBLIC
#  $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:TEST_LIB_STATIC_DEFINE>
#)

# Set include directories
target_include_directories(csp-lib PUBLIC
  PUBLIC
    # Include dir when building from source
    $<BUILD_INTERFACE:${CSP_INCLUDE_DIR}>
    # Include binary dir for TestLibExport.h
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    # Include dir after installation 
    $<INSTALL_INTERFACE:include>
  PRIVATE
     ${CSP_SOURCE_DIR}
)

# Project uses C++17
target_compile_features(csp-lib PUBLIC cxx_std_17)

include(CTest)

if (BUILD_TESTING)
  # Create test executable
  #find_package(GTest CONFIG REQUIRED)

  set(CSP_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Tests)

  add_executable(tests ${CSP_TEST_DIR}/src/Main.cpp)

#  target_link_libraries(csp-lib-tests
#    PRIVATE csp-lib
#    GTest::gtest
#    GTest::gtest_main
#  )

endif ()