# Temporary project to support existing CSP WebAssembly builds for JavaScript/TypeScript.

# The new binding repository separates JavaScript binding generation from the
# csp static library generation.

# Until the binding project is complete, this target creates a JavaScript
# "executable" that links against symbols from the csp-lib static library and
# produces the WebAssembly/JavaScript output.

# We still need to generate typescript bindings, as emscripten only produces .wasm and .js artifacts.
# Before running the CMake build step, generate the wrapper files with:
#
#   python3 Tools/WrapperGenerator/WrapperGenerator.py --generate_typescript
#
# Dummy.cpp is required to avoid the CMake configure-time error:
#
#   No SOURCES given to target

cmake_minimum_required(VERSION 3.31)

project(LegacyEmscriptenWrapper
    DESCRIPTION "LegacyEmscriptenWrapper" VERSION 1.0.0
    LANGUAGES C CXX)

add_executable(csp-emscripten ${CMAKE_CURRENT_SOURCE_DIR}/Dummy.cpp)

target_link_libraries(csp-emscripten PRIVATE
  "$<LINK_LIBRARY:WHOLE_ARCHIVE,csp-lib>"
)

option(WASM_WITH_NODE "Build wasm target with Node.js environment support" OFF)

if(EMSCRIPTEN)
    set_target_properties(csp-emscripten PROPERTIES
        OUTPUT_NAME "ConnectedSpacesPlatform_WASM"
        SUFFIX ".js"
    )

    target_compile_options(csp-emscripten PRIVATE
        --no-entry
        -pthread
        -fwasm-exceptions
        -fno-rtti
        -Wno-error=deprecated-declarations
        -Wno-braced-scalar-init
        -Wno-missing-field-initializers
        -Wno-ignored-qualifiers
        $<$<CONFIG:Debug>:-gdwarf-5>
        $<$<CONFIG:Debug>:-gseparate-dwarf>
        $<$<CONFIG:Release>:-Os>
        $<$<CONFIG:Release>:-flto>
    )

    target_link_options(csp-emscripten PRIVATE
        -pthread
        -sUSE_PTHREADS=1
        -fwasm-exceptions
        -sPTHREAD_POOL_SIZE_STRICT=0
        "SHELL:-sEXPORTED_FUNCTIONS=['_malloc','_free']"
        -sEXPORT_ES6=1
        -sMODULARIZE=1
        -sEXPORT_NAME=createModule
        -sFETCH
        -sALLOW_TABLE_GROWTH=1
        -sWASM_BIGINT
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=33554432
        -sMAXIMUM_MEMORY=1073741824
        "SHELL:-sEXPORTED_RUNTIME_METHODS=['ccall','setValue','getValue','addFunction','removeFunction','UTF8ToString','stringToUTF8','lengthBytesUTF8']"
        "SHELL:-sINCOMING_MODULE_JS_API=['buffer','fetchSettings','instantiateWasm','locateFile','mainScriptUrlOrBlob','wasmMemory']"
        --no-entry
        $<$<CONFIG:Debug>:-gdwarf-5>
        $<$<CONFIG:Debug>:-gseparate-dwarf>
        $<$<CONFIG:Debug>:-sSEPARATE_DWARF_URL=../Debug/ConnectedSpacesPlatform_WASM.wasm.debug.wasm>
        $<$<CONFIG:Release>:-Os>
        $<$<CONFIG:Release>:-flto>
    )
endif()