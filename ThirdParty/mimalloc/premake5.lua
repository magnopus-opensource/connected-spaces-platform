#!lua

if not MiMalloc then
	MiMalloc = {}
end

function MiMalloc.AddProject()
    project "mimalloc"
    location "ThirdParty/mimalloc"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    files {
        "%{prj.location}/**.h",
        "%{prj.location}/src/static.c",
    }

    -- Source directories for this project
    sysincludedirs { 
        "%{prj.location}/include",
        "%{prj.location}/src",
    }

    filter "platforms:Android"
        flags { "ExcludeFromBuild" }
    filter "platforms:macosx"
        flags { "ExcludeFromBuild" }
    filter {}
end