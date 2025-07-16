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

#include "CSP/Common/Interfaces/IScriptBinding.h"
#include "Debug/Logging.h"
#include "quickjspp.hpp"

namespace csp::common
{
class LogSystem;
class IJSScriptRunner;
}

namespace csp::multiplayer
{

class SpaceEntitySystem;

class SpaceScriptInterface;

class EntityScriptBinding : public csp::common::IScriptBinding
{
public:
    EntityScriptBinding(SpaceEntitySystem* InEntitySystem);
    EntityScriptBinding(SpaceEntitySystem* InEntitySystem, SpaceScriptInterface* InSpaceInterface, csp::common::LogSystem& LogSystem);
    void SetSpaceScriptInterface(SpaceScriptInterface* InSpaceInterface);
    void Bind(int64_t ContextId, csp::common::IJSScriptRunner& ScriptRunner) override;
    void BindLocalScriptRoot(qjs::Context* Context, qjs::Context::Module* Module, SpaceScriptInterface* SpaceInterface = nullptr);
    void BindWithSpaceInterface(int64_t ContextId, csp::common::IJSScriptRunner& ScriptRunner, SpaceScriptInterface* SpaceInterface);


    static EntityScriptBinding* BindEntitySystem(
        SpaceEntitySystem* InEntitySystem, csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& ScriptRunner);
    static void RemoveBinding(EntityScriptBinding* InEntityBinding, csp::common::IJSScriptRunner& ScriptRunner);

private:
    SpaceEntitySystem* EntitySystem;
    qjs::Context* Context;
    SpaceScriptInterface* SpaceInterface;
    csp::common::LogSystem& LogSystem;
};

} // namespace csp::multiplayer
