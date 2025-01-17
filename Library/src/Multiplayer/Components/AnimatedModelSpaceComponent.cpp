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

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/AnimatedModelSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

AnimatedModelSpaceComponent::AnimatedModelSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::AnimatedModel, Parent)
{
    Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetId)] = "";
    Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetCollectionId)] = "";
    Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::MaterialOverrides)]
        = csp::common::Map<csp::common::String, csp::multiplayer::ReplicatedValue>();
    Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::Position)] = csp::common::Vector3 { 0, 0, 0 };
    Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::Rotation)] = csp::common::Vector4 { 0, 0, 0, 1 };
    Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::Scale)] = csp::common::Vector3 { 1, 1, 1 };
    Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::IsLoopPlayback)] = false;
    Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::IsPlaying)] = false;
    Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::IsVisible)] = true;
    Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::AnimationIndex)] = static_cast<int64_t>(-1);
    Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::IsARVisible)] = true;
    Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::ThirdPartyComponentRef)] = "";
    Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::IsShadowCaster)] = true;

    SetScriptInterface(CSP_NEW AnimatedModelSpaceComponentScriptInterface(this));
}

/* IExternalResourceComponent */

const csp::common::String& AnimatedModelSpaceComponent::GetExternalResourceAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetId));
}

void AnimatedModelSpaceComponent::SetExternalResourceAssetId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetId), Value);
}

const csp::common::String& AnimatedModelSpaceComponent::GetExternalResourceAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetCollectionId));
}

void AnimatedModelSpaceComponent::SetExternalResourceAssetCollectionId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetCollectionId), Value);
}

/* ITransformComponent */

const csp::common::Vector3& AnimatedModelSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(AnimatedModelPropertyKeys::Position));
}

void AnimatedModelSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::Position), Value);
}

const csp::common::Vector4& AnimatedModelSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(AnimatedModelPropertyKeys::Rotation));
}

void AnimatedModelSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::Rotation), Value);
}

const csp::common::Vector3& AnimatedModelSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(AnimatedModelPropertyKeys::Scale));
}

void AnimatedModelSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::Scale), Value);
}

SpaceTransform AnimatedModelSpaceComponent::GetTransform() const
{
    SpaceTransform Transform;
    Transform.Position = GetPosition();
    Transform.Rotation = GetRotation();
    Transform.Scale = GetScale();

    return Transform;
}

void AnimatedModelSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
    SetPosition(InValue.Position);
    SetRotation(InValue.Rotation);
    SetScale(InValue.Scale);
}

bool AnimatedModelSpaceComponent::GetIsLoopPlayback() const
{
    return GetBooleanProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsLoopPlayback));
}

void AnimatedModelSpaceComponent::SetIsLoopPlayback(bool Value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsLoopPlayback), Value);
}

bool AnimatedModelSpaceComponent::GetIsPlaying() const { return GetBooleanProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsPlaying)); }

void AnimatedModelSpaceComponent::SetIsPlaying(bool Value) { SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsPlaying), Value); }

int64_t AnimatedModelSpaceComponent::GetAnimationIndex() const
{
    return GetIntegerProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::AnimationIndex));
}

void AnimatedModelSpaceComponent::SetAnimationIndex(int64_t Value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::AnimationIndex), Value);
}

csp::common::Map<csp::common::String, csp::common::String> AnimatedModelSpaceComponent::GetMaterialOverrides() const
{
    // Convert replicated values map to string values
    common::Map<common::String, multiplayer::ReplicatedValue> ReplicatedOverrides
        = GetStringMapProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::MaterialOverrides));

    csp::common::Map<common::String, common::String> Overrides;

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

void AnimatedModelSpaceComponent::AddMaterialOverride(const csp::common::String& ModelPath, const csp::common::String& MaterialId)
{
    auto ReplicatedOverrides = GetStringMapProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::MaterialOverrides));
    ReplicatedOverrides[ModelPath] = MaterialId;

    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::MaterialOverrides), ReplicatedOverrides);
}

void AnimatedModelSpaceComponent::RemoveMaterialOverride(const csp::common::String& ModelPath)
{
    auto ReplicatedOverrides = GetStringMapProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::MaterialOverrides));
    ReplicatedOverrides.Remove(ModelPath);

    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::MaterialOverrides), ReplicatedOverrides);
}

/* IVisibleComponent */

bool AnimatedModelSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsVisible)); }

void AnimatedModelSpaceComponent::SetIsVisible(bool Value) { SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsVisible), Value); }

bool AnimatedModelSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsARVisible)); }

void AnimatedModelSpaceComponent::SetIsARVisible(bool Value) { SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsARVisible), Value); }

/* IThirdPartyRefComponent */

const csp::common::String& AnimatedModelSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ThirdPartyComponentRef));
}

void AnimatedModelSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ThirdPartyComponentRef), InValue);
}

/* IShadowCasterComponent */

bool AnimatedModelSpaceComponent::GetIsShadowCaster() const
{
    return GetBooleanProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsShadowCaster));
}

void AnimatedModelSpaceComponent::SetIsShadowCaster(bool Value)
{
    SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsShadowCaster), Value);
}

} // namespace csp::multiplayer
