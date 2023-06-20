#!lua

CurrentDirectory = os.getcwd()

    includedirs { 
        CurrentDirectory .. "/include" 
    }

    configuration "*DebugStatic*"
        links {
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gmock_maind",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gmockd",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gtest_maind",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gtestd",
        }
    configuration "*ReleaseStatic*"
        links {
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gmock_main",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gmock",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gtest_main",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gtest",
        }
    configuration "*DebugDLL*"
        links {
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gmock_maind",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gmockd",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gtest_maind",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Debug/gtestd",
        }
    configuration "*ReleaseDLL*"
        links {
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gmock_main",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gmock",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gtest_main",
            "$(SolutionDir)/ThirdParty/googletest/lib/x64/Release/gtest",
        }        
    configuration {}
    