#!lua

if not Tests then
	Tests = {}
end

if not Tests.MultiplayerTestClient then
	Tests.MultiplayerTestClient = {}
    
    function Tests.MultiplayerTestClient.AddProject()
        project "MultiplayerTestClient"
        location "Tests/Multiplayer"
        kind "ConsoleApp"
        language "C#"
        dotnetframework "4.7.1"
        csversion "8.0"
        
        -- We only include this project for the C# build configs as it is not needed by the pure-C++ variants
        filter "configurations:not *CSharp*"
            flags { "ExcludeFromBuild" }
        filter {}

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
            "OlympusFoundation",
            "POCOFoundation",
            "CSharpWrapper"
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

        postbuildcommands {
            "{COPY} %{wks.location}/Tests/Multiplayer/3rdparty/bin/x64/ %{wks.location}/Tests/Binaries/%{cfg.platform}/%{cfg.buildcfg}"
        }
    end
end

return Tests.MultiplayerTestClient
