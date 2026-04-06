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

#include "Multiplayer/Script/RuntimeMaterialTextureScriptInterface.h"

#include "CSP/Systems/Assets/AlphaVideoMaterial.h"
#include "CSP/Systems/Assets/GLTFMaterial.h"
#include "CSP/Systems/Assets/RuntimeMaterialSystem.h"

namespace
{

bool TryGetTextureInfo(const csp::systems::Material& Material, csp::systems::RuntimeMaterialTextureSlot Slot, csp::systems::TextureInfo& OutTextureInfo)
{
    if (Material.GetShaderType() == csp::systems::EShaderType::Standard)
    {
        const auto* GLTF = static_cast<const csp::systems::GLTFMaterial*>(&Material);
        switch (Slot)
        {
        case csp::systems::RuntimeMaterialTextureSlot::BaseColor:
            OutTextureInfo = GLTF->GetBaseColorTexture();
            return true;
        case csp::systems::RuntimeMaterialTextureSlot::MetallicRoughness:
            OutTextureInfo = GLTF->GetMetallicRoughnessTexture();
            return true;
        case csp::systems::RuntimeMaterialTextureSlot::Normal:
            OutTextureInfo = GLTF->GetNormalTexture();
            return true;
        case csp::systems::RuntimeMaterialTextureSlot::Occlusion:
            OutTextureInfo = GLTF->GetOcclusionTexture();
            return true;
        case csp::systems::RuntimeMaterialTextureSlot::Emissive:
            OutTextureInfo = GLTF->GetEmissiveTexture();
            return true;
        case csp::systems::RuntimeMaterialTextureSlot::Color:
            return false;
        }
    }

    if (Material.GetShaderType() == csp::systems::EShaderType::AlphaVideo)
    {
        const auto* AlphaVideo = static_cast<const csp::systems::AlphaVideoMaterial*>(&Material);
        if (Slot == csp::systems::RuntimeMaterialTextureSlot::Color)
        {
            OutTextureInfo = AlphaVideo->GetColorTexture();
            return true;
        }
    }

    return false;
}

} // namespace

namespace csp::multiplayer
{

RuntimeMaterialTextureScriptInterface::RuntimeMaterialTextureScriptInterface(
    csp::systems::RuntimeMaterialSystem* InRuntimeMaterialSystem, csp::systems::RuntimeMaterialTextureSlot InSlot,
    const csp::common::String& InMaterialId)
    : RuntimeMaterialSystem(InRuntimeMaterialSystem)
    , Slot(InSlot)
    , Kind(RefKind::Global)
    , MaterialId(InMaterialId)
{
}

RuntimeMaterialTextureScriptInterface::RuntimeMaterialTextureScriptInterface(
    csp::systems::RuntimeMaterialSystem* InRuntimeMaterialSystem, csp::systems::RuntimeMaterialTextureSlot InSlot, uint64_t InEntityId,
    ComponentType InComponentType, int32_t InComponentIndex, const csp::common::String& InMaterialPath, const csp::common::String& InMaterialId)
    : RuntimeMaterialSystem(InRuntimeMaterialSystem)
    , Slot(InSlot)
    , Kind(RefKind::Binding)
    , MaterialId(InMaterialId)
    , MaterialPath(InMaterialPath)
    , EntityId(InEntityId)
    , ComponentTypeValue(InComponentType)
    , ComponentIndex(InComponentIndex)
{
}

csp::systems::RuntimeMaterialState RuntimeMaterialTextureScriptInterface::Resolve() const
{
    if (RuntimeMaterialSystem == nullptr)
    {
        return {};
    }

    if (Kind == RefKind::Binding)
    {
        return RuntimeMaterialSystem->ResolveForBinding(EntityId, ComponentTypeValue, ComponentIndex, MaterialPath, MaterialId);
    }

    return RuntimeMaterialSystem->Resolve(MaterialId);
}

bool RuntimeMaterialTextureScriptInterface::ResolveTextureInfo(csp::systems::TextureInfo& OutTextureInfo) const
{
    const auto State = Resolve();
    return State.Status == csp::systems::RuntimeMaterialStatus::Resolved && State.MaterialRef != nullptr
        && TryGetTextureInfo(*State.MaterialRef, Slot, OutTextureInfo);
}

bool RuntimeMaterialTextureScriptInterface::ApplyPatch(const csp::systems::RuntimeMaterialTexturePatch& Patch)
{
    const auto State = Resolve();
    return State.Status == csp::systems::RuntimeMaterialStatus::Resolved && RuntimeMaterialSystem != nullptr
        && RuntimeMaterialSystem->PatchTextureHandle(State.Handle, Slot, Patch);
}

bool RuntimeMaterialTextureScriptInterface::GetIsSet() const
{
    csp::systems::TextureInfo TextureInfo;
    return ResolveTextureInfo(TextureInfo) ? TextureInfo.IsSet() : false;
}

int32_t RuntimeMaterialTextureScriptInterface::GetSourceType() const
{
    csp::systems::TextureInfo TextureInfo;
    return ResolveTextureInfo(TextureInfo) ? static_cast<int32_t>(TextureInfo.GetSourceType()) : 0;
}

std::string RuntimeMaterialTextureScriptInterface::GetAssetCollectionId() const
{
    csp::systems::TextureInfo TextureInfo;
    return ResolveTextureInfo(TextureInfo) ? TextureInfo.GetAssetCollectionId().c_str() : "";
}

std::string RuntimeMaterialTextureScriptInterface::GetAssetId() const
{
    csp::systems::TextureInfo TextureInfo;
    return ResolveTextureInfo(TextureInfo) ? TextureInfo.GetAssetId().c_str() : "";
}

std::string RuntimeMaterialTextureScriptInterface::GetEntityComponentId() const
{
    csp::systems::TextureInfo TextureInfo;
    return ResolveTextureInfo(TextureInfo) ? TextureInfo.GetEntityComponentId().c_str() : "";
}

RuntimeMaterialTextureScriptInterface::Vector2 RuntimeMaterialTextureScriptInterface::GetUVOffset() const
{
    csp::systems::TextureInfo TextureInfo;
    if (!ResolveTextureInfo(TextureInfo))
    {
        return {};
    }

    const auto Value = TextureInfo.GetUVOffset();
    return { Value.X, Value.Y };
}

void RuntimeMaterialTextureScriptInterface::SetUVOffset(Vector2 Value)
{
    if (Value.size() < 2)
    {
        return;
    }

    csp::systems::RuntimeMaterialTexturePatch Patch;
    Patch.HasUVOffset = true;
    Patch.UVOffset = csp::common::Vector2(Value[0], Value[1]);
    ApplyPatch(Patch);
}

void RuntimeMaterialTextureScriptInterface::SetUVOffsetXY(float X, float Y)
{
    csp::systems::RuntimeMaterialTexturePatch Patch;
    Patch.HasUVOffset = true;
    Patch.UVOffset = csp::common::Vector2(X, Y);
    ApplyPatch(Patch);
}

RuntimeMaterialTextureScriptInterface::Vector2 RuntimeMaterialTextureScriptInterface::GetUVScale() const
{
    csp::systems::TextureInfo TextureInfo;
    if (!ResolveTextureInfo(TextureInfo))
    {
        return {};
    }

    const auto Value = TextureInfo.GetUVScale();
    return { Value.X, Value.Y };
}

void RuntimeMaterialTextureScriptInterface::SetUVScale(Vector2 Value)
{
    if (Value.size() < 2)
    {
        return;
    }

    csp::systems::RuntimeMaterialTexturePatch Patch;
    Patch.HasUVScale = true;
    Patch.UVScale = csp::common::Vector2(Value[0], Value[1]);
    ApplyPatch(Patch);
}

void RuntimeMaterialTextureScriptInterface::SetUVScaleXY(float X, float Y)
{
    csp::systems::RuntimeMaterialTexturePatch Patch;
    Patch.HasUVScale = true;
    Patch.UVScale = csp::common::Vector2(X, Y);
    ApplyPatch(Patch);
}

float RuntimeMaterialTextureScriptInterface::GetUVRotation() const
{
    csp::systems::TextureInfo TextureInfo;
    return ResolveTextureInfo(TextureInfo) ? TextureInfo.GetUVRotation() : 0.0f;
}

void RuntimeMaterialTextureScriptInterface::SetUVRotation(float Value)
{
    csp::systems::RuntimeMaterialTexturePatch Patch;
    Patch.HasUVRotation = true;
    Patch.UVRotation = Value;
    ApplyPatch(Patch);
}

int32_t RuntimeMaterialTextureScriptInterface::GetTexCoord() const
{
    csp::systems::TextureInfo TextureInfo;
    return ResolveTextureInfo(TextureInfo) ? TextureInfo.GetTexCoord() : 0;
}

void RuntimeMaterialTextureScriptInterface::SetTexCoord(int32_t Value)
{
    csp::systems::RuntimeMaterialTexturePatch Patch;
    Patch.HasTexCoord = true;
    Patch.TexCoord = Value;
    ApplyPatch(Patch);
}

int32_t RuntimeMaterialTextureScriptInterface::GetStereoVideoType() const
{
    csp::systems::TextureInfo TextureInfo;
    return ResolveTextureInfo(TextureInfo) ? static_cast<int32_t>(TextureInfo.GetStereoVideoType()) : 0;
}

void RuntimeMaterialTextureScriptInterface::SetStereoVideoType(int32_t Value)
{
    csp::systems::RuntimeMaterialTexturePatch Patch;
    Patch.HasStereoVideoType = true;
    Patch.StereoVideoType = Value;
    ApplyPatch(Patch);
}

bool RuntimeMaterialTextureScriptInterface::GetIsStereoFlipped() const
{
    csp::systems::TextureInfo TextureInfo;
    return ResolveTextureInfo(TextureInfo) ? TextureInfo.GetIsStereoFlipped() : false;
}

void RuntimeMaterialTextureScriptInterface::SetIsStereoFlipped(bool Value)
{
    csp::systems::RuntimeMaterialTexturePatch Patch;
    Patch.HasIsStereoFlipped = true;
    Patch.IsStereoFlipped = Value;
    ApplyPatch(Patch);
}

bool RuntimeMaterialTextureScriptInterface::SetAssetSource(const std::string& AssetCollectionId, const std::string& AssetId)
{
    csp::systems::RuntimeMaterialTexturePatch Patch;
    Patch.HasIsSet = true;
    Patch.IsSet = true;
    Patch.HasSourceType = true;
    Patch.SourceType = static_cast<int32_t>(csp::systems::ETextureResourceType::ImageAsset);
    Patch.HasAssetCollectionId = true;
    Patch.AssetCollectionId = AssetCollectionId.c_str();
    Patch.HasAssetId = true;
    Patch.AssetId = AssetId.c_str();
    return ApplyPatch(Patch);
}

bool RuntimeMaterialTextureScriptInterface::SetComponentSource(const std::string& ComponentId)
{
    csp::systems::RuntimeMaterialTexturePatch Patch;
    Patch.HasIsSet = true;
    Patch.IsSet = true;
    Patch.HasSourceType = true;
    Patch.SourceType = static_cast<int32_t>(csp::systems::ETextureResourceType::Component);
    Patch.HasEntityComponentId = true;
    Patch.EntityComponentId = ComponentId.c_str();
    return ApplyPatch(Patch);
}

bool RuntimeMaterialTextureScriptInterface::Clear()
{
    csp::systems::RuntimeMaterialTexturePatch Patch;
    Patch.HasIsSet = true;
    Patch.IsSet = false;
    return ApplyPatch(Patch);
}

} // namespace csp::multiplayer
