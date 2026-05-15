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

#include "CSP/Multiplayer/Components/AnimatedModelSpaceComponent.h"

#include "CSP/Multiplayer/ComponentSchema.h"

#include <memory>

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::AnimatedModel),
    "AnimatedModel",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::ExternalResourceAssetId),
            "externalResourceAssetId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::ExternalResourceAssetCollectionId),
            "externalResourceAssetCollectionId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::MaterialOverrides),
            {}, // not exposed to scripting
            csp::common::Map<csp::common::String, csp::common::ReplicatedValue>(),
        },
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::Position),
            "position",
            csp::common::Vector3 { 0, 0, 0 },
        },
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::Rotation),
            "rotation",
            csp::common::Vector4 { 0, 0, 0, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::Scale),
            "scale",
            csp::common::Vector3 { 1, 1, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::IsLoopPlayback),
            "isLoopPlayback",
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::IsPlaying),
            "isPlaying",
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::IsVisible),
            "isVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::AnimationIndex),
            "animationIndex",
            static_cast<int64_t>(-1),
        },
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::IsARVisible),
            "isARVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::ThirdPartyComponentRef),
            {}, // not exposed to scripting
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::IsShadowCaster),
            "isShadowCaster",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::ShowAsHoldoutInAR),
            "showAsHoldoutInAR",
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::ShowAsHoldoutInVirtual),
            "showAsHoldoutInVirtual",
            false,
        },
    },
};

const ComponentSchema& AnimatedModelSpaceComponent::GetSchema() { return Schema; }

AnimatedModelSpaceComponent::AnimatedModelSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
}

/* IExternalResourceComponent */

const csp::common::String& AnimatedModelSpaceComponent::GetExternalResourceAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetId));
}

void AnimatedModelSpaceComponent::SetExternalResourceAssetId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetId), value);
}

const csp::common::String& AnimatedModelSpaceComponent::GetExternalResourceAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetCollectionId));
}

void AnimatedModelSpaceComponent::SetExternalResourceAssetCollectionId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetCollectionId), value);
}

/* ITransformComponent */

const csp::common::Vector3& AnimatedModelSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(AnimatedModelPropertyKeys::Position));
}

void AnimatedModelSpaceComponent::SetPosition(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::Position), value);
}

const csp::common::Vector4& AnimatedModelSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(AnimatedModelPropertyKeys::Rotation));
}

void AnimatedModelSpaceComponent::SetRotation(const csp::common::Vector4& value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::Rotation), value);
}

const csp::common::Vector3& AnimatedModelSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(AnimatedModelPropertyKeys::Scale));
}

void AnimatedModelSpaceComponent::SetScale(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::Scale), value);
}

SpaceTransform AnimatedModelSpaceComponent::GetTransform() const
{
    SpaceTransform transform;
    transform.Position = GetPosition();
    transform.Rotation = GetRotation();
    transform.Scale = GetScale();

    return transform;
}

void AnimatedModelSpaceComponent::SetTransform(const SpaceTransform& inValue)
{
    SetPosition(inValue.Position);
    SetRotation(inValue.Rotation);
    SetScale(inValue.Scale);
}

bool AnimatedModelSpaceComponent::GetIsLoopPlayback() const
{
    return GetBooleanProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsLoopPlayback));
}

void AnimatedModelSpaceComponent::SetIsLoopPlayback(bool value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsLoopPlayback), value);
}

bool AnimatedModelSpaceComponent::GetIsPlaying() const { return GetBooleanProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsPlaying)); }

void AnimatedModelSpaceComponent::SetIsPlaying(bool value) { SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsPlaying), value); }

int64_t AnimatedModelSpaceComponent::GetAnimationIndex() const
{
    return GetIntegerProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::AnimationIndex));
}

void AnimatedModelSpaceComponent::SetAnimationIndex(int64_t value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::AnimationIndex), value);
}

csp::common::Map<csp::common::String, csp::common::String> AnimatedModelSpaceComponent::GetMaterialOverrides() const
{
    // Convert replicated values map to string values
    common::Map<common::String, common::ReplicatedValue> replicatedOverrides
        = GetStringMapProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::MaterialOverrides));

    csp::common::Map<common::String, common::String> overrides;

    std::unique_ptr<common::Array<common::String>> keys(const_cast<common::Array<common::String>*>(replicatedOverrides.Keys()));

    for (size_t i = 0; i < keys->Size(); ++i)
    {
        const auto& currentKey = (*keys)[i];
        overrides[currentKey] = replicatedOverrides[currentKey].GetString();
    }

    return overrides;
}

void AnimatedModelSpaceComponent::AddMaterialOverride(const csp::common::String& modelPath, const csp::common::String& materialId)
{
    auto replicatedOverrides = GetStringMapProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::MaterialOverrides));
    replicatedOverrides[modelPath] = materialId;

    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::MaterialOverrides), replicatedOverrides);
}

void AnimatedModelSpaceComponent::RemoveMaterialOverride(const csp::common::String& modelPath)
{
    auto replicatedOverrides = GetStringMapProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::MaterialOverrides));
    replicatedOverrides.Remove(modelPath);

    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::MaterialOverrides), replicatedOverrides);
}

/* IVisibleComponent */

bool AnimatedModelSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsVisible)); }

void AnimatedModelSpaceComponent::SetIsVisible(bool value) { SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsVisible), value); }

bool AnimatedModelSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsARVisible)); }

void AnimatedModelSpaceComponent::SetIsARVisible(bool value) { SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsARVisible), value); }

bool AnimatedModelSpaceComponent::GetIsVirtualVisible() const
{
    return GetBooleanProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsVirtualVisible));
}

void AnimatedModelSpaceComponent::SetIsVirtualVisible(bool value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsVirtualVisible), value);
}

/* IThirdPartyRefComponent */

const csp::common::String& AnimatedModelSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ThirdPartyComponentRef));
}

void AnimatedModelSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& inValue)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ThirdPartyComponentRef), inValue);
}

/* IShadowCasterComponent */

bool AnimatedModelSpaceComponent::GetIsShadowCaster() const
{
    return GetBooleanProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsShadowCaster));
}

void AnimatedModelSpaceComponent::SetIsShadowCaster(bool value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsShadowCaster), value);
}

/* IRenderBehaviourComponent */

bool AnimatedModelSpaceComponent::GetShowAsHoldoutInAR() const
{
    return GetBooleanProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ShowAsHoldoutInAR));
}

void AnimatedModelSpaceComponent::SetShowAsHoldoutInAR(bool value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ShowAsHoldoutInAR), value);
}

bool AnimatedModelSpaceComponent::GetShowAsHoldoutInVirtual() const
{
    return GetBooleanProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ShowAsHoldoutInVirtual));
}

void AnimatedModelSpaceComponent::SetShowAsHoldoutInVirtual(bool value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ShowAsHoldoutInVirtual), value);
}

} // namespace csp::multiplayer
