#!lua

CurrentDirectory = os.getcwd()

    includedirs { 
        CurrentDirectory .. "/include" 
    }
    
    libdirs {
        CurrentDirectory .. "/libs/"
    }
    
    filter "configurations:Debug"
        postbuildcommands { "copy $(SolutionDir)\\ThirdParty\\python\\python38.dll $(SolutionDir)\\Application\\Binaries\\x64\\Debug\\python38.dll" }
    filter "configurations:Release"
        postbuildcommands { "copy $(SolutionDir)\\ThirdParty\\python\\python38.dll $(SolutionDir)\\Application\\Binaries\\x64\\Release\\python38.dll" }
    filter {}