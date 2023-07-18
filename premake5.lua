#!lua

include "premake5_helpers.lua"
include "Tests/premake5.lua"
include "Tests/CSharp/premake5.lua"
include "Tests/Multiplayer/premake5.lua"
include "Library/premake5.lua"


--Custom build options
newoption {
    trigger     = "generate_wasm",
    description = "Generate the project for building WebAssembly. This option should only be used with the gmake2 action"
}


-- @RossB: We create separate workspaces/projects for iOS and MacOSX platforms
premake.override(_G, "project", function(base, ...)
    local rval = base(...)
    local args = {...}
    filter "system:ios"
        filename(args[1] .. "_ios")
    filter {}
    return rval
end)

premake.override(_G, "workspace", function(base, ...)
    local rval = base(...)
    local args = {...}
    filter "system:ios"
        filename(args[1] .. "_ios")
    filter {}
    return rval
end)


solution( "ConnectedSpacesPlatform" )
    -- Build configurations
    if CSP.HasCommandLineArgument("DLLOnly") then
        configurations { "DebugDLL", "ReleaseDLL", "DebugDLL-CSharp", "ReleaseDLL-CSharp" }
    elseif CSP.IsWebAssemblyGeneration() then
        configurations { "Debug", "Release" }
    else
        configurations { "DebugStatic", "ReleaseStatic", "DebugDLL", "ReleaseDLL", "DebugDLL-CSharp", "ReleaseDLL-CSharp" }
    end

    -- Apply standard settings across the solution
    CSP.UseStandardSettings()

    code = Project.Preprocess()
    
    if (code ~= true) then
        io.stderr:write("** ERROR: Project.Preprocess failed. Please check log for details.")
        return
    end

    if CSP.IsWebAssemblyGeneration() then
        CSP.Platforms.AddWebAssembly()
    else
        CSP.Platforms.AddWindows()
        CSP.Platforms.AddAndroid()
        CSP.Platforms.AddMac()
        CSP.Platforms.AddIOS()
    end
    
    -- Visual studio projects
    Project.AddProject()
    
    if not CSP.IsWebAssemblyGeneration() then
        if not CSP.IsGeneratingCSharpOnMac() then
            Tests.AddProject()
        end

        if not CSP.IsGeneratingCPPOnMac() then
            Tests.CSharp.AddProject()
        end
        
        if not CSP.IsGeneratingCPPOnMac() then
            Tests.MultiplayerTestClient.AddProject()
        end
    end
