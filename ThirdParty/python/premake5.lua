#!lua

CurrentDirectory = os.getcwd()

    includedirs { 
        CurrentDirectory .. "/include" 
    }
    
    libdirs {
        CurrentDirectory .. "/libs/"
    }
    
    configuration "Debug"
        postbuildcommands { "copy $(SolutionDir)\\ThirdParty\\python\\python38.dll $(SolutionDir)\\Application\\Binaries\\x64\\Debug\\python38.dll" }
    configuration "Release"
        postbuildcommands { "copy $(SolutionDir)\\ThirdParty\\python\\python38.dll $(SolutionDir)\\Application\\Binaries\\x64\\Release\\python38.dll" }
    configuration {}