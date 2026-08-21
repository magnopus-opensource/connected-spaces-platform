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
#include "Multiplayer/ComponentSchema.h"

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::AvatarData),
    "Avatar",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::AvatarId),
            "avatarId",
            PlainValue<std::string> { "" },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::UserId),
            "userId",
            PlainValue<std::string> { "" },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::State),
            "state",
            EnumeratedValue<int64_t> {
                static_cast<int64_t>(AvatarState::Idle),
                {
                    SchemaOption<int64_t> { "Idle", static_cast<int64_t>(AvatarState::Idle) },
                    SchemaOption<int64_t> { "Walking", static_cast<int64_t>(AvatarState::Walking) },
                    SchemaOption<int64_t> { "Running", static_cast<int64_t>(AvatarState::Running) },
                    SchemaOption<int64_t> { "Flying", static_cast<int64_t>(AvatarState::Flying) },
                    SchemaOption<int64_t> { "Jumping", static_cast<int64_t>(AvatarState::Jumping) },
                    SchemaOption<int64_t> { "Falling", static_cast<int64_t>(AvatarState::Falling) },
                },
            },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::AvatarMeshIndex_DEPRECATED),
            "avatarMeshIndex_DEPRECATED",
            PlainValue<int64_t> { static_cast<int64_t>(-1) },
            /*.IsScriptable =*/false,
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::AgoraUserId),
            "agoraUserId",
            PlainValue<std::string> { "" },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::CustomAvatarUrl_DEPRECATED),
            "customAvatarUrl_DEPRECATED",
            PlainValue<std::string> { "" },
            /*.IsScriptable =*/false,
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::IsHandIKEnabled),
            "isHandIKEnabled",
            PlainValue<bool> { false },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::TargetHandIKTargetLocation),
            "targetHandIKTargetLocation",
            PlainValue<csp::common::Vector3> { csp::common::Vector3 { 0.0f, 0.0f, 0.0f } },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::HandRotation),
            "handRotation",
            PlainValue<csp::common::Vector4> { csp::common::Vector4 { 0.0f, 0.0f, 0.0f, 1.0f } },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::HeadRotation),
            "headRotation",
            PlainValue<csp::common::Vector4> { csp::common::Vector4 { 0.0f, 0.0f, 0.0f, 1.0f } },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::WalkRunBlendPercentage),
            "walkRunBlendPercentage",
            PlainValue<float> { 0.0f },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::TorsoTwistAlpha),
            "torsoTwistAlpha",
            PlainValue<float> { 0.0f },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::AvatarPlayMode),
            "avatarPlayMode",
            EnumeratedValue<int64_t> {
                static_cast<int64_t>(AvatarPlayMode::Default),
                {
                    SchemaOption<int64_t> { "Default", static_cast<int64_t>(AvatarPlayMode::Default) },
                    SchemaOption<int64_t> { "AR", static_cast<int64_t>(AvatarPlayMode::AR) },
                    SchemaOption<int64_t> { "VR", static_cast<int64_t>(AvatarPlayMode::VR) },
                    SchemaOption<int64_t> { "Creator", static_cast<int64_t>(AvatarPlayMode::Creator) },
                },
            },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::MovementDirection),
            "movementDirection",
            PlainValue<csp::common::Vector3> { csp::common::Vector3 { 0.0f, 0.0f, 0.0f } },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::LocomotionModel),
            "locomotionModel",
            EnumeratedValue<int64_t> {
                static_cast<int64_t>(LocomotionModel::Grounded),
                {
                    SchemaOption<int64_t> { "Grounded", static_cast<int64_t>(LocomotionModel::Grounded) },
                    SchemaOption<int64_t> { "FreeCamera", static_cast<int64_t>(LocomotionModel::FreeCamera) },
                },
            },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::IsVisible),
            "isVisible",
            PlainValue<bool> { true },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::IsARVisible),
            "isARVisible",
            PlainValue<bool> { true },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            PlainValue<bool> { true },
        },
        {
            static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::AvatarUrl),
            "avatarUrl",
            PlainValue<std::string> { "" },
        },
    },
};

const ComponentSchema& AvatarSpaceComponent::GetSchema() { return Schema; }

AvatarSpaceComponent::AvatarSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : AvatarSpaceComponent(Schema, LogSystem, Parent)
{
}

std::unique_ptr<AvatarSpaceComponent> AvatarSpaceComponent::TryMake(
    const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
{
    if (!IsCompatible(AvatarSpaceComponent::GetSchema(), InSchema))
    {
        return nullptr;
    }

    return std::unique_ptr<AvatarSpaceComponent>(new AvatarSpaceComponent(InSchema, LogSystem, Parent));
}

AvatarSpaceComponent::AvatarSpaceComponent(const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(InSchema, LogSystem, Parent)
{
}

const csp::common::String& AvatarSpaceComponent::GetAvatarId() const
{
    return GetStringProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarId));
}

void AvatarSpaceComponent::SetAvatarId(const csp::common::String& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarId), Value);
}

const csp::common::String& AvatarSpaceComponent::GetUserId() const
{
    return GetStringProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::UserId));
}

void AvatarSpaceComponent::SetUserId(const csp::common::String& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::UserId), Value);
}

AvatarState AvatarSpaceComponent::GetState() const
{
    return static_cast<AvatarState>(GetIntegerProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::State)));
}

void AvatarSpaceComponent::SetState(AvatarState Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::State), static_cast<int64_t>(Value));
}

AvatarPlayMode AvatarSpaceComponent::GetAvatarPlayMode() const
{
    return static_cast<AvatarPlayMode>(GetIntegerProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarPlayMode)));
}

void AvatarSpaceComponent::SetAvatarPlayMode(AvatarPlayMode Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarPlayMode), static_cast<int64_t>(Value));
}

const csp::common::String& AvatarSpaceComponent::GetAgoraUserId() const
{
    return GetStringProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AgoraUserId));
}

void AvatarSpaceComponent::SetAgoraUserId(const csp::common::String& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::AgoraUserId), Value);
}

bool AvatarSpaceComponent::GetIsHandIKEnabled() const
{
    return GetBooleanProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsHandIKEnabled));
}

void AvatarSpaceComponent::SetIsHandIKEnabled(bool Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsHandIKEnabled), Value);
}

const csp::common::Vector3& AvatarSpaceComponent::GetTargetHandIKTargetLocation() const
{
    return GetVector3Property(static_cast<uint32_t>(AvatarComponentPropertyKeys::TargetHandIKTargetLocation));
}

void AvatarSpaceComponent::SetTargetHandIKTargetLocation(const csp::common::Vector3& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::TargetHandIKTargetLocation), Value);
}

const csp::common::Vector4& AvatarSpaceComponent::GetHandRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(AvatarComponentPropertyKeys::HandRotation));
}

void AvatarSpaceComponent::SetHandRotation(const csp::common::Vector4& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::HandRotation), Value);
}

const csp::common::Vector4& AvatarSpaceComponent::GetHeadRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(AvatarComponentPropertyKeys::HeadRotation));
}

void AvatarSpaceComponent::SetHeadRotation(const csp::common::Vector4& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::HeadRotation), Value);
}

float AvatarSpaceComponent::GetWalkRunBlendPercentage() const
{
    return GetFloatProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::WalkRunBlendPercentage));
}

void AvatarSpaceComponent::SetWalkRunBlendPercentage(float Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::WalkRunBlendPercentage), Value);
}

float AvatarSpaceComponent::GetTorsoTwistAlpha() const
{
    return GetFloatProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::TorsoTwistAlpha));
}

void AvatarSpaceComponent::SetTorsoTwistAlpha(float Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::TorsoTwistAlpha), Value);
}

const csp::common::Vector3& csp::multiplayer::AvatarSpaceComponent::GetMovementDirection() const
{
    return GetVector3Property(static_cast<uint32_t>(AvatarComponentPropertyKeys::MovementDirection));
}

void csp::multiplayer::AvatarSpaceComponent::SetMovementDirection(const csp::common::Vector3& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::MovementDirection), Value);
}

LocomotionModel AvatarSpaceComponent::GetLocomotionModel() const
{
    return static_cast<LocomotionModel>(GetIntegerProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::LocomotionModel)));
}

void AvatarSpaceComponent::SetLocomotionModel(LocomotionModel Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::LocomotionModel), static_cast<int64_t>(Value));
}

bool AvatarSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsVisible)); }

void AvatarSpaceComponent::SetIsVisible(bool InValue) { SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsVisible), InValue); }

bool AvatarSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsARVisible)); }

void AvatarSpaceComponent::SetIsARVisible(bool InValue)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsARVisible), InValue);
}

bool AvatarSpaceComponent::GetIsVirtualVisible() const
{
    return GetBooleanProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsVirtualVisible));
}

void AvatarSpaceComponent::SetIsVirtualVisible(bool InValue)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsVirtualVisible), InValue);
}

const csp::common::String& AvatarSpaceComponent::GetAvatarUrl() const
{
    return GetStringProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarUrl));
}

void AvatarSpaceComponent::SetAvatarUrl(const csp::common::String& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarUrl), Value);
}

} // namespace csp::multiplayer
