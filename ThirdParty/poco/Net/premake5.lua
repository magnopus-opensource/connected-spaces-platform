#!lua

if not POCO then
	POCO = {}
end

if not POCO.Net then
	POCO.Net = {}
    
    function POCO.Net.AddProject()
        project "POCONet"
        location "ThirdParty/poco/Net"
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
            "%{prj.location}/include",
            "%{prj.location}/../Foundation/include"
        }
        
        rtti("On")
        
        filter "platforms:x64"
            disablewarnings {
                "4005" -- 'NOMINMAX': macro redefinition
            }

            links {
                "iphlpapi",
            }
        filter "platforms:Android"
            linkoptions { "-lm" } -- For gcc's math lib
            staticruntime("On")

            links {
                "POCOFoundation",
                "POCOUtil"
            }
        filter {}
    end
end