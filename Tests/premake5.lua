#!lua

include "Library/premake5.lua"


if not Tests then
    Tests = {}
        
    function Tests.AddProject()
        project "Tests"
        location "Tests"

        kind "ConsoleApp"
        
        removeplatforms  { "ios", "macosx", "Android" }

        files {
            "%{prj.location}/src/**.h",
            "%{prj.location}/src/**.cpp",
            "%{prj.location}/src/**.hpp",
            "%{prj.location}/assets/**.*"
        }
        
        externalincludedirs { 
            "%{prj.location}/src",
            "%{wks.location}/ThirdParty/googletest/include",
            "%{wks.location}/ThirdParty/googlemock/include",
            "%{wks.location}/ThirdParty/uuid-v4",
            "%{wks.location}/ThirdParty/tiny-process-library/install/include",
            "%{wks.location}/MultiplayerTestRunner/include"
        }   
        
        debugdir "%{prj.location}\\Binaries\\%{cfg.platform}\\%{cfg.buildcfg}"
        
        -- The tests project is configured for the most part like the Connected Spaces Platform project itself.
        -- This allows us to test both internal functionality as well as the public API.
        Project.DefineProject()
        
        -- Set tests executable name
        filter "platforms:x64"
            targetname( "Tests" )
        filter "platforms:wasm"
            targetname( "Tests_WASM.js" )
        filter {}

        -- Tell Connected Spaces Platform we're compiling tests
        defines { "CSP_TESTS" }

        -- Compile support for MessagePack
        defines { "USE_MSGPACK" }
		
		-- We're building LibAsync statically and need this
		defines { "LIBASYNC_STATIC" }
       
        -- If we're running on a build agent, we want to run *all* the tests
        if CSP.IsRunningOnTeamCityAgent() and not CSP.IsWebAssemblyGeneration() then
            result = iif(CSP.IsRunningNightlyBuild(), "RUN_NIGHTLY_TESTS", "RUN_ALL_UNIT_TESTS")
            defines { result }
        end
        
        -- Config for platforms
        filter "platforms:x64"
            defines { "CSP_WINDOWS" }
            linkoptions { "/ignore:4099"} -- Because we don't have debug symbols for OpenSSL libs
        filter "platforms:wasm"
            rtti("Off")

            defines {
                "CSP_WASM",
                "USE_STD_MALLOC=1",
                "SKIP_INTERNAL_TESTS",
                "RUN_PLATFORM_TESTS"    -- enable platform tests by default for wasm
            }

            buildoptions {
                "-pthread",             -- enable threading
                "-fwasm-exceptions"     -- enable native wasm exceptions
            }

            linkoptions { 
                "--emrun",                                                      -- make tests emrun compatible
                "--embed-file ./assets/test_account_creds.txt@/",               -- give account credentials to emscripten
                "-pthread",                                                     -- enable threading
                "-fwasm-exceptions",                                            -- enable native wasm exceptions
                "-sPTHREAD_POOL_SIZE_STRICT=0",                                 -- disable thread pool and spin up threads when we need them
                "-sEXPORTED_FUNCTIONS=[" ..                            
                    "'_malloc'," ..
                    "'_main'" ..                                                -- force export _malloc function
                "]",                             
                "-sEXPORT_ES6=1 -sMODULARIZE=1 -sEXPORT_NAME='createModule'",   -- export binary as an ES6 module
                "-sFETCH",                                                      -- enable Emscripten's Fetch API (needed for making REST calls to CHS)
                "-sALLOW_TABLE_GROWTH=1",                                       -- needed for registering callbacks that are passed to Foundation
                "-sWASM_BIGINT",                                                -- enable support for JavaScript's bigint (needed for 64-bit integer support)
                "-sENVIRONMENT='web,worker'",                                   -- only compile for web and worker (worker is required for multi-threading)
                "-sALLOW_MEMORY_GROWTH=1",                                      -- we don't know how much memory we'll need, so allow WASM to dynamically allocate more memory
                "-sINITIAL_MEMORY=33554432",
                "-sMAXIMUM_MEMORY=1073741824",                                  -- set an upper memory allocation bound to prevent Emscripten from trying to allocate too much memory   
                "-sPROXY_TO_PTHREAD",
                "-sSTACK_SIZE=5MB",
                "-sEXIT_RUNTIME",
                "-sEXPORTED_RUNTIME_METHODS=[" ..
                    "'ccall'," ..
                    "'setValue'," ..
                    "'getValue'," ..
                    "'addFunction'," ..
                    "'removeFunction'," ..
                    "'UTF8ToString'," ..
                    "'stringToUTF8'," ..
                    "'lengthBytesUTF8'," ..
                    "'callMain'" .. 
                "]",                                                            -- export needed Emscripten functions for wrapper
                "-sINCOMING_MODULE_JS_API=[" ..
                    "'buffer'," ..
                    "'fetchSettings'," ..
                    "'instantiateWasm'," ..
                    "'locateFile'," ..
                    "'mainScriptUrlOrBlob'," ..
                    "'wasmMemory'," ..
                    "'arguments'" ..
                "]"
            }

            links {
                "websocket.js"
            }
        filter {}


        filter "platforms:wasm"
            buildoptions {
                "-gdwarf-5",
                "-gseparate-dwarf"  -- preserve debug information (DWARF)
            }

            linkoptions {
                "-gdwarf-5",
                "-gseparate-dwarf", -- preserve debug information (DWARF)
                "-sSEPARATE_DWARF_URL=../debug/Tests_WASM.wasm.debug.wasm"
            }
        filter {}

        links { "%{wks.location}/ThirdParty/tiny-process-library/install/lib/tiny-process-library" }
            
        filter "configurations:*DLL*"
            links {
                "%{wks.location}/ThirdParty/googletest/lib/x64/Release/gtest_main_md",
                "%{wks.location}/ThirdParty/googletest/lib/x64/Release/gtest_md",
                "%{wks.location}/ThirdParty/googletest/lib/x64/Release/gmock_md"
            }
        filter "configurations:*Static*"
            links {
                "%{wks.location}/ThirdParty/googletest/lib/x64/Release/gtest_main_mt",
                "%{wks.location}/ThirdParty/googletest/lib/x64/Release/gtest_mt",
                "%{wks.location}/ThirdParty/googletest/lib/x64/Release/gmock_mt"
            }
        filter {}

        filter "platforms:x64"
            -- All configs need their assets in the right place
            postbuildcommands {
                "{COPY} %{prj.location}\\assets\\ %{cfg.buildtarget.directory}\\assets\\"
            }
        filter {}

        -- The tests project depend on ConnectedSpacesPlatform first finishing in order to be able to guarantee the DLLs exist before we copy them
        -- NOTE: This will slow builds down as it effectively means we need to build ConnectedSpacesPlatform twice in a linear fashion, so we need to address this in a better manner long-term.
        dependson {"ConnectedSpacesPlatform"}
        -- We call the multiplayer test runner in our tests
        dependson {"MultiplayerTestRunner"}

        filter "platforms:x64"
            postbuildcommands {
                "{COPY} %{wks.location}\\Library\\Binaries\\%{cfg.platform}\\%{cfg.buildcfg}\\ %{cfg.buildtarget.directory}", -- CSP
                "{COPY} %{wks.location}\\MultiplayerTestRunner\\Binaries\\%{cfg.platform}\\%{cfg.buildcfg}\\ %{cfg.buildtarget.directory}" --MultiplayerTestRunner
            }
        filter {}

    end
end

return Tests