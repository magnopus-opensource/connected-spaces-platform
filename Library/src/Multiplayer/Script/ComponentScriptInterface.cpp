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
#include "Multiplayer/Script/ComponentScriptInterface.h"

#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Debug/Logging.h"

using namespace csp::systems;

namespace csp::multiplayer
{

ComponentScriptInterface::ComponentScriptInterface(ComponentBase* InComponenty)
    : Component(InComponenty)
{
}

void ComponentScriptInterface::SubscribeToPropertyChange(int32_t PropertyKey, std::string Message)
{
    if (Component)
    {
        Component->SubscribeToPropertyChange(PropertyKey, Message.c_str());
    }
}

void ComponentScriptInterface::InvokeAction(std::string ActionId, std::string ActionParams)
{
    if (Component)
    {
        Component->InvokeAction(ActionId.c_str(), ActionParams.c_str());
    }
}

int64_t ComponentScriptInterface::GetComponentId() const
{
    if (Component)
    {
        return Component->GetId();
    }

    return INVALID_COMPONENT_ID;
}

int64_t ComponentScriptInterface::GetComponentType() const
{
    if (Component)
    {
        return (int64_t)Component->GetComponentType();
    }

    return (int64_t)ComponentType::Invalid;
}

void ComponentScriptInterface::SetComponentName(std::string name)
{
    if (Component)
    {
        Component->SetComponentName(name.c_str());
    }
}

std::string ComponentScriptInterface::GetComponentName() const
{
    if (Component)
    {
        return Component->GetComponentName().c_str();
    }

    return "";
}

void ComponentScriptInterface::SendPropertyUpdate()
{
    if (Component)
    {
        Component->GetParent()->MarkForUpdate();
    }
}

} // namespace csp::multiplayer
