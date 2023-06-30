#!lua

if not Tests then
	Tests = {}
end

if not Tests.CSharp then
	Tests.CSharp = {}
    
    function Tests.CSharp.AddProject()
        project "CSharpTests"
        location "Tests/CSharp"
        kind "ConsoleApp"
        language "C#"
        dotnetframework "4.7.1"
        csversion "8.0"
        
        -- We only include this project for the C# build configs as it is not needed by the pure-C++ variants
        filter "configurations:not *CSharp*"
            flags { "ExcludeFromBuild" }
        filter {}

        -- If we're running on a build agent, we want to run *all* the tests
        if CSP.IsRunningOnTeamCityAgent() then
            result = iif(CSP.IsRunningNightlyBuild(), "RUN_NIGHTLY_TESTS", "RUN_ALL_UNIT_TESTS")
            defines { result }
        end

        files {
            "%{prj.location}/src/**.cs",
            "%{prj.location}/assets/**.*",
        }
        
        targetdir "%{wks.location}/Tests/Binaries/%{cfg.platform}/%{cfg.buildcfg}"

        links {
            "%{wks.location}/Tests/Multiplayer/3rdparty/bin/x64/ServiceWire",
        }
        	
        links {
            "System",
            "System.Core",
            "Microsoft.CSharp",
            "CSharpWrapper",
            "MultiplayerTestClient"
        }

        filter "platforms:Android"
            flags { "ExcludeFromBuild" }
        filter "configurations:*Static*"
            flags { "ExcludeFromBuild" }
        filter "configurations:DebugDLL"
            defines { "TRACE", "DEBUG" }
        filter "files:*/assets/**.*"
            buildaction "Copy"
        filter {}

        defines { "$(DefineConstants)" }
    end
end

return Tests.CSharp
