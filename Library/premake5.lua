#!lua

include "Tools/PremakeFixes/AndroidFixes.lua"

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

        if CSP.IsAppleTarget() then
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
			"%{wks.location}/Library/docs/source/**.md",
            "%{wks.location}/modules/csp-services/generated/**.h",
            "%{wks.location}/modules/csp-services/generated/**.cpp"
        }
        
        -- Exclude signalr clients as appropriate for specified configs
        if CSP.IsWebAssemblyGeneration() then 
            excludes { 
                "**POCOSignalRClient**",
                "**POCOWebClient**"
            }
        else
            excludes { 
                "**EmscriptenSignalRClient**",
                "**EmscriptenWebClient**"
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
            "%{wks.location}/ThirdParty/glm",
            "%{wks.location}/ThirdParty/atomic_queue/include",
            "%{wks.location}/modules/csp-services/generated",
			"%{wks.location}/modules/tinyspline/src"
        }

        filter "platforms:not wasm"
            externalincludedirs {
                -- mimalloc is not used in WASM builds 
                "%{wks.location}/ThirdParty/mimalloc/include",
                -- POCO is not used in WASM builds
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

            links {
                "WS2_32"
            }
        filter "platforms:Android"
            defines {
                "CSP_ANDROID",
                "USE_STD_MALLOC=1"
            }
            staticruntime("On")
			
			buildoptions {
				"-Wno-error=deprecated-declarations", --Don't error on deprecation warnings, this is because we use Uri a lot in our services generated code, which has deprecation warnings for some unused but still generated endpoints.
				"-Wno-braced-scalar-init", -- Don't warn against doing stuff like `return {0}`, which we do in the interop output.
				"-Wno-error=unused-lambda-capture", --This shouldn't be disabled, we just had to rush to unblock android builds. Take all the this captures out and remove.
				"-Wno-unknown-pragmas", --Also not the greatest. This is to try and suppress a signalR warning, even though the signalR project dosen't emit warnings (I think this error is a bit unique cause of preprocessor stuff)
				"-Wno-error=nonportable-include-path", --Include paths dont match file structure. Should get around to fixing
				"-Wno-error=format-security" --In logging, __android_log_print(ANDROID_LOG_VERBOSE, "CSP", InMessage.c_str()) is unnaceptable for some reason.
            }

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
                "USE_STD_MALLOC=1",
                "JS_STRICT_NAN_BOXING"
            }

            externalincludedirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/include/platform/macos"
            }

            libdirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/lib/Mac"
            }
			
			-- Could not manage to get xcode to co-operate in any other less specific manner of setting the flags.
			-- These disables are to do with warnings in generated code that we should get around to dealing with.
			xcodebuildsettings {
				["WARNING_CFLAGS"] = "-Wno-error=deprecated-declarations -Wno-braced-scalar-init"
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
			
			-- Could not manage to get xcode to co-operate in any other less specific manner of setting the flags.
			-- These disables are to do with warnings in generated code that we should get around to dealing with.
			xcodebuildsettings {
				["WARNING_CFLAGS"] = "-Wno-error=deprecated-declarations -Wno-braced-scalar-init"
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
                "-fwasm-exceptions",     -- enable native wasm exceptions
				"-Wno-error=deprecated-declarations", --Don't error on deprecation warnings, this is because we use Uri a lot in our services generated code, which has deprecation warnings for some unused but still generated endpoints.
				"-Wno-braced-scalar-init" -- Don't warn against doing stuff like `return {0}`, which we do in the interop output.
            }

            linkoptions { 
                "-pthread",                                                     -- enable threading
                "-fwasm-exceptions",                                            -- enable native wasm exceptions
                "-sPTHREAD_POOL_SIZE_STRICT=0",                                 -- disable thread pool and spin up threads when we need them
                "-sEXPORTED_FUNCTIONS=['_malloc','_free']",                     -- force export _malloc and _free function
                "-sEXPORT_ES6=1 -sMODULARIZE=1 -sEXPORT_NAME='createModule'",   -- export binary as an ES6 module
                "-sFETCH",                                                      -- enable Emscripten's Fetch API (needed for making REST calls to CHS)
                "-sALLOW_TABLE_GROWTH=1",                                       -- needed for registering callbacks that are passed to Connected Spaces Platform
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
                "]",
                --"-sUSE_ES6_IMPORT_META=0"                                       -- disable use of import.meta as it is not yet supported everywhere
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
                "-sSEPARATE_DWARF_URL=../debug/ConnectedSpacesPlatform_WASM.wasm.debug.wasm"
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

        filter { "platforms:not wasm", "platforms:not Android", "platforms:not macosx", "platforms:not ios" }
            links {
                "mimalloc"
            }
        filter "platforms:not wasm"
            links {
                "POCONetSSL_OpenSSL"
            }
        filter {}

        if CSP.IsVisionOSTarget() then
            filter "platforms:ios"
                libdirs {
                    "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/lib/VisionOS"
                }
            filter {}
        else
            filter "platforms:ios"
                libdirs {
                    "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/lib/IOS"
                }
            filter {}
        end

        -- Debug/Release config settings
        filter "configurations:*Debug*"
            targetname( "ConnectedSpacesPlatform_D" )
        filter "configurations:*Release*"
            targetname( "ConnectedSpacesPlatform" )
        filter "platforms:wasm"
            targetname( "ConnectedSpacesPlatform_WASM.js" )
            kind "None"
        filter {}
    end
        
    function Project.AddProject()
        if not CSP.IsGeneratingCSharpOnMac() then
            project "ConnectedSpacesPlatform"
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
