#!lua

if not POCO then
	POCO = {}
end

if not POCO.NETSSL_OpenSSL then
	POCO.NETSSL_OpenSSL = {}
    
    function POCO.NETSSL_OpenSSL.AddProject()
        project "POCONetSSL_OpenSSL"
        location "ThirdParty/poco/NETSSL_OpenSSL"
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
            "%{prj.location}/../Foundation/include",
            "%{prj.location}/../Net/include",
            "%{prj.location}/../Crypto/include",
            "%{prj.location}/../Util/include",
            "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/include"
        }
        
        rtti("On")
        
        links {
            "POCOFoundation",
            "POCONet",
            "POCOCrypto",
            "POCOUtil"
        }
        
        filter "platforms:x64"
            externalincludedirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/include/platform/win64"
            }

            libdirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/lib/Win64/VS2015/Release"
            }

            links {
                "crypt32",
                "libcrypto.lib",
                "libssl.lib",
            }
        filter "platforms:Android"
            linkoptions { "-lm" } -- For gcc's math lib
            staticruntime("On")

            externalincludedirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/include/platform/android"
            }

            libdirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/lib/Android/ARM64"
            }

            links {
                "ssl",            
                "crypto",
            }
        filter "platforms:macosx"
            externalincludedirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/include/platform/macos"
            }

            libdirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/lib/Mac"
            }

            links {
                "ssl",            
                "crypto",
            }
        filter "platforms:ios"
            externalincludedirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/include/platform/ios"
            }

            libdirs {
                "%{wks.location}/ThirdParty/OpenSSL/1.1.1k/lib/IOS"
            }

            links {
                "ssl",            
                "crypto",
            }
        filter {}
    end
end
