#!lua

include "Tools/VisualStudioNDK21Fix/VisualStudioNDK21Fix.lua"

include "Library/CSharpWrapper/premake5.lua"

include "ThirdParty/poco/Foundation/premake5.lua"
include "ThirdParty/poco/Util/premake5.lua"
include "ThirdParty/poco/Net/premake5.lua"
include "ThirdParty/poco/Crypto/premake5.lua"
include "ThirdParty/poco/NETSSL_OpenSSL/premake5.lua"

include "ThirdParty/signalrclient/premake5.lua"
include "ThirdParty/mimalloc/premake5.lua"
include "ThirdParty/quickjs/premake5.lua"
include "modules/premake5.lua"


if not Project then
    Project = {}
    
    function Project.Preprocess()
        -- Generate version
        cwd = os.getcwd()

        if CSP.IsTargettingMacOS() then
            if not CSP.IsGeneratingVS() then
                code = os.execute("python3 " .. cwd .. "/Tools/VersionGenerator/VersionGenerator.py -ci=" .. tostring(CSP.IsRunningOnTeamCityAgent()))

                if (code ~= true) then
                    return code
                end
            end
        else
            code = os.execute("py " .. cwd .. "/Tools/VersionGenerator/VersionGenerator.py -ci=" .. tostring(CSP.IsRunningOnTeamCityAgent()))

            if (code ~= true) then
                return code
            end
        end
    
        -- Generate wrapper code
        return WrapperGenerator.Generate()
    end
    
    function Project.DefineProject()
        language "C++"

        files {
            "%{wks.location}/Library/**.h",
            "%{wks.location}/Library/**.cpp",
            "%{wks.location}/modules/olympus-foundation-chs/generated/**.h",
            "%{wks.location}/modules/olympus-foundation-chs/generated/**.cpp"
        }
        
        -- Exclude signalr clients as appropriate for specified configs
        if CSP.IsWebAssemblyGeneration() then 
            excludes { 
                "**POCOSignalRClient**",
                "**POCOWebClient**",
            }
        else
            excludes { 
                "**EmscriptenSignalRClient**",
                "**EmscriptenWebClient**",
            }
        end

        characterset ("ASCII")

        -- Include directories for this project
        externalincludedirs {
            "%{wks.location}/Library/src",
            "%{wks.location}/Library/include",
            "%{wks.location}/ThirdParty/signalrclient/include",
            "%{wks.location}/ThirdParty/rapidjson/include",
            "%{wks.location}/ThirdParty/msgpack/include",
            "%{wks.location}/ThirdParty/quickjs/include",
            "%{wks.location}/ThirdParty/atomic_queue/include",
            "%{wks.location}/modules/olympus-foundation-chs/generated",
			"%{wks.location}/modules/tinyspline/src",
        }

        filter "platforms:not wasm"
            externalincludedirs {
                -- mimalloc and POCO are not used in WASM builds 
                "%{wks.location}/ThirdParty/mimalloc/include",
                "%{wks.location}/ThirdParty/poco/Foundation/include",
                "%{wks.location}/ThirdParty/poco/Util/include",
                "%{wks.location}/ThirdParty/poco/Net/include",
                "%{wks.location}/ThirdParty/poco/Crypto/include",
                "%{wks.location}/ThirdParty/poco/NETSSL_OpenSSL/include",
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/include"
            }
        filter {}
        
        -- Preprocessor defines
        defines { 
            "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING",
            "NO_SIGNALRCLIENT_EXPORTS",
            "USE_MSGPACK",
            "POCO_NO_INOTIFY",
            "POCO_NO_FILECHANNEL",
            "POCO_NO_SPLITTERCHANNEL",
            "POCO_NO_SYSLOGCHANNEL",
            "POCO_UTIL_NO_INIFILECONFIGURATION",
            "POCO_UTIL_NO_JSONCONFIGURATION",
            "POCO_UTIL_NO_XMLCONFIGURATION",
            "POCO_NET_NO_IPv6"
        }

        filter "platforms:not wasm"
            defines { 
                "POCO_STATIC",
                "POCO_NO_AUTOMATIC_LIBS"
            }

        filter {}
        
        -- Needed for dynamic_cast
        rtti("On")
        
        -- Config for platforms
        filter "platforms:x64"
            defines { 
				"CSP_WINDOWS",
				"JS_STRICT_NAN_BOXING",
                "PERMISSIVE"
			}

			disablewarnings {
                "4251",  -- Ignore dll interface warnings
                "4996",  -- Ignore deprecated warnings
                "4200"   -- Ignore nonstandard extension warnings (for quickjspp)
            }

            linkoptions {
                "-IGNORE:4099"  -- Because we don't have debug symbols for OpenSSL libs
            }

            externalincludedirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/include/platform/win64"
            }

            flags { 
                "MultiProcessorCompile" 
            }

            files {
                "%{wks.location}/Library/cpp.hint"
            }
			
			buildoptions{
				"/bigobj"
			}
        filter "platforms:Android"
            defines { "CSP_ANDROID" }
            staticruntime("On")

            linkoptions { "-lm" } -- For gcc's math lib

            externalincludedirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/include/platform/android"
            }

            libdirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/lib/Android/ARM64"
            }

            links {
                "ssl",
                "crypto"
            }

            flags { 
                "MultiProcessorCompile" 
            }
        filter "platforms:macosx"
            defines { 
                "CSP_MACOSX", 				
                "JS_STRICT_NAN_BOXING"
            }

            externalincludedirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/include/platform/macos"
            }

            libdirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/lib/Mac"
            }

            links { 
                "ssl",
                "crypto"
            }
        filter "platforms:ios"
            defines { 
                "CSP_IOS",
                "USE_STD_MALLOC=1",
                "JS_STRICT_NAN_BOXING"
            }

            externalincludedirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/include/platform/ios"
            }

            libdirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/lib/IOS"
            }

            links {
                "ssl",            
                "crypto"
            }
        filter "platforms:wasm"
            rtti("Off")

            defines {
                "CSP_WASM",
                "USE_STD_MALLOC=1"
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
                "-sEXPORTED_FUNCTIONS=['_malloc']",                             -- force export _malloc function
                "-sEXPORT_ES6=1 -sMODULARIZE=1 -sEXPORT_NAME='createModule'",   -- export binary as an ES6 module
                "-sFETCH",                                                      -- enable Emscripten's Fetch API (needed for making REST calls to CHS)
                "-sALLOW_TABLE_GROWTH=1",                                       -- needed for registering callbacks that are passed to Foundation
                "-sWASM_BIGINT",                                                -- enable support for JavaScript's bigint (needed for 64-bit integer support)
                "-sENVIRONMENT='web,worker'",                                   -- only compile for web and worker (worker is required for multi-threading)
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
                "-sINCOMING_MODULE_JS_API=[" ..
                    "'buffer'," ..
                    "'fetchSettings'," ..
                    "'instantiateWasm'," ..
                    "'locateFile'," ..
                    "'mainScriptUrlOrBlob'," ..
                    "'wasmMemory'" ..
                "]"
            }

            links {
                "websocket.js"
            }
        filter { "platforms:wasm", "configurations:*Debug*" }
            buildoptions {
                "-gdwarf-5",
                "-gseparate-dwarf"  -- preserve debug information (DWARF)
            }

            linkoptions {
                "-gdwarf-5",
                "-gseparate-dwarf", -- preserve debug information (DWARF)
                "-sSEPARATE_DWARF_URL=../debug/OlympusFoundation_WASM.wasm.debug.wasm"
            }
        filter { "platforms:wasm", "configurations:*Release*" }
            -- We want to reduce the size of Release builds as much as possible
            buildoptions {
                "-Os",
                "-flto"
            }

            linkoptions {
                "-Os",
                "-flto"
            }
        filter {}

        -- Libs for all configs to link against
        links {
            "signalrclient",
            "quickjs",
			"tinyspline"
        }

        filter "platforms:not wasm"
            links {
                "WS2_32",
                "POCONetSSL_OpenSSL",
                "mimalloc",
            }

        filter {}

        -- Debug/Release config settings
        filter "configurations:*Debug*"
            targetname( "OlympusFoundation_D" )
        filter "configurations:*Release*"
            targetname( "OlympusFoundation" )
        filter "platforms:wasm"
            targetname( "OlympusFoundation_WASM.js" )
            kind "None"
        filter {}
    end
        
    function Project.AddProject()
        if not CSP.IsGeneratingCSharpOnMac() then
            project "OlympusFoundation"
            location "Library"
            
            -- Static/shared lib settings
            filter "configurations:*Static*"
                kind "StaticLib"
            filter "configurations:*DLL*"    
                kind "SharedLib"
                defines { "BUILD_CSP_DLL" }
            filter {}

            Project.DefineProject()

            SignalRClient.AddProject()
            QuickJS.AddProject()
			TinySpline.AddProject()
        end
        
        --Add the following projects only for a non WebAssembly project generation
        if not CSP.IsWebAssemblyGeneration() then
            if not CSP.IsGeneratingCSharpOnMac() then
                MiMalloc.AddProject()

                group("POCO")
                    POCO.Foundation.AddProject()
                    POCO.Util.AddProject()
                    POCO.Net.AddProject()
                    POCO.Crypto.AddProject()
                    POCO.NETSSL_OpenSSL.AddProject()
                group("")
            end

            if not CSP.IsGeneratingCPPOnMac() then
                WrapperGenerator.AddProject()
            end
        end
    end
end
