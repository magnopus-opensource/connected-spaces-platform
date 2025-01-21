#!lua

include "premake5_helpers.lua"
include "Tests/premake5.lua"
include "MultiplayerTestRunner/premake5.lua"
include "Library/premake5.lua"

-- The root premake script for CSP.
-- Windows and Android builds require a Windows workstation.
-- iOS, VisionOS and MacOS require a MacOS workstation.

--Custom build options
newoption {
    trigger     = "generate_wasm",
    description = "Generate the project for building WebAssembly. This option should only be used with the gmake2 action"
}

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

		MultiplayerTestRunner.AddProject()
    end
