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

newoption {
   trigger = "wasm_with_node",
   description = "Compile nodeJS support into the wasm build"
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

    postbuildcommands {
        'if "$(ProjectName)" == "ConnectedSpacesPlatform" goto :run_cppcheck',
        'if "$(ProjectName)" == "Tests" goto :run_cppcheck',
        'if "$(ProjectName)" == "MultiplayerTestRunner" goto :run_cppcheck',
        'goto :skip_cppcheck',

        ':run_cppcheck',
        'echo "Running Cppcheck analysis for $(ProjectName) - $(Configuration)..."',
        'setlocal enabledelayedexpansion',
        'set "DEFS=$(PreprocessorDefinitions)"',
        'set "INCS=$(AdditionalIncludeDirectories)"',
        'set "DEFS=!DEFS:;= -D!"',
        'set "INCS=!INCS:;= -I!"',
        'cppcheck --enable=all --xml --xml-version=2 --output-file="$(SolutionDir)$(ProjectName)_$(Configuration)_$(Platform).xml" --suppressions-list=$(SolutionDir)cppcheck_suppressions.txt --std=c++17 -D"!DEFS!" -I"!INCS!" "$(ProjectDir)src"',
        'echo "Cppcheck analysis complete for $(ProjectName) - $(Configuration)."',
        'goto :eof',

        ':skip_cppcheck',
        'echo "Skipping Cppcheck analysis for $(ProjectName) - not a specified project to run on."',
        ':eof'
    }

    -- Visual studio projects
    Project.AddProject()
    
    if not CSP.IsWebAssemblyGeneration() then

		if not CSP.IsGeneratingCSharpOnMac() then
			Tests.AddProject()
		end

		MultiplayerTestRunner.AddProject()
    end
