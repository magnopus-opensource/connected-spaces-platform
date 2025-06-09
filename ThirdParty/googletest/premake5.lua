#!lua

CurrentDirectory = os.getcwd()

    includedirs { 
        CurrentDirectory .. "/include" 
    }

    filter "configurations:*DebugStatic*"
        links {
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gmock_maind",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gmockd",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gtest_maind",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gtestd",
        }
    filter "configurations:*ReleaseStatic*"
        links {
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gmock_main",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gmock",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gtest_main",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gtest",
        }
    filter "configurations:*DebugDLL*"
        links {
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gmock_maind",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gmockd",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gtest_maind",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gtestd",
        }
    filter "configurations:*ReleaseDLL*"
        links {
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gmock_main",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gmock",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gtest_main",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gtest",
        }        
    filter {}
    