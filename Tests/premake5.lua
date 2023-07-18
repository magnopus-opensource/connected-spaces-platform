#!lua

include "Library/premake5.lua"


if not Tests then
	Tests = {}
        
    function Tests.AddProject()
        project "Tests"
        location "Tests"
        kind "None" -- By default we don't build this project
        filter "platforms:x64"
            kind "ConsoleApp" -- We build this as a console app for windows
        filter {}
        
        files {
            "%{prj.location}/src/**.h",
            "%{prj.location}/src/**.cpp",
            "%{prj.location}/src/**.hpp",
            "%{prj.location}/assets/**.*"
        }
        
        sysincludedirs { 
            "%{prj.location}/src",
            "%{wks.location}/ThirdParty/googletest/include",
        }   
        
        debugdir "%{prj.location}\\Binaries\\%{cfg.platform}\\%{cfg.buildcfg}"
        
        -- The tests project is configured for the most part like the foundation project itself.
        -- This allows us to test both internal functionality as well as the public API.
        Project.DefineProject()
        
        -- Set tests executable name
        targetname( "Tests" )

        -- Tell Foundation we're compiling tests
        defines { "CSP_TESTS" }

        -- Compile support for MessagePack
        defines { "USE_MSGPACK" }
       
        -- If we're running on a build agent, we want to run *all* the tests
        if CSP.IsRunningOnTeamCityAgent() then
            result = iif(CSP.IsRunningNightlyBuild(), "RUN_NIGHTLY_TESTS", "RUN_ALL_UNIT_TESTS")
            defines { result }
        end
        
        -- Config for platforms
        filter "platforms:x64"
            defines { "CSP_WINDOWS" }
            linkoptions { "/ignore:4099"} -- Because we don't have debug symbols for OpenSSL libs
        filter "platforms:Android"
            defines { "CSP_ANDROID" }
        filter {}
            
        filter "configurations:*DLL*"
            links {
                "%{wks.location}/ThirdParty/googletest/lib/x64/Release/gtest_main_md",
                "%{wks.location}/ThirdParty/googletest/lib/x64/Release/gtest_md"
            }
        filter "configurations:*Static*"
            links {
                "%{wks.location}/ThirdParty/googletest/lib/x64/Release/gtest_main_mt",
                "%{wks.location}/ThirdParty/googletest/lib/x64/Release/gtest_mt"
            }
        filter {}
        
        -- All configs need their assets in the right place
        postbuildcommands {
            "{COPY} %{prj.location}\\assets\\ %{cfg.buildtarget.directory}\\assets\\"
        }
        
        -- The tests project depend on ConnectedSpacesPlatform first finishing in order to be able to guarantee the DLLs exist before we copy them
        -- NOTE: This will slow builds down as it effectively means we need to build ConnectedSpacesPlatform twice in a linear fashion, so we need to address this in a better manner long-term.
        dependson {"ConnectedSpacesPlatform"}
            
        postbuildcommands {
            "{COPY} %{wks.location}\\Library\\Binaries\\%{cfg.platform}\\%{cfg.buildcfg}\\ %{cfg.buildtarget.directory}"
        }
    end
end

return Tests