#!lua

if not WrapperGenerator then
    WrapperGenerator = {}
        
    function WrapperGenerator.Generate()
        local script_path = "Tools/WrapperGenerator/WrapperGenerator.py"

        if os.istarget("macosx") or os.istarget("ios") then
            if _ACTION == "vs2019" then
                return os.execute("python3 " .. script_path .. " --generate_csharp")
            else
                return os.execute("python3 " .. script_path)
            end
        elseif os.istarget("windows") then
            return os.execute("py " .. script_path .. " --generate_csharp --generate_typescript")
        else
            return os.execute("py " .. script_path)
        end
    end
    
    function WrapperGenerator.AddProject()
        project "CSharpWrapper"
        location "Library/CSharpWrapper"
        targetname "ConnectedSpacesPlatform"
        kind "SharedLib"
        targetdir "%{wks.location}/Library/Binaries/%{cfg.platform}/%{cfg.buildcfg}"
        language "C#"
        dotnetframework "4.7.1"
        csversion "8.0"
        clr "unsafe"
        
        -- We only include this project for the C# build configs as it is not needed by the pure-C++ variants
        filter "configurations:not *CSharp*"
            flags { "ExcludeFromBuild" }
        filter {}

        files {
            "%{prj.location}/src/**.cs",
            "%{wks.location}/Tools/WrapperGenerator/Output/CSharp/**.cs"
        }

        vpaths {
            ["src/generated/*"] = "Tools/WrapperGenerator/Output/CSharp/**.cs"
        }

        links {
            "System",
            "System.Core",
            "Microsoft.CSharp"
        }

        configuration "*Debug*"
            defines { "TRACE", "DEBUG" }
        configuration {} 
    end
end