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

#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"

#include "CSP/Multiplayer/ComponentSchema.h"

#include <memory>

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::StaticModel),
    "StaticModel",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::ExternalResourceAssetId),
            "externalResourceAssetId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::ExternalResourceAssetCollectionId),
            "externalResourceAssetCollectionId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::MaterialOverrides),
            {}, // not exposed to scripting
            csp::common::Map<csp::common::String, csp::common::ReplicatedValue>(),
        },
        {
            static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::Position),
            "position",
            csp::common::Vector3::Zero(),
        },
        {
            static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::Rotation),
            "rotation",
            csp::common::Vector4::Identity(),
        },
        {
            static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::Scale),
            "scale",
            csp::common::Vector3::One(),
        },
        {
            static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::IsVisible),
            "isVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::IsARVisible),
            "isARVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::ThirdPartyComponentRef),
            {}, // not exposed to scripting
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::IsShadowCaster),
            "isShadowCaster",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::ShowAsHoldoutInAR),
            "showAsHoldoutInAR",
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::ShowAsHoldoutInVirtual),
            "showAsHoldoutInVirtual",
            false,
        },
    },
};

const ComponentSchema& StaticModelSpaceComponent::GetSchema() { return Schema; }

StaticModelSpaceComponent::StaticModelSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
}

/* IExternalResourceComponent */

const csp::common::String& StaticModelSpaceComponent::GetExternalResourceAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ExternalResourceAssetId));
}

void StaticModelSpaceComponent::SetExternalResourceAssetId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ExternalResourceAssetId), value);
}

const csp::common::String& StaticModelSpaceComponent::GetExternalResourceAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ExternalResourceAssetCollectionId));
}

void StaticModelSpaceComponent::SetExternalResourceAssetCollectionId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ExternalResourceAssetCollectionId), value);
}

csp::common::Map<csp::common::String, csp::common::String> StaticModelSpaceComponent::GetMaterialOverrides() const
{
    // Convert replicated values map to string values
    common::Map<common::String, common::ReplicatedValue> replicatedOverrides
        = GetStringMapProperty(static_cast<uint32_t>(StaticModelPropertyKeys::MaterialOverrides));

    csp::common::Map<csp::common::String, csp::common::String> overrides;

    std::unique_ptr<common::Array<common::String>> keys(const_cast<common::Array<common::String>*>(replicatedOverrides.Keys()));

    for (size_t i = 0; i < keys->Size(); ++i)
    {
        const auto& currentKey = (*keys)[i];
        overrides[currentKey] = replicatedOverrides[currentKey].GetString();
    }

    return overrides;
}

void StaticModelSpaceComponent::AddMaterialOverride(const csp::common::String& modelPath, const csp::common::String& materialId)
{
    common::Map<common::String, common::ReplicatedValue> replicatedOverrides
        = GetStringMapProperty(static_cast<uint32_t>(StaticModelPropertyKeys::MaterialOverrides));

    replicatedOverrides[modelPath] = materialId;

    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::MaterialOverrides), replicatedOverrides);
}

void StaticModelSpaceComponent::RemoveMaterialOverride(const csp::common::String& modelPath)
{
    auto replicatedOverrides = GetStringMapProperty(static_cast<uint32_t>(StaticModelPropertyKeys::MaterialOverrides));
    replicatedOverrides.Remove(modelPath);

    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::MaterialOverrides), replicatedOverrides);
}

/* ITransformComponent */

const csp::common::Vector3& StaticModelSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(StaticModelPropertyKeys::Position));
}

void StaticModelSpaceComponent::SetPosition(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::Position), value);
}

const csp::common::Vector4& StaticModelSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(StaticModelPropertyKeys::Rotation));
}

void StaticModelSpaceComponent::SetRotation(const csp::common::Vector4& value)
{
    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::Rotation), value);
}

const csp::common::Vector3& StaticModelSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(StaticModelPropertyKeys::Scale));
}

void StaticModelSpaceComponent::SetScale(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::Scale), value);
}

SpaceTransform StaticModelSpaceComponent::GetTransform() const
{
    SpaceTransform transform;
    transform.Position = GetPosition();
    transform.Rotation = GetRotation();
    transform.Scale = GetScale();

    return transform;
}

void StaticModelSpaceComponent::SetTransform(const SpaceTransform& inValue)
{
    SetPosition(inValue.Position);
    SetRotation(inValue.Rotation);
    SetScale(inValue.Scale);
}

/* IVisibleComponent */

bool StaticModelSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsVisible)); }

void StaticModelSpaceComponent::SetIsVisible(bool inValue) { SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsVisible), inValue); }

bool StaticModelSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsARVisible)); }

void StaticModelSpaceComponent::SetIsARVisible(bool inValue) { SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsARVisible), inValue); }

bool StaticModelSpaceComponent::GetIsVirtualVisible() const
{
    return GetBooleanProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsVirtualVisible));
}

void StaticModelSpaceComponent::SetIsVirtualVisible(bool inValue)
{
    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsVirtualVisible), inValue);
}

const csp::common::String& StaticModelSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ThirdPartyComponentRef));
}

void StaticModelSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& inValue)
{
    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ThirdPartyComponentRef), inValue);
}

bool StaticModelSpaceComponent::GetIsShadowCaster() const
{
    return GetBooleanProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsShadowCaster));
}

void StaticModelSpaceComponent::SetIsShadowCaster(bool value) { SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsShadowCaster), value); }

/* IRenderBehaviourComponent */

bool StaticModelSpaceComponent::GetShowAsHoldoutInAR() const
{
    return GetBooleanProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ShowAsHoldoutInAR));
}

void StaticModelSpaceComponent::SetShowAsHoldoutInAR(bool inValue)
{
    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ShowAsHoldoutInAR), inValue);
}

bool StaticModelSpaceComponent::GetShowAsHoldoutInVirtual() const
{
    return GetBooleanProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ShowAsHoldoutInVirtual));
}

void StaticModelSpaceComponent::SetShowAsHoldoutInVirtual(bool inValue)
{
    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ShowAsHoldoutInVirtual), inValue);
}

} // namespace csp::multiplayer
