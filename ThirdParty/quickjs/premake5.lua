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
        "CONFIG_BIGNUM",
    }

    warnings "Off"

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
    filter "platforms:wasm"
        defines {
            "_GNU_SOURCE",       -- expose environ and sighandler_t (musl hides them by default in emsdk 5.0+)
            "EMSCRIPTEN",        -- quickjs.c checks `defined(EMSCRIPTEN)` (bare, not __EMSCRIPTEN__) to skip malloc_usable_size
            "CONFIG_STACK_CHECK" -- enable js_check_stack_overflow; without it JS_SetMaxStackSize is a no-op and deep recursion falls through to V8's RangeError
        }
    filter {}
	
    files {
        "%{prj.location}/**.h",
        "%{prj.location}/**.hpp",
        "%{prj.location}/src/**.c",
    }

    -- Source directories for this project
    externalincludedirs { 
        "%{prj.location}/include",
        "%{prj.location}/src",
    }
end
