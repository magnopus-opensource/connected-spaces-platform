require "android"

-- This premake script accounts for a known in issue in VS2019 where NDKs 21 and upwards fail to compile due to Visual Studio not taking into account gcc x64 compiler API changes in NDK versions later than 16.
-- - The specific issue is described here: https://developercommunity.visualstudio.com/idea/1003001/android-ndk-r21-cannot-find-landroid-support-1.html
-- - There is a community thread tracking the lack of modern NDK support in Visual Studio here: https://developercommunity.visualstudio.com/content/idea/782660/update-the-bundled-android-ndk-to-release-r21-lts.html

-- To summarise, the x64 ARM compilation environment as of NDK 21 no longer ships with an 'android_support' lib, as it is no longer required. This script replaces the directive that previously led to android_support being
-- included by replacing the 'android_support' token in the LibraryDependencies property of the project.

premake.override(premake.vstudio.vc2010, "additionalDependencies", function(oldfn, cfg)
    if cfg.system == premake.ANDROID then
        local links = {}

        -- If we need sibling projects to be listed explicitly, grab them first
        if explicit then
            links = premake.config.getlinks(cfg, "siblings", "fullpath")
        end

        -- Then the system libraries, which come undecorated
        local system = premake.config.getlinks(cfg, "system", "name")
        for i = 1, #system do
            local link = system[i]
            table.insert(links, link)
        end

        -- TODO: When to use LibraryDependencies vs AdditionalDependencies

        if #links > 0 then
            links = path.translate(table.concat(links, ";"))
            premake.vstudio.vc2010.element("LibraryDependencies", nil, "$([MSBuild]::Unescape($([System.String]::Copy('%%(LibraryDependencies)').Replace('android_support',''))));%s", links)
        end
    else
        return oldfn(cfg, explicit)
    end
end)