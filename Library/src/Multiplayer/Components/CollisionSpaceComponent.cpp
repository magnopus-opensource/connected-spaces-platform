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

#include "CSP/Multiplayer/Components/CollisionSpaceComponent.h"

#include "CSP/Multiplayer/ComponentSchema.h"
#include "Multiplayer/Script/ComponentBinding/CollisionSpaceComponentScriptInterface.h"

namespace
{

constexpr const float DefaultSphereRadius = 0.5f;
constexpr const float DefaultCapsuleHalfWidth = 0.5f;
constexpr const float DefaultCapsuleHalfHeight = 1.f;

} // namespace

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::Collision),
    {}, // not exposed to scripting
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::Position),
            {}, // not exposed to scripting
            csp::common::Vector3 { 0, 0, 0 },
        },
        {
            static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::Rotation),
            {}, // not exposed to scripting
            csp::common::Vector4 { 0, 0, 0, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::Scale),
            {}, // not exposed to scripting
            csp::common::Vector3 { 1, 1, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::CollisionShape),
            {}, // not exposed to scripting
            static_cast<int64_t>(CollisionShape::Box),
        },
        {
            static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::CollisionMode),
            {}, // not exposed to scripting
            static_cast<int64_t>(CollisionMode::Collision),
        },
        {
            static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::CollisionAssetId),
            {}, // not exposed to scripting
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::AssetCollectionId),
            {}, // not exposed to scripting
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::ThirdPartyComponentRef),
            {}, // not exposed to scripting
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::IsEnabled),
            {}, // not exposed to scripting
            true,
        },
    },
};

const ComponentSchema& CollisionSpaceComponent::GetSchema() { return Schema; }

CollisionSpaceComponent::CollisionSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
    SetScriptInterface(new CollisionSpaceComponentScriptInterface(this));
}

/* ITransformComponent */

const csp::common::Vector3& CollisionSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(CollisionPropertyKeys::Position));
}

void CollisionSpaceComponent::SetPosition(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::Position), value);
}

const csp::common::Vector4& CollisionSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(CollisionPropertyKeys::Rotation));
}

void CollisionSpaceComponent::SetRotation(const csp::common::Vector4& value)
{
    SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::Rotation), value);
}

const csp::common::Vector3& CollisionSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(CollisionPropertyKeys::Scale));
}

void CollisionSpaceComponent::SetScale(const csp::common::Vector3& value) { SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::Scale), value); }

SpaceTransform CollisionSpaceComponent::GetTransform() const
{
    SpaceTransform transform;
    transform.Position = GetPosition();
    transform.Rotation = GetRotation();
    transform.Scale = GetScale();

    return transform;
}

void CollisionSpaceComponent::SetTransform(const SpaceTransform& inValue)
{
    SetPosition(inValue.Position);
    SetRotation(inValue.Rotation);
    SetScale(inValue.Scale);
}

CollisionShape CollisionSpaceComponent::GetCollisionShape() const
{
    return static_cast<CollisionShape>(GetIntegerProperty(static_cast<uint32_t>(CollisionPropertyKeys::CollisionShape)));
}

void CollisionSpaceComponent::SetCollisionShape(CollisionShape collisionShape)
{
    SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::CollisionShape), static_cast<int64_t>(collisionShape));
}

CollisionMode CollisionSpaceComponent::GetCollisionMode() const
{
    return static_cast<CollisionMode>(GetIntegerProperty(static_cast<uint32_t>(CollisionPropertyKeys::CollisionMode)));
}

void CollisionSpaceComponent::SetCollisionMode(CollisionMode collisionMode)
{
    SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::CollisionMode), static_cast<int64_t>(collisionMode));
}

const csp::common::String& CollisionSpaceComponent::GetCollisionAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(CollisionPropertyKeys::CollisionAssetId));
}

void CollisionSpaceComponent::SetCollisionAssetId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::CollisionAssetId), value);
}

const csp::common::String& CollisionSpaceComponent::GetAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(CollisionPropertyKeys::AssetCollectionId));
}

void CollisionSpaceComponent::SetAssetCollectionId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::AssetCollectionId), value);
}

const csp::common::Vector3 CollisionSpaceComponent::GetUnscaledBoundingBoxMin() { return csp::common::Vector3(-0.5f, -0.5f, -0.5f); }

const csp::common::Vector3 CollisionSpaceComponent::GetUnscaledBoundingBoxMax() { return csp::common::Vector3(0.5f, 0.5f, 0.5f); }

const csp::common::Vector3 CollisionSpaceComponent::GetScaledBoundingBoxMin()
{
    return csp::common::Vector3(-0.5f * GetScale().X, -0.5f * GetScale().Y, -0.5f * GetScale().Z);
}

const csp::common::Vector3 CollisionSpaceComponent::GetScaledBoundingBoxMax()
{
    return csp::common::Vector3(0.5f * GetScale().X, 0.5f * GetScale().Y, 0.5f * GetScale().Z);
}

float CollisionSpaceComponent::GetDefaultSphereRadius() { return DefaultSphereRadius; }

float CollisionSpaceComponent::GetDefaultCapsuleHalfWidth() { return DefaultCapsuleHalfWidth; }

float CollisionSpaceComponent::GetDefaultCapsuleHalfHeight() { return DefaultCapsuleHalfHeight; }

const csp::common::String& CollisionSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(CollisionPropertyKeys::ThirdPartyComponentRef));
}

void CollisionSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& inValue)
{
    SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::ThirdPartyComponentRef), inValue);
}

bool CollisionSpaceComponent::GetIsEnabled() const { return GetBooleanProperty(static_cast<uint32_t>(CollisionPropertyKeys::IsEnabled)); }

void CollisionSpaceComponent::SetIsEnabled(bool value) { SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::IsEnabled), value); }

} // namespace csp::multiplayer
