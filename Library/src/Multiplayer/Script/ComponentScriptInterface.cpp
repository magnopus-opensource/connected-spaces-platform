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

#include <memory>

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
        if (LocalScope)
        {
            Component->GetParent()->ApplyLocalPatch();
        }
        else
        {
            Component->GetParent()->QueueUpdate();
        }
    }
}

SpaceEntity* ComponentScriptInterface::GetParentEntity() const
{
    return Component != nullptr ? Component->GetParent() : nullptr;
}

uint64_t ComponentScriptInterface::GetParentEntityId() const
{
    const auto* Parent = GetParentEntity();
    return Parent != nullptr ? Parent->GetId() : 0;
}

int32_t ComponentScriptInterface::GetTypeIndexWithinParent() const
{
    if (Component == nullptr)
    {
        return -1;
    }

    const auto* Parent = Component->GetParent();
    if (Parent == nullptr)
    {
        return -1;
    }

    const auto* Components = Parent->GetComponents();
    if (Components == nullptr)
    {
        return -1;
    }

    std::unique_ptr<csp::common::Array<uint16_t>> Keys(const_cast<csp::common::Array<uint16_t>*>(Components->Keys()));
    int32_t MatchingIndex = 0;
    for (size_t Index = 0; Index < Keys->Size(); ++Index)
    {
        auto* CurrentComponent = (*Components)[(*Keys)[Index]];
        if (CurrentComponent == nullptr || CurrentComponent->GetComponentType() != Component->GetComponentType())
        {
            continue;
        }

        if (CurrentComponent == Component)
        {
            return MatchingIndex;
        }

        ++MatchingIndex;
    }

    return -1;
}

} // namespace csp::multiplayer
