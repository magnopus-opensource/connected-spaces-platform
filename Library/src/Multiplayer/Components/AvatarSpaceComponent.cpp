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
#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"

#include "Multiplayer/Component/Schema.h"
#include "Multiplayer/Script/ComponentBinding/AvatarSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

const auto Schema = ComponentBase::ComponentSchema {
    ComponentType::AvatarData,
    std::vector<ComponentBase::ComponentSchema::Property> {
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::AvatarId),
            "",
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::UserId),
            "",
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::State),
            static_cast<int64_t>(AvatarState::Idle),
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::AvatarMeshIndex_DEPRECATED),
            static_cast<int64_t>(-1),
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::AgoraUserId),
            "",
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::CustomAvatarUrl_DEPRECATED),
            "",
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::IsHandIKEnabled),
            false,
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::TargetHandIKTargetLocation),
            csp::common::Vector3 { 0.0f, 0.0f, 0.0f },
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::HandRotation),
            csp::common::Vector4 { 0.0f, 0.0f, 0.0f, 1.0f },
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::HeadRotation),
            csp::common::Vector4 { 0.0f, 0.0f, 0.0f, 1.0f },
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::WalkRunBlendPercentage),
            0.0f,
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::TorsoTwistAlpha),
            0.0f,
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::AvatarPlayMode),
            static_cast<int64_t>(AvatarPlayMode::Default),
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::MovementDirection),
            csp::common::Vector3 { 0.0f, 0.0f, 0.0f },
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::LocomotionModel),
            static_cast<int64_t>(LocomotionModel::Grounded),
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::IsVisible),
            true,
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::IsARVisible),
            true,
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::IsVirtualVisible),
            true,
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::AvatarUrl),
            "",
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::CameraType),
            static_cast<int64_t>(CameraType::FirstAndThirdPerson),
        },
        {
            static_cast<ComponentBase::PropertyKey>(AvatarComponentPropertyKeys::IsScripted),
            false,
        },
    },
};

AvatarSpaceComponent::AvatarSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(Schema, LogSystem, Parent)
{
    SetScriptInterface(new AvatarSpaceComponentScriptInterface(this));
}

const csp::common::String& AvatarSpaceComponent::GetAvatarId() const
{
    return GetStringProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarId));
}

void AvatarSpaceComponent::SetAvatarId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarId), Value);
}

const csp::common::String& AvatarSpaceComponent::GetUserId() const
{
    return GetStringProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::UserId));
}

void AvatarSpaceComponent::SetUserId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::UserId), Value);
}

AvatarState AvatarSpaceComponent::GetState() const
{
    return static_cast<AvatarState>(GetIntegerProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::State)));
}

void AvatarSpaceComponent::SetState(AvatarState Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::State), static_cast<int64_t>(Value));
}

AvatarPlayMode AvatarSpaceComponent::GetAvatarPlayMode() const
{
    return static_cast<AvatarPlayMode>(GetIntegerProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarPlayMode)));
}

void AvatarSpaceComponent::SetAvatarPlayMode(AvatarPlayMode Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarPlayMode), static_cast<int64_t>(Value));
}

const csp::common::String& AvatarSpaceComponent::GetAgoraUserId() const
{
    return GetStringProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AgoraUserId));
}

void AvatarSpaceComponent::SetAgoraUserId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AgoraUserId), Value);
}

bool AvatarSpaceComponent::GetIsHandIKEnabled() const
{
    return GetBooleanProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsHandIKEnabled));
}

void AvatarSpaceComponent::SetIsHandIKEnabled(bool Value) { SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsHandIKEnabled), Value); }

const csp::common::Vector3& AvatarSpaceComponent::GetTargetHandIKTargetLocation() const
{
    return GetVector3Property(static_cast<uint32_t>(AvatarComponentPropertyKeys::TargetHandIKTargetLocation));
}

void AvatarSpaceComponent::SetTargetHandIKTargetLocation(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::TargetHandIKTargetLocation), Value);
}

const csp::common::Vector4& AvatarSpaceComponent::GetHandRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(AvatarComponentPropertyKeys::HandRotation));
}

void AvatarSpaceComponent::SetHandRotation(const csp::common::Vector4& Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::HandRotation), Value);
}

const csp::common::Vector4& AvatarSpaceComponent::GetHeadRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(AvatarComponentPropertyKeys::HeadRotation));
}

void AvatarSpaceComponent::SetHeadRotation(const csp::common::Vector4& Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::HeadRotation), Value);
}

float AvatarSpaceComponent::GetWalkRunBlendPercentage() const
{
    return GetFloatProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::WalkRunBlendPercentage));
}

void AvatarSpaceComponent::SetWalkRunBlendPercentage(float Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::WalkRunBlendPercentage), Value);
}

float AvatarSpaceComponent::GetTorsoTwistAlpha() const
{
    return GetFloatProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::TorsoTwistAlpha));
}

void AvatarSpaceComponent::SetTorsoTwistAlpha(float Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::TorsoTwistAlpha), Value);
}

const csp::common::Vector3& csp::multiplayer::AvatarSpaceComponent::GetMovementDirection() const
{
    return GetVector3Property(static_cast<uint32_t>(AvatarComponentPropertyKeys::MovementDirection));
}

void csp::multiplayer::AvatarSpaceComponent::SetMovementDirection(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::MovementDirection), Value);
}

LocomotionModel AvatarSpaceComponent::GetLocomotionModel() const
{
    return static_cast<LocomotionModel>(GetIntegerProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::LocomotionModel)));
}

void AvatarSpaceComponent::SetLocomotionModel(LocomotionModel Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::LocomotionModel), static_cast<int64_t>(Value));
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsScripted), Value == LocomotionModel::Scripted);
}

bool AvatarSpaceComponent::GetIsScripted() const
{
    return GetBooleanProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsScripted));
}

void AvatarSpaceComponent::SetIsScripted(bool Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsScripted), Value);
}

bool AvatarSpaceComponent::GetIsAvatarEnabled() const
{
    return GetBooleanProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsAvatarEnabled));
}

void AvatarSpaceComponent::SetIsAvatarEnabled(bool Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsAvatarEnabled), Value);
}

CameraType AvatarSpaceComponent::GetCameraType() const
{
    return static_cast<CameraType>(GetIntegerProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::CameraType)));
}

void AvatarSpaceComponent::SetCameraType(CameraType Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::CameraType), static_cast<int64_t>(Value));
}

bool AvatarSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsVisible)); }

void AvatarSpaceComponent::SetIsVisible(bool InValue) { SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsVisible), InValue); }

bool AvatarSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsARVisible)); }

void AvatarSpaceComponent::SetIsARVisible(bool InValue) { SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsARVisible), InValue); }

bool AvatarSpaceComponent::GetIsVirtualVisible() const
{
    return GetBooleanProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsVirtualVisible));
}

void AvatarSpaceComponent::SetIsVirtualVisible(bool InValue)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsVirtualVisible), InValue);
}

const csp::common::String& AvatarSpaceComponent::GetAvatarUrl() const
{
    return GetStringProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarUrl));
}

void AvatarSpaceComponent::SetAvatarUrl(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarUrl), Value);
}

} // namespace csp::multiplayer
