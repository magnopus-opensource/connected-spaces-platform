#!lua

if not MultiplayerTestRunner then
	MultiplayerTestRunner = {}
        
    function MultiplayerTestRunner.AddProject()
        project "MultiplayerTestRunner"
        location "MultiplayerTestRunner"

        kind "ConsoleApp"
        
        removeplatforms  { "ios", "macosx", "Android", "wasm" }

        files {
            "%{prj.location}/src/**.h",
            "%{prj.location}/src/**.cpp",
            "%{prj.location}/assets/**.*",
			"%{prj.location}/include/**.h",
			"%{prj.location}/thirdparty/**.h",
			"%{prj.location}/thirdparty/**.hpp"
        }
		
		includedirs {
			"%{prj.location}/include/**.h"
		}
		
		externalincludedirs {
			"%{prj.location}/thirdparty/**.hpp",
			"%{prj.location}/thirdparty/**.h",
            "%{wks.location}/Library/include", --CSP
			"%{wks.location}/ThirdParty/googletest/include",
			"%{wks.location}/ThirdParty/uuid-v4"
		}

        debugdir "%{prj.location}/Binaries/%{cfg.platform}/%{cfg.buildcfg}"
        
		-- Link to CSP in place
        dependson {"ConnectedSpacesPlatform"}
        libdirs {"%{wks.location}/Library/Binaries/%{cfg.platform}/%{cfg.buildcfg}"}
		links {"ConnectedSpacesPlatform"}
		defines {"USING_CSP_DLL"}
		
		filter "platforms:x64"
            linkoptions { "/ignore:4099"} --Complains about no PDB for googletest, don't care.
			   
		 -- Conditionally link google test, not the standard _d stuff so we need to do it per config
	   filter "configurations:*Debug*"
		  links { "gtestd_md" } -- Debug versions
		  libdirs {"%{wks.location}/ThirdParty/googletest/lib/%{cfg.platform}/Debug"}
		  staticruntime "off"
		  runtime "Debug"

	   filter "configurations:*Release*"
		  links { "gtest_md" } -- Release versions
		  libdirs {"%{wks.location}/ThirdParty/googletest/lib/%{cfg.platform}/Release"}
		  staticruntime "off"
	      runtime "Release"

	   -- Reset filters
	   filter {}

       -- Set MultiplayerTestRunner executable name
       targetname( "MultiplayerTestRunner" )
		
	   -- Copy CSP to the working directory
	   postbuildcommands {
	       "{COPY} %{wks.location}\\Library\\Binaries\\%{cfg.platform}\\%{cfg.buildcfg}\\ %{cfg.buildtarget.directory}"
	   }

    end
end

return MultiplayerTestRunner