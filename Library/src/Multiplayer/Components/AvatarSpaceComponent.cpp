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

#include "CSP/Multiplayer/ComponentSchema.h"

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::AvatarData),
    "Avatar",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::AvatarId),
            "avatarId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::UserId),
            "userId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::State),
            "state",
            static_cast<int64_t>(AvatarState::Idle),
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::AvatarMeshIndex_DEPRECATED),
            {}, // not exposed to scripting
            static_cast<int64_t>(-1),
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::AgoraUserId),
            "agoraUserId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::CustomAvatarUrl_DEPRECATED),
            {}, // not exposed to scripting
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::IsHandIKEnabled),
            "isHandIKEnabled",
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::TargetHandIKTargetLocation),
            "targetHandIKTargetLocation",
            csp::common::Vector3 { 0.0f, 0.0f, 0.0f },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::HandRotation),
            "handRotation",
            csp::common::Vector4 { 0.0f, 0.0f, 0.0f, 1.0f },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::HeadRotation),
            "headRotation",
            csp::common::Vector4 { 0.0f, 0.0f, 0.0f, 1.0f },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::WalkRunBlendPercentage),
            "walkRunBlendPercentage",
            0.0f,
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::TorsoTwistAlpha),
            "torsoTwistAlpha",
            0.0f,
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::AvatarPlayMode),
            "avatarPlayMode",
            static_cast<int64_t>(AvatarPlayMode::Default),
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::MovementDirection),
            "movementDirection",
            csp::common::Vector3 { 0.0f, 0.0f, 0.0f },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::LocomotionModel),
            "locomotionModel",
            static_cast<int64_t>(LocomotionModel::Grounded),
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::IsVisible),
            "isVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::IsARVisible),
            "isARVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::AvatarUrl),
            "avatarUrl",
            "",
        },
    },
};

const ComponentSchema& AvatarSpaceComponent::GetSchema() { return Schema; }

AvatarSpaceComponent::AvatarSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
}

const csp::common::String& AvatarSpaceComponent::GetAvatarId() const
{
    return GetStringProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarId));
}

void AvatarSpaceComponent::SetAvatarId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarId), value);
}

const csp::common::String& AvatarSpaceComponent::GetUserId() const
{
    return GetStringProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::UserId));
}

void AvatarSpaceComponent::SetUserId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::UserId), value);
}

AvatarState AvatarSpaceComponent::GetState() const
{
    return static_cast<AvatarState>(GetIntegerProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::State)));
}

void AvatarSpaceComponent::SetState(AvatarState value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::State), static_cast<int64_t>(value));
}

AvatarPlayMode AvatarSpaceComponent::GetAvatarPlayMode() const
{
    return static_cast<AvatarPlayMode>(GetIntegerProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarPlayMode)));
}

void AvatarSpaceComponent::SetAvatarPlayMode(AvatarPlayMode value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarPlayMode), static_cast<int64_t>(value));
}

const csp::common::String& AvatarSpaceComponent::GetAgoraUserId() const
{
    return GetStringProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AgoraUserId));
}

void AvatarSpaceComponent::SetAgoraUserId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AgoraUserId), value);
}

bool AvatarSpaceComponent::GetIsHandIKEnabled() const
{
    return GetBooleanProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsHandIKEnabled));
}

void AvatarSpaceComponent::SetIsHandIKEnabled(bool value) { SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsHandIKEnabled), value); }

const csp::common::Vector3& AvatarSpaceComponent::GetTargetHandIKTargetLocation() const
{
    return GetVector3Property(static_cast<uint32_t>(AvatarComponentPropertyKeys::TargetHandIKTargetLocation));
}

void AvatarSpaceComponent::SetTargetHandIKTargetLocation(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::TargetHandIKTargetLocation), value);
}

const csp::common::Vector4& AvatarSpaceComponent::GetHandRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(AvatarComponentPropertyKeys::HandRotation));
}

void AvatarSpaceComponent::SetHandRotation(const csp::common::Vector4& value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::HandRotation), value);
}

const csp::common::Vector4& AvatarSpaceComponent::GetHeadRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(AvatarComponentPropertyKeys::HeadRotation));
}

void AvatarSpaceComponent::SetHeadRotation(const csp::common::Vector4& value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::HeadRotation), value);
}

float AvatarSpaceComponent::GetWalkRunBlendPercentage() const
{
    return GetFloatProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::WalkRunBlendPercentage));
}

void AvatarSpaceComponent::SetWalkRunBlendPercentage(float value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::WalkRunBlendPercentage), value);
}

float AvatarSpaceComponent::GetTorsoTwistAlpha() const
{
    return GetFloatProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::TorsoTwistAlpha));
}

void AvatarSpaceComponent::SetTorsoTwistAlpha(float value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::TorsoTwistAlpha), value);
}

const csp::common::Vector3& csp::multiplayer::AvatarSpaceComponent::GetMovementDirection() const
{
    return GetVector3Property(static_cast<uint32_t>(AvatarComponentPropertyKeys::MovementDirection));
}

void csp::multiplayer::AvatarSpaceComponent::SetMovementDirection(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::MovementDirection), value);
}

LocomotionModel AvatarSpaceComponent::GetLocomotionModel() const
{
    return static_cast<LocomotionModel>(GetIntegerProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::LocomotionModel)));
}

void AvatarSpaceComponent::SetLocomotionModel(LocomotionModel value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::LocomotionModel), static_cast<int64_t>(value));
}

bool AvatarSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsVisible)); }

void AvatarSpaceComponent::SetIsVisible(bool inValue) { SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsVisible), inValue); }

bool AvatarSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsARVisible)); }

void AvatarSpaceComponent::SetIsARVisible(bool inValue) { SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsARVisible), inValue); }

bool AvatarSpaceComponent::GetIsVirtualVisible() const
{
    return GetBooleanProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsVirtualVisible));
}

void AvatarSpaceComponent::SetIsVirtualVisible(bool inValue)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsVirtualVisible), inValue);
}

const csp::common::String& AvatarSpaceComponent::GetAvatarUrl() const
{
    return GetStringProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarUrl));
}

void AvatarSpaceComponent::SetAvatarUrl(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarUrl), value);
}

} // namespace csp::multiplayer
