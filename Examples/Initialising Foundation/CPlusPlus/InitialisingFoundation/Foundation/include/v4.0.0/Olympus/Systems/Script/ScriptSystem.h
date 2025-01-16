#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"

#include <functional>
#include <string>
#include <vector>

namespace oly_systems
{

class IScriptBinding
{
public:
    virtual ~IScriptBinding() = default;
    virtual void Bind(int64_t ContextId, class ScriptSystem* InScriptSystem) = 0;
};

class OLY_API OLY_NO_DISPOSE ScriptSystem
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    friend class ScriptContext;
    /** @endcond */

public:
    ~ScriptSystem();

    void Initialise();
    void Shutdown();

    bool RunScript(int64_t ContextId, const oly_common::String& ScriptText);
    bool RunScriptFile(int64_t ContextId, const oly_common::String& ScriptFilePath);

    // Experimental binding interface (not exposed to wrappergen)
    OLY_START_IGNORE
    bool CreateContext(int64_t ContextId);
    bool DestroyContext(int64_t ContextId);
    bool BindContext(int64_t ContextId);
    bool ResetContext(int64_t ContextId);
    bool ExistsInContext(int64_t ContextId, const oly_common::String& ObjectName);
    void* GetContext(int64_t ContextId);
    void* GetModule(int64_t ContextId, const oly_common::String& ModuleName);
    void RegisterScriptBinding(IScriptBinding* ScriptBinding);
    void UnregisterScriptBinding(IScriptBinding* ScriptBinding);
    void SetModuleSource(oly_common::String ModuleUrl, oly_common::String Source);
    void AddModuleUrlAlias(const oly_common::String& ModuleUrl, const oly_common::String& ModuleUrlAlias);
    bool GetModuleUrlAlias(const oly_common::String& ModuleUrl, oly_common::String& OutModuleUrlAlias);
    void ClearModuleSource(oly_common::String ModuleUrl);
    oly_common::String GetModuleSource(oly_common::String ModuleUrl);
    size_t GetNumImportedModules(int64_t ContextId) const;
    const char* GetImportedModule(int64_t ContextId, size_t Index) const;
    OLY_END_IGNORE

private:
    ScriptSystem();

    class ScriptRuntime* TheScriptRuntime;
};

} // namespace oly_systems
