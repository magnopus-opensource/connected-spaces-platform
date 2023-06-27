#! lua

include "../premake5_helpers.lua"


--Custom build options
newoption {
    trigger     = "generate_wasm",
    description = "Generate the project for building WebAssembly. This option should only be used with the gmake2 action"
}


workspace "WrapperGeneratorUnitTesting"
    configurations { "Debug" }

    CSP.UseStandardSettings()
    
    if CSP.IsWebAssemblyGeneration() then
        CSP.Platforms.AddWebAssembly()
    else
        CSP.Platforms.AddWindows()
    end

    local args = "--include_directory ../../UnitTesting/include/"

    if CSP.IsWebAssemblyGeneration() then
        args = args .. " --generate_typescript"
    else
        args = args .. " --generate_csharp"
    end

    os.execute("python ../Tools/WrapperGenerator/WrapperGenerator.py " .. args)


project "UnitTestingBinary"
    language "C++"
    characterset "ASCII"
    rtti "On"

    files {
        "%{prj.location}/include/**.h",
        "%{prj.location}/src/**.h",
        "%{prj.location}/src/**.cpp",
        "%{prj.location}/../Tools/WrapperGenerator/Output/C/generated_wrapper.h",
        "%{prj.location}/../Library/include/CSP/CSPCommon.h",
        "%{prj.location}/../Library/include/CSP/Common/Array.h",
        "%{prj.location}/../Library/include/CSP/Common/String.h",
        "%{prj.location}/../Library/src/Common/String.cpp",
        "%{prj.location}/../Library/src/Memory/DllAllocator.cpp",
        "%{prj.location}/../Library/src/Memory/Memory.h",
        "%{prj.location}/../Library/src/Memory/Memory.cpp",
        "%{prj.location}/../Library/src/Memory/MemoryManager.cpp"
    }

    externalincludedirs {
        "%{prj.location}/include",
        "%{prj.location}/src",
        "%{prj.location}/../Library/include",
        "%{prj.location}/../Library/src"
    }
    
    filter "platforms:x64"
        targetname "OlympusFoundation_D"
        kind "SharedLib"

        externalincludedirs {
            "%{prj.location}/../ThirdParty/mimalloc/include"
        }

        defines {
            "CSP_WINDOWS",
            "BUILD_CSP_DLL"
        }

        flags {
            "MultiProcessorCompile"
        }

        links {
            "mimalloc"
        }
    filter "platforms:wasm"
        targetname "OlympusFoundation_WASM.js"
        kind "None"

        defines {
            "USE_STD_MALLOC=1",
            "CSP_WASM"
        }

        buildoptions {
            "--no-entry",           -- remove default library entry point
            "-pthread",             -- enable threading
            "-fwasm-exceptions"     -- enable native wasm exceptions
        }

        linkoptions { 
            "-pthread",                                                     -- enable threading
            "-fwasm-exceptions",                                            -- enable native wasm exceptions
            "-sPTHREAD_POOL_SIZE_STRICT=0",                                 -- disable thread pool and spin up threads when we need them
            "-sEXPORT_ES6=1 -sMODULARIZE=1 -sEXPORT_NAME='createModule'",   -- export binary as an ES6 module
            "-sFETCH",                                                      -- enable Emscripten's Fetch API (needed for making REST calls to CHS)
            "-sALLOW_TABLE_GROWTH=1",                                       -- needed for registering callbacks that are passed to Foundation
            "-sWASM_BIGINT",                                                -- enable support for JavaScript's bigint (needed for 64-bit integer support)
            "-sENVIRONMENT='node,worker'",                                  -- only compile for node and worker (worker is required for multi-threading)
            "-sALLOW_MEMORY_GROWTH=1",                                      -- we don't know how much memory we'll need, so allow WASM to dynamically allocate more memory
            "-sINITIAL_MEMORY=33554432",
            "-sMAXIMUM_MEMORY=1073741824",                                  -- set an upper memory allocation bound to prevent Emscripten from trying to allocate too much memory
            "-sEXPORTED_RUNTIME_METHODS=[" ..
                "'ccall'," ..
                "'setValue'," ..
                "'getValue'," ..
                "'addFunction'," ..
                "'removeFunction'," ..
                "'UTF8ToString'," ..
                "'stringToUTF8'," ..
                "'lengthBytesUTF8'" ..
            "]",                                                            -- export needed Emscripten functions for wrapper
            "-sEXPORTED_FUNCTIONS=[" ..
                "'_malloc'" ..
            "]",                                                            -- ensure _malloc is not trimmed
            "-sINCOMING_MODULE_JS_API=[" ..
                "'buffer'," ..
                "'fetchSettings'," ..
                "'instantiateWasm'," ..
                "'locateFile'," ..
                "'mainScriptUrlOrBlob'," ..
                "'wasmMemory'" ..
            "]"
        }
    filter {}


project "mimalloc"
    filter "platforms:x64" 
        kind "StaticLib"

        language "C++"
        cppdialect "C++17"

        files {
            "%{prj.location}/../ThirdParty/mimalloc/include/**.h",
            "%{prj.location}/../ThirdParty/mimalloc/src/static.c"
        }

        externalincludedirs { 
            "%{prj.location}/../ThirdParty/mimalloc/include",
            "%{prj.location}/../ThirdParty/mimalloc/src",
        }
    filter "platforms:wasm"
        kind "None"
    filter{}