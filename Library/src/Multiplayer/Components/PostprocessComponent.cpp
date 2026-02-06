/*
 * Copyright 2026 Magnopus LLC

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

#include "CSP/Multiplayer/Components/PostprocessComponent.h"

#include "Multiplayer/Script/ComponentBinding/PostprocessComponentScriptInterface.h"

namespace csp::multiplayer
{

PostprocessSpaceComponent::PostprocessSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(ComponentType::Reflection, LogSystem, Parent)
{
    Properties[static_cast<uint32_t>(PostprocessPropertyKeys::Position)] = csp::common::Vector3::Zero();
    Properties[static_cast<uint32_t>(PostprocessPropertyKeys::Rotation)] = csp::common::Vector4 { 0.0f, 0.0f, 0.0f, 1.0f };
    Properties[static_cast<uint32_t>(PostprocessPropertyKeys::Scale)] = csp::common::Vector3::One();
    Properties[static_cast<uint32_t>(PostprocessPropertyKeys::Exposure)] = 100.0f;
    Properties[static_cast<uint32_t>(PostprocessPropertyKeys::IsUnbound)] = true;

    SetScriptInterface(new PostprocessSpaceComponentScriptInterface(this));
}

/* IPositionComponent */

const csp::common::Vector3& PostprocessSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(PostprocessPropertyKeys::Position));
}

void PostprocessSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(PostprocessPropertyKeys::Position), Value);
}

/* IRotationComponent */

const csp::common::Vector4& PostprocessSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(PostprocessPropertyKeys::Rotation));
}

void PostprocessSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
    SetProperty(static_cast<uint32_t>(PostprocessPropertyKeys::Rotation), Value);
}

/* IScaleComponent */

const csp::common::Vector3& PostprocessSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(PostprocessPropertyKeys::Scale));
}

void PostprocessSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(PostprocessPropertyKeys::Scale), Value);
}

const float PostprocessSpaceComponent::GetExposure() const
{
    return GetFloatProperty(static_cast<uint32_t>(PostprocessPropertyKeys::Exposure));
}

void PostprocessSpaceComponent::SetExposure(float Value)
{
    SetProperty(static_cast<uint32_t>(PostprocessPropertyKeys::Exposure), Value);
}

bool PostprocessSpaceComponent::GetIsUnbound() const
{
    return GetBooleanProperty(static_cast<uint32_t>(PostprocessPropertyKeys::IsUnbound));
}

void PostprocessSpaceComponent::SetIsUnbound(bool Value)
{
    SetProperty(static_cast<uint32_t>(PostprocessPropertyKeys::IsUnbound), Value);
}

} // namespace csp::multiplayer
