#!lua

if not POCO then
	POCO = {}
end

if not POCO.Foundation then
	POCO.Foundation = {}
    
    function POCO.Foundation.AddProject()
        project "POCOFoundation"
        location "ThirdParty/poco/Foundation"
        kind "StaticLib"
        language "C++"
        cppdialect "C++17"
		warnings "Off"

        files {
            "%{prj.location}/**.h",
            "%{prj.location}/**.cpp",
            "%{prj.location}/**.c"
        }
        
        defines {
            "POCO_STATIC",
            "POCO_NO_AUTOMATIC_LIBS",
            "POCO_NO_INOTIFY",
            "POCO_NO_FILECHANNEL",
            "POCO_NO_SPLITTERCHANNEL",
            "POCO_NO_SYSLOGCHANNEL",
            "POCO_UTIL_NO_INIFILECONFIGURATION",
            "POCO_UTIL_NO_JSONCONFIGURATION",
            "POCO_UTIL_NO_XMLCONFIGURATION",
            "POCO_NET_NO_IPv6"
        }
        
        externalincludedirs {
            "%{prj.location}/include"
        }
        
        excludes { 
            "**_POSIX**", 
            "**_UNIX**",
            "**_WINCE**",
            "**_C99**",
            "**_DEC**",
            "**_SUN**",
            "**_DUMMY**",
            "**_HPUX**",
            "**_STD**",
            "**_QNX**",
            "**_VX**",
            "**_WIN32**",
            "**IOS**",
            "**Android**",
        }
        
        rtti("On")

        filter "platforms:x64"
            excludes { 
                "**SysLog**", -- UNIX-specific logging
            }
            links {
                "iphlpapi",
            }
        filter { "platforms:Android or macosx or ios" }
            excludes { 
                "**Windows**",
                "**EventLog**", -- Windows-specific logging
            }
            -- This cancels the exclusion of encoding source files with 'Windows' in the filename that we do want to compile on other platforms
            files { 
                "%{prj.location}/**Encoding**"
            }
            
            defines {
                "POCO_NO_WINDOWS_H",
                "POCO_NO_FPENVIRONMENT",
                "POCO_NO_WSTRING",
                "POCO_NO_SHAREDMEMORY"
            }
        filter "platforms:ios"
            defines { "POCO_NO_STAT64" }
        filter "platforms:Android"
            linkoptions { "-lm" } -- For gcc's math lib
            staticruntime("On")
        filter {}
    end
end