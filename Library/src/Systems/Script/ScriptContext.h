/*
 * Copyright 2023 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "CSP/Common/String.h"
#include "Debug/Logging.h"
#include "quickjspp.hpp"

#include <map>
#include <string>

namespace csp::systems
{

struct ScriptModule
{
    std::string ModuleName;
    qjs::Context::Module* Module;
};

class ScriptContext
{
    friend class ScriptSystem;

public:
    ScriptContext(ScriptSystem* InScriptSystem, qjs::Runtime* Runtime, uint64_t InContextId);
    ~ScriptContext();

    void AddModule(const csp::common::String& ModuleName);
    ScriptModule* GetModule(const csp::common::String& ModuleName);

    uint64_t GetId() const;

    bool ExistsInContext(const csp::common::String& ObjectName);

    size_t GetNumImportedModules() const;
    const char* GetImportedModule(size_t Index) const;

    void Reset();

private:
    void Initialise();
    void Shutdown();

    void AddImport(const csp::common::String& Url);

    using ModuleMap = std::map<std::string, ScriptModule*>;
    using ImportedModules = std::vector<std::string>;

    uint64_t ContextId;
    ScriptSystem* TheScriptSystem;

    qjs::Context* Context;
    qjs::Runtime* Runtime;
    ModuleMap Modules;
    ImportedModules Imports;
};

} // namespace csp::systems
