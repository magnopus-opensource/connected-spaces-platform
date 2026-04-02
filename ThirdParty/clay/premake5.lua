#!lua

if not Clay then
	Clay = {}
end

function Clay.AddProject()
    project "clay"
    location "ThirdParty/clay"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    files {
        "%{prj.location}/clay.h",
        "%{prj.location}/clay_impl.c",
    }

    -- Source directories for this project
    externalincludedirs { 
        "%{prj.location}",
        "%{prj.location}",
    }
end