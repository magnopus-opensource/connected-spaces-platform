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

/// @file AIChatbotComponent.h
/// @brief Definitions and support for AI chatbot components.

#include "CSP/Multiplayer/Components/AIChatbotComponent.h"

namespace csp::multiplayer
{

csp::multiplayer::AIChatbotSpaceComponent::AIChatbotSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(ComponentType::AIChatbot, LogSystem, Parent)
{
    Properties[static_cast<uint32_t>(AIChatbotPropertyKeys::Position)] = csp::common::Vector3::Zero();
    Properties[static_cast<uint32_t>(AIChatbotPropertyKeys::Rotation)] = csp::common::Vector4::Identity();
    Properties[static_cast<uint32_t>(AIChatbotPropertyKeys::Scale)] = csp::common::Vector3::One();
    Properties[static_cast<uint32_t>(AIChatbotPropertyKeys::ContextAssetId)] = "";
    Properties[static_cast<uint32_t>(AIChatbotPropertyKeys::GuardrailAssetId)] = "";
    Properties[static_cast<uint32_t>(AIChatbotPropertyKeys::VisualState)] = static_cast<int64_t>(0);
}

const csp::common::String& AIChatbotSpaceComponent::GetContextAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(AIChatbotPropertyKeys::ContextAssetId));
}

void AIChatbotSpaceComponent::SetContextAssetId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(AIChatbotPropertyKeys::ContextAssetId), Value);
}

const csp::common::String& AIChatbotSpaceComponent::GetGuardrailAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(AIChatbotPropertyKeys::GuardrailAssetId));
}

void AIChatbotSpaceComponent::SetGuardrailAssetId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(AIChatbotPropertyKeys::GuardrailAssetId), Value);
}

AIChatbotVisualState AIChatbotSpaceComponent::GetVisualState() const
{
    return static_cast<AIChatbotVisualState>(GetIntegerProperty((uint32_t)AIChatbotPropertyKeys::VisualState));
}

void AIChatbotSpaceComponent::SetVisualState(AIChatbotVisualState Value)
{
    SetProperty(static_cast<uint32_t>(AIChatbotPropertyKeys::VisualState), static_cast<int64_t>(Value));
}

/* ITransformComponent */

const csp::common::Vector3& AIChatbotSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(AIChatbotPropertyKeys::Position));
}

void AIChatbotSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(AIChatbotPropertyKeys::Position), Value);
}

const csp::common::Vector4& AIChatbotSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(AIChatbotPropertyKeys::Rotation));
}

void AIChatbotSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
    SetProperty(static_cast<uint32_t>(AIChatbotPropertyKeys::Rotation), Value);
}

const csp::common::Vector3& AIChatbotSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(AIChatbotPropertyKeys::Scale));
}

void AIChatbotSpaceComponent::SetScale(const csp::common::Vector3& Value) { SetProperty(static_cast<uint32_t>(AIChatbotPropertyKeys::Scale), Value); }

SpaceTransform AIChatbotSpaceComponent::GetTransform() const
{
    SpaceTransform Transform;
    Transform.Position = GetPosition();
    Transform.Rotation = GetRotation();
    Transform.Scale = GetScale();

    return Transform;
}

void AIChatbotSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
    SetPosition(InValue.Position);
    SetRotation(InValue.Rotation);
    SetScale(InValue.Scale);
}
} // namespace csp::multiplayer
