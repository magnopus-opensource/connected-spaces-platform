#!lua

if not QuickJS then
	QuickJS = {}
end

function QuickJS.AddProject()
    project "quickjs"
    location "ThirdParty/quickjs"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    defines { 
        "_HAS_EXCEPTIONS=0", 
    }

    filter "platforms:x64"
        defines { 
            "JS_STRICT_NAN_BOXING"
        }
    filter "platforms:macosx"
        defines { 
            "JS_STRICT_NAN_BOXING"
        }    
    filter "platforms:ios"
        defines { 
            "JS_STRICT_NAN_BOXING"
        }
    filter "platforms:Android"
        staticruntime("On")
    filter {}
	
    files {
        "%{prj.location}/**.h",
        "%{prj.location}/**.hpp",
        "%{prj.location}/src/**.c",
    }

    -- Source directories for this project
    sysincludedirs { 
        "%{prj.location}/include",
        "%{prj.location}/src",
    }
end
