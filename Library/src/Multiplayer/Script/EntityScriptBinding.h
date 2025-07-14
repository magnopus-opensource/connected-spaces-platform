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

#include "CSP/Systems/Script/ScriptSystem.h"
#include "Debug/Logging.h"
#include "quickjspp.hpp"

namespace csp::systems
{
class ScriptSystem;
}

namespace csp::multiplayer
{

class SpaceEntitySystem;

class SpaceScriptInterface;

class EntityScriptBinding : public csp::systems::IScriptBinding
{
public:
    EntityScriptBinding(SpaceEntitySystem* InEntitySystem);
    EntityScriptBinding(SpaceEntitySystem* InEntitySystem, SpaceScriptInterface* InSpaceInterface);
    void SetSpaceScriptInterface(SpaceScriptInterface* InSpaceInterface);
    void BindLocalScriptRoot(qjs::Context* Context, qjs::Context::Module* Module, SpaceScriptInterface* SpaceInterface = nullptr);
    virtual void Bind(int64_t ContextId, class csp::systems::ScriptSystem* ScriptSystem) override;
    void BindWithSpaceInterface(int64_t ContextId, class csp::systems::ScriptSystem* ScriptSystem, SpaceScriptInterface* SpaceInterface);
    

    static EntityScriptBinding* BindEntitySystem(SpaceEntitySystem* InEntitySystem);
    static void RemoveBinding(EntityScriptBinding* InEntityBinding);

private:
    SpaceEntitySystem* EntitySystem;
    qjs::Context* Context;
    SpaceScriptInterface* SpaceInterface;
};

} // namespace csp::multiplayer
