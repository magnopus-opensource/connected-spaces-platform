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

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/StaticModelSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

StaticModelSpaceComponent::StaticModelSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::StaticModel, Parent)
{
    Properties[static_cast<uint32_t>(StaticModelPropertyKeys::ExternalResourceAssetId)] = "";
    Properties[static_cast<uint32_t>(StaticModelPropertyKeys::ExternalResourceAssetCollectionId)] = "";
    Properties[static_cast<uint32_t>(StaticModelPropertyKeys::MaterialOverrides)]
        = csp::common::Map<csp::common::String, csp::multiplayer::ReplicatedValue>();
    Properties[static_cast<uint32_t>(StaticModelPropertyKeys::Position)] = csp::common::Vector3::Zero();
    Properties[static_cast<uint32_t>(StaticModelPropertyKeys::Rotation)] = csp::common::Vector4::Identity();
    Properties[static_cast<uint32_t>(StaticModelPropertyKeys::Scale)] = csp::common::Vector3::One();
    Properties[static_cast<uint32_t>(StaticModelPropertyKeys::IsVisible)] = true;
    Properties[static_cast<uint32_t>(StaticModelPropertyKeys::IsARVisible)] = true;
    Properties[static_cast<uint32_t>(StaticModelPropertyKeys::ThirdPartyComponentRef)] = "";
    Properties[static_cast<uint32_t>(StaticModelPropertyKeys::IsShadowCaster)] = true;

    SetScriptInterface(CSP_NEW StaticModelSpaceComponentScriptInterface(this));
}

/* IExternalResourceComponent */

const csp::common::String& StaticModelSpaceComponent::GetExternalResourceAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ExternalResourceAssetId));
}

void StaticModelSpaceComponent::SetExternalResourceAssetId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ExternalResourceAssetId), Value);
}

const csp::common::String& StaticModelSpaceComponent::GetExternalResourceAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ExternalResourceAssetCollectionId));
}

void StaticModelSpaceComponent::SetExternalResourceAssetCollectionId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ExternalResourceAssetCollectionId), Value);
}

csp::common::Map<csp::common::String, csp::common::String> StaticModelSpaceComponent::GetMaterialOverrides() const
{
    // Convert replicated values map to string values
    common::Map<common::String, multiplayer::ReplicatedValue> ReplicatedOverrides
        = GetStringMapProperty(static_cast<uint32_t>(StaticModelPropertyKeys::MaterialOverrides));

    csp::common::Map<csp::common::String, csp::common::String> Overrides;

    auto Deleter = [](const common::Array<common::String>* Ptr) { CSP_DELETE(Ptr); };

    std::unique_ptr<common::Array<common::String>, decltype(Deleter)> Keys(
        const_cast<common::Array<common::String>*>(ReplicatedOverrides.Keys()), Deleter);

    for (size_t i = 0; i < Keys->Size(); ++i)
    {
        const auto& CurrentKey = (*Keys)[i];
        Overrides[CurrentKey] = ReplicatedOverrides[CurrentKey].GetString();
    }

    return Overrides;
}

void StaticModelSpaceComponent::AddMaterialOverride(const csp::common::String& ModelPath, const csp::common::String& MaterialId)
{
    common::Map<common::String, multiplayer::ReplicatedValue> ReplicatedOverrides
        = GetStringMapProperty(static_cast<uint32_t>(StaticModelPropertyKeys::MaterialOverrides));

    ReplicatedOverrides[ModelPath] = MaterialId;

    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::MaterialOverrides), ReplicatedOverrides);
}

void StaticModelSpaceComponent::RemoveMaterialOverride(const csp::common::String& ModelPath)
{
    auto ReplicatedOverrides = GetStringMapProperty(static_cast<uint32_t>(StaticModelPropertyKeys::MaterialOverrides));
    ReplicatedOverrides.Remove(ModelPath);

    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::MaterialOverrides), ReplicatedOverrides);
}

/* ITransformComponent */

const csp::common::Vector3& StaticModelSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(StaticModelPropertyKeys::Position));
}

void StaticModelSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::Position), Value);
}

const csp::common::Vector4& StaticModelSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(StaticModelPropertyKeys::Rotation));
}

void StaticModelSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::Rotation), Value);
}

const csp::common::Vector3& StaticModelSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(StaticModelPropertyKeys::Scale));
}

void StaticModelSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::Scale), Value);
}

SpaceTransform StaticModelSpaceComponent::GetTransform() const
{
    SpaceTransform Transform;
    Transform.Position = GetPosition();
    Transform.Rotation = GetRotation();
    Transform.Scale = GetScale();

    return Transform;
}

void StaticModelSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
    SetPosition(InValue.Position);
    SetRotation(InValue.Rotation);
    SetScale(InValue.Scale);
}

/* IVisibleComponent */

bool StaticModelSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsVisible)); }

void StaticModelSpaceComponent::SetIsVisible(bool InValue) { SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsVisible), InValue); }

bool StaticModelSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsARVisible)); }

void StaticModelSpaceComponent::SetIsARVisible(bool InValue) { SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsARVisible), InValue); }

const csp::common::String& StaticModelSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ThirdPartyComponentRef));
}

void StaticModelSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
    SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ThirdPartyComponentRef), InValue);
}

bool StaticModelSpaceComponent::GetIsShadowCaster() const
{
    return GetBooleanProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsShadowCaster));
}

void StaticModelSpaceComponent::SetIsShadowCaster(bool Value) { SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsShadowCaster), Value); }

} // namespace csp::multiplayer
