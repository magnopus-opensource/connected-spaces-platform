cmake_minimum_required(VERSION 3.20)
project(CspSwigProto LANGUAGES CXX)

# ---- User-configurable paths (pass via -D or edit here) ----------------------
set(CSP_INCLUDE_DIR "C:/dev/connected-spaces-platform/Library/include" CACHE PATH "CSP public headers")
set(CSP_LIB_DIR     "C:/dev/connected-spaces-platform/Library/Binaries/x64/DebugDLL" CACHE PATH "Path to ConnectedSpacesPlatform_D.lib")
set(CSP_LIB_NAME    "ConnectedSpacesPlatform_D" CACHE STRING "Import lib name (no extension)")
                     
set(SWIG_EXECUTABLE "C:/ProgramData/chocolatey/bin/swig.exe" CACHE FILEPATH "swig.exe")
set(IFILE           "${CMAKE_SOURCE_DIR}/OfflineRealtimeEngine_CreateEntity.i" CACHE FILEPATH "SWIG .i file")

# Output dirs for generated code
set(GEN_CPP_DIR "${CMAKE_BINARY_DIR}/generated/cpp")
set(GEN_CS_DIR  "${CMAKE_BINARY_DIR}/generated/cs")

# SWIG module/DLL name
set(WRAPPER_NAME "CspSwigProto")
set(WRAP_CPP "${GEN_CPP_DIR}/${WRAPPER_NAME}_wrap.cxx")

# ---- Generate wrapper (C++ + C#) --------------------------------------------
find_program(SWIG_EXE swig HINTS "${SWIG_EXECUTABLE}" REQUIRED)

add_custom_command(
  OUTPUT "${WRAP_CPP}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${GEN_CPP_DIR}" "${GEN_CS_DIR}"
  COMMAND "${SWIG_EXE}"
          -c++ -csharp
          -module ${WRAPPER_NAME}
          -dllimport ${WRAPPER_NAME}
          -namespace Csp
          -outdir "${GEN_CS_DIR}"
          -o "${WRAP_CPP}"
          "${IFILE}"
  DEPENDS "${IFILE}"
  VERBATIM
)

# ---- Build the wrapper DLL ---------------------------------------------------
add_library(${WRAPPER_NAME} SHARED "${WRAP_CPP}")
set_target_properties(${WRAPPER_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
)

# MSVC runtime: /MDd for Debug, /MD for Release
if(MSVC)
    set_property(TARGET ${WRAPPER_NAME} PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")  # always /MD (CSP does this :( )
  target_compile_options(${WRAPPER_NAME} PRIVATE /EHsc /permissive- /Zc:__cplusplus)
endif()

target_include_directories(${WRAPPER_NAME} PRIVATE "${CSP_INCLUDE_DIR}")
set(CSP_BIN_ROOT "C:/dev/connected-spaces-platform/Library/Binaries/x64")
target_link_libraries(${WRAPPER_NAME} PRIVATE
  "$<$<CONFIG:Debug>:${CSP_BIN_ROOT}/DebugDLL/ConnectedSpacesPlatform_D.lib>"
  "$<$<CONFIG:Release>:${CSP_BIN_ROOT}/ReleaseDLL/ConnectedSpacesPlatform.lib>"
)

file(GLOB _csp_libs "${CSP_LIB_DIR}/*.lib")
message(STATUS "Found libs:\n  ${_csp_libs}")

# Optionally copy generated C# to an easy-to-find spot
add_custom_command(TARGET ${WRAPPER_NAME} POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E echo "C# files generated at: ${GEN_CS_DIR}"
)

message(STATUS "SWIG C++ out: ${WRAP_CPP}")
message(STATUS "SWIG C# out : ${GEN_CS_DIR}")