/*
 * Copyright 2025 Magnopus LLC

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

#include "CSP/Multiplayer/Components/AIChatbotComponent.h"

#include "Multiplayer/ComponentSchemaRegistry.h"

namespace csp::multiplayer
{

csp::multiplayer::AIChatbotSpaceComponent::AIChatbotSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : AIChatbotSpaceComponent(GetAIChatbotSchema(), LogSystem, Parent)
{
}

std::unique_ptr<AIChatbotSpaceComponent> AIChatbotSpaceComponent::TryMake(
    const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
{
    if (!IsCompatible(GetAIChatbotSchema(), InSchema))
    {
        return nullptr;
    }

    return std::unique_ptr<AIChatbotSpaceComponent>(new AIChatbotSpaceComponent(InSchema, LogSystem, Parent));
}

AIChatbotSpaceComponent::AIChatbotSpaceComponent(const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(InSchema, LogSystem, Parent)
{
}

const csp::common::String& AIChatbotSpaceComponent::GetVoice() const
{
    return GetStringProperty(static_cast<uint32_t>(AIChatbotPropertyKeys::Voice));
}

void AIChatbotSpaceComponent::SetVoice(const csp::common::String& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AIChatbotPropertyKeys::Voice), Value);
}

const csp::common::String& AIChatbotSpaceComponent::GetGuardrailAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(AIChatbotPropertyKeys::GuardrailAssetCollectionId));
}

void AIChatbotSpaceComponent::SetGuardrailAssetCollectionId(const csp::common::String& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AIChatbotPropertyKeys::GuardrailAssetCollectionId), Value);
}

AIChatbotVisualState AIChatbotSpaceComponent::GetVisualState() const
{
    return static_cast<AIChatbotVisualState>(GetIntegerProperty((uint32_t)AIChatbotPropertyKeys::VisualState));
}

void AIChatbotSpaceComponent::SetVisualState(AIChatbotVisualState Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AIChatbotPropertyKeys::VisualState), static_cast<int64_t>(Value));
}

/* ITransformComponent */

const csp::common::Vector3& AIChatbotSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(AIChatbotPropertyKeys::Position));
}

void AIChatbotSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AIChatbotPropertyKeys::Position), Value);
}

} // namespace csp::multiplayer
