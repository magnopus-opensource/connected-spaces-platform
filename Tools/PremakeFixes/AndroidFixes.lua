require "android"

local p = premake
local m = p.vstudio.vc2010

-- This premake script accounts for a known in issue in VS2019 where NDKs 21 and upwards fail to compile due to Visual Studio not taking into account gcc x64 compiler API changes in NDK versions later than 16.
-- - The specific issue is described here: https://developercommunity.visualstudio.com/idea/1003001/android-ndk-r21-cannot-find-landroid-support-1.html
-- - There is a community thread tracking the lack of modern NDK support in Visual Studio here: https://developercommunity.visualstudio.com/content/idea/782660/update-the-bundled-android-ndk-to-release-r21-lts.html

-- To summarise, the x64 ARM compilation environment as of NDK 21 no longer ships with an 'android_support' lib, as it is no longer required. This script replaces the directive that previously led to android_support being
-- included by replacing the 'android_support' token in the LibraryDependencies property of the project.

p.override(m, "additionalDependencies", function(base, cfg, explicit)
    if cfg.system == p.ANDROID then
        local links = {}

        -- Do NOT explicitly link project references! This is taken care of by the "projectReferences" override below
        --if explicit then
        --    links = premake.config.getlinks(cfg, "siblings", "fullpath")
        --end

        -- Then the system libraries, which come undecorated
        local system = p.config.getlinks(cfg, "system", "name")
        for i = 1, #system do
            local link = system[i]
            table.insert(links, link)
        end

        -- TODO: When to use LibraryDependencies vs AdditionalDependencies

        if #links > 0 then
            links = path.translate(table.concat(links, ";"))
            m.element("LibraryDependencies", nil, "$([MSBuild]::Unescape($([System.String]::Copy('%%(LibraryDependencies)').Replace('android_support',''))));%s", links)
        end
    else
        return base(cfg, explicit)
    end
end)


-- This override fixes an issue with platform-specific project references not being correctly linked against
local function projectReferenceLinkLibraryDependencies(prj, ref)
    for cfg in p.project.eachconfig(prj) do
        if cfg._needsExplicitLink then
            local deps = p.config.getlinks(cfg, "dependencies", "object")
            
            if #deps > 0 then
                local dependsOn = false

                for i = 1, #deps do
                    local dep = deps[i]

                    if dep.filename == ref.name then
                        dependsOn = true
                        break
                    end
                end
                
                if dependsOn then
                    p.w("<LinkLibraryDependencies %s>true</LinkLibraryDependencies>", m.condition(cfg))
                end
            end
        end
    end
end

p.override(m.elements, "projectReferences", function(base, prj, ref)
    local calls = base(prj)

    if prj.clr == p.OFF then
        table.insertafter(calls, m.referenceProject, projectReferenceLinkLibraryDependencies)
    end

    return calls
end)