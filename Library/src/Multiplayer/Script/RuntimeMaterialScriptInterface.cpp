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

#include "Multiplayer/Script/RuntimeMaterialScriptInterface.h"

#include "CSP/Systems/Assets/AlphaVideoMaterial.h"
#include "CSP/Systems/Assets/GLTFMaterial.h"
#include "CSP/Systems/Assets/RuntimeMaterialSystem.h"

namespace
{

std::string ToStatusString(csp::systems::RuntimeMaterialStatus Status)
{
    switch (Status)
    {
    case csp::systems::RuntimeMaterialStatus::Resolved:
        return "resolved";
    case csp::systems::RuntimeMaterialStatus::Loading:
        return "loading";
    default:
        return "missing";
    }
}

} // namespace

namespace csp::multiplayer
{

RuntimeMaterialScriptInterface::RuntimeMaterialScriptInterface(
    csp::systems::RuntimeMaterialSystem* InRuntimeMaterialSystem, const csp::common::String& InMaterialId)
    : RuntimeMaterialSystem(InRuntimeMaterialSystem)
    , Kind(RefKind::Global)
    , MaterialId(InMaterialId)
{
}

RuntimeMaterialScriptInterface::RuntimeMaterialScriptInterface(csp::systems::RuntimeMaterialSystem* InRuntimeMaterialSystem, uint64_t InEntityId,
    ComponentType InComponentType, int32_t InComponentIndex, const csp::common::String& InMaterialPath, const csp::common::String& InMaterialId)
    : RuntimeMaterialSystem(InRuntimeMaterialSystem)
    , Kind(RefKind::Binding)
    , MaterialId(InMaterialId)
    , MaterialPath(InMaterialPath)
    , EntityId(InEntityId)
    , ComponentTypeValue(InComponentType)
    , ComponentIndex(InComponentIndex)
{
}

csp::systems::RuntimeMaterialState RuntimeMaterialScriptInterface::Resolve() const
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

bool RuntimeMaterialScriptInterface::ApplyPatch(const csp::systems::RuntimeMaterialPatch& Patch)
{
    const auto State = Resolve();
    return State.Status == csp::systems::RuntimeMaterialStatus::Resolved && RuntimeMaterialSystem != nullptr
        && RuntimeMaterialSystem->PatchHandle(State.Handle, Patch);
}

std::string RuntimeMaterialScriptInterface::GetStatus() const { return ToStatusString(Resolve().Status); }

std::string RuntimeMaterialScriptInterface::GetMaterialId() const { return Resolve().MaterialId.c_str(); }

std::string RuntimeMaterialScriptInterface::GetMaterialPath() const { return Resolve().MaterialPath.c_str(); }

int32_t RuntimeMaterialScriptInterface::GetShaderType() const
{
    const auto State = Resolve();
    return State.MaterialRef != nullptr ? static_cast<int32_t>(State.MaterialRef->GetShaderType()) : 0;
}

std::string RuntimeMaterialScriptInterface::GetName() const
{
    const auto State = Resolve();
    return State.MaterialRef != nullptr ? State.MaterialRef->GetName().c_str() : "";
}

RuntimeMaterialScriptInterface::Vector4 RuntimeMaterialScriptInterface::GetBaseColorFactor() const
{
    const auto State = Resolve();
    const auto* GLTF = (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::Standard)
        ? static_cast<csp::systems::GLTFMaterial*>(State.MaterialRef)
        : nullptr;
    if (GLTF == nullptr)
    {
        return {};
    }

    const auto& Value = GLTF->GetBaseColorFactor();
    return { Value.X, Value.Y, Value.Z, Value.W };
}

void RuntimeMaterialScriptInterface::SetBaseColorFactor(Vector4 Value)
{
    if (Value.size() < 4)
    {
        return;
    }

    csp::systems::RuntimeMaterialPatch Patch;
    Patch.HasBaseColorFactor = true;
    Patch.BaseColorFactor = csp::common::Vector4(Value[0], Value[1], Value[2], Value[3]);
    ApplyPatch(Patch);
}

float RuntimeMaterialScriptInterface::GetMetallicFactor() const
{
    const auto State = Resolve();
    const auto* GLTF = (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::Standard)
        ? static_cast<csp::systems::GLTFMaterial*>(State.MaterialRef)
        : nullptr;
    return GLTF != nullptr ? GLTF->GetMetallicFactor() : 0.0f;
}

void RuntimeMaterialScriptInterface::SetMetallicFactor(float Value)
{
    csp::systems::RuntimeMaterialPatch Patch;
    Patch.HasMetallicFactor = true;
    Patch.MetallicFactor = Value;
    ApplyPatch(Patch);
}

float RuntimeMaterialScriptInterface::GetRoughnessFactor() const
{
    const auto State = Resolve();
    const auto* GLTF = (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::Standard)
        ? static_cast<csp::systems::GLTFMaterial*>(State.MaterialRef)
        : nullptr;
    return GLTF != nullptr ? GLTF->GetRoughnessFactor() : 0.0f;
}

void RuntimeMaterialScriptInterface::SetRoughnessFactor(float Value)
{
    csp::systems::RuntimeMaterialPatch Patch;
    Patch.HasRoughnessFactor = true;
    Patch.RoughnessFactor = Value;
    ApplyPatch(Patch);
}

RuntimeMaterialScriptInterface::Vector3 RuntimeMaterialScriptInterface::GetEmissiveFactor() const
{
    const auto State = Resolve();
    const auto* GLTF = (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::Standard)
        ? static_cast<csp::systems::GLTFMaterial*>(State.MaterialRef)
        : nullptr;
    if (GLTF == nullptr)
    {
        return {};
    }

    const auto& Value = GLTF->GetEmissiveFactor();
    return { Value.X, Value.Y, Value.Z };
}

void RuntimeMaterialScriptInterface::SetEmissiveFactor(Vector3 Value)
{
    if (Value.size() < 3)
    {
        return;
    }

    csp::systems::RuntimeMaterialPatch Patch;
    Patch.HasEmissiveFactor = true;
    Patch.EmissiveFactor = csp::common::Vector3(Value[0], Value[1], Value[2]);
    ApplyPatch(Patch);
}

float RuntimeMaterialScriptInterface::GetEmissiveStrength() const
{
    const auto State = Resolve();
    const auto* GLTF = (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::Standard)
        ? static_cast<csp::systems::GLTFMaterial*>(State.MaterialRef)
        : nullptr;
    return GLTF != nullptr ? GLTF->GetEmissiveStrength() : 0.0f;
}

void RuntimeMaterialScriptInterface::SetEmissiveStrength(float Value)
{
    csp::systems::RuntimeMaterialPatch Patch;
    Patch.HasEmissiveStrength = true;
    Patch.EmissiveStrength = Value;
    ApplyPatch(Patch);
}

float RuntimeMaterialScriptInterface::GetAlphaCutoff() const
{
    const auto State = Resolve();
    const auto* GLTF = (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::Standard)
        ? static_cast<csp::systems::GLTFMaterial*>(State.MaterialRef)
        : nullptr;
    return GLTF != nullptr ? GLTF->GetAlphaCutoff() : 0.0f;
}

void RuntimeMaterialScriptInterface::SetAlphaCutoff(float Value)
{
    csp::systems::RuntimeMaterialPatch Patch;
    Patch.HasAlphaCutoff = true;
    Patch.AlphaCutoff = Value;
    ApplyPatch(Patch);
}

RuntimeMaterialScriptInterface::Vector3 RuntimeMaterialScriptInterface::GetTint() const
{
    const auto State = Resolve();
    const auto* AlphaVideo = (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::AlphaVideo)
        ? static_cast<csp::systems::AlphaVideoMaterial*>(State.MaterialRef)
        : nullptr;
    if (AlphaVideo == nullptr)
    {
        return {};
    }

    const auto& Value = AlphaVideo->GetTint();
    return { Value.X, Value.Y, Value.Z };
}

void RuntimeMaterialScriptInterface::SetTint(Vector3 Value)
{
    if (Value.size() < 3)
    {
        return;
    }

    csp::systems::RuntimeMaterialPatch Patch;
    Patch.HasTint = true;
    Patch.Tint = csp::common::Vector3(Value[0], Value[1], Value[2]);
    ApplyPatch(Patch);
}

float RuntimeMaterialScriptInterface::GetAlphaFactor() const
{
    const auto State = Resolve();
    const auto* AlphaVideo = (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::AlphaVideo)
        ? static_cast<csp::systems::AlphaVideoMaterial*>(State.MaterialRef)
        : nullptr;
    return AlphaVideo != nullptr ? AlphaVideo->GetAlphaFactor() : 0.0f;
}

void RuntimeMaterialScriptInterface::SetAlphaFactor(float Value)
{
    csp::systems::RuntimeMaterialPatch Patch;
    Patch.HasAlphaFactor = true;
    Patch.AlphaFactor = Value;
    ApplyPatch(Patch);
}

float RuntimeMaterialScriptInterface::GetEmissiveIntensity() const
{
    const auto State = Resolve();
    const auto* AlphaVideo = (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::AlphaVideo)
        ? static_cast<csp::systems::AlphaVideoMaterial*>(State.MaterialRef)
        : nullptr;
    return AlphaVideo != nullptr ? AlphaVideo->GetEmissiveIntensity() : 0.0f;
}

void RuntimeMaterialScriptInterface::SetEmissiveIntensity(float Value)
{
    csp::systems::RuntimeMaterialPatch Patch;
    Patch.HasEmissiveIntensity = true;
    Patch.EmissiveIntensity = Value;
    ApplyPatch(Patch);
}

float RuntimeMaterialScriptInterface::GetFresnelFactor() const
{
    const auto State = Resolve();
    const auto* AlphaVideo = (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::AlphaVideo)
        ? static_cast<csp::systems::AlphaVideoMaterial*>(State.MaterialRef)
        : nullptr;
    return AlphaVideo != nullptr ? AlphaVideo->GetFresnelFactor() : 0.0f;
}

void RuntimeMaterialScriptInterface::SetFresnelFactor(float Value)
{
    csp::systems::RuntimeMaterialPatch Patch;
    Patch.HasFresnelFactor = true;
    Patch.FresnelFactor = Value;
    ApplyPatch(Patch);
}

float RuntimeMaterialScriptInterface::GetAlphaMask() const
{
    const auto State = Resolve();
    const auto* AlphaVideo = (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::AlphaVideo)
        ? static_cast<csp::systems::AlphaVideoMaterial*>(State.MaterialRef)
        : nullptr;
    return AlphaVideo != nullptr ? AlphaVideo->GetAlphaMask() : 0.0f;
}

void RuntimeMaterialScriptInterface::SetAlphaMask(float Value)
{
    csp::systems::RuntimeMaterialPatch Patch;
    Patch.HasAlphaMask = true;
    Patch.AlphaMask = Value;
    ApplyPatch(Patch);
}

bool RuntimeMaterialScriptInterface::GetDoubleSided() const
{
    const auto State = Resolve();
    if (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::Standard)
    {
        const auto* GLTF = static_cast<csp::systems::GLTFMaterial*>(State.MaterialRef);
        return GLTF->GetDoubleSided();
    }
    if (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::AlphaVideo)
    {
        const auto* AlphaVideo = static_cast<csp::systems::AlphaVideoMaterial*>(State.MaterialRef);
        return AlphaVideo->GetDoubleSided();
    }
    return false;
}

void RuntimeMaterialScriptInterface::SetDoubleSided(bool Value)
{
    csp::systems::RuntimeMaterialPatch Patch;
    Patch.HasDoubleSided = true;
    Patch.DoubleSided = Value;
    ApplyPatch(Patch);
}

bool RuntimeMaterialScriptInterface::GetIsEmissive() const
{
    const auto State = Resolve();
    const auto* AlphaVideo = (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::AlphaVideo)
        ? static_cast<csp::systems::AlphaVideoMaterial*>(State.MaterialRef)
        : nullptr;
    return AlphaVideo != nullptr ? AlphaVideo->GetIsEmissive() : false;
}

void RuntimeMaterialScriptInterface::SetIsEmissive(bool Value)
{
    csp::systems::RuntimeMaterialPatch Patch;
    Patch.HasIsEmissive = true;
    Patch.IsEmissive = Value;
    ApplyPatch(Patch);
}

int32_t RuntimeMaterialScriptInterface::GetBlendMode() const
{
    const auto State = Resolve();
    const auto* AlphaVideo = (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::AlphaVideo)
        ? static_cast<csp::systems::AlphaVideoMaterial*>(State.MaterialRef)
        : nullptr;
    return AlphaVideo != nullptr ? static_cast<int32_t>(AlphaVideo->GetBlendMode()) : 0;
}

void RuntimeMaterialScriptInterface::SetBlendMode(int32_t Value)
{
    csp::systems::RuntimeMaterialPatch Patch;
    Patch.HasBlendMode = true;
    Patch.BlendMode = Value;
    ApplyPatch(Patch);
}

int32_t RuntimeMaterialScriptInterface::GetReadAlphaFromChannel() const
{
    const auto State = Resolve();
    const auto* AlphaVideo = (State.MaterialRef != nullptr && State.MaterialRef->GetShaderType() == csp::systems::EShaderType::AlphaVideo)
        ? static_cast<csp::systems::AlphaVideoMaterial*>(State.MaterialRef)
        : nullptr;
    return AlphaVideo != nullptr ? static_cast<int32_t>(AlphaVideo->GetReadAlphaFromChannel()) : 0;
}

void RuntimeMaterialScriptInterface::SetReadAlphaFromChannel(int32_t Value)
{
    csp::systems::RuntimeMaterialPatch Patch;
    Patch.HasReadAlphaFromChannel = true;
    Patch.ReadAlphaFromChannel = Value;
    ApplyPatch(Patch);
}

bool RuntimeMaterialScriptInterface::Reset()
{
    const auto State = Resolve();
    return State.Status == csp::systems::RuntimeMaterialStatus::Resolved && RuntimeMaterialSystem != nullptr
        && RuntimeMaterialSystem->ResetHandle(State.Handle);
}

bool RuntimeMaterialScriptInterface::Save()
{
    const auto State = Resolve();
    return State.Status == csp::systems::RuntimeMaterialStatus::Resolved && RuntimeMaterialSystem != nullptr
        && RuntimeMaterialSystem->SaveHandle(State.Handle);
}

} // namespace csp::multiplayer
