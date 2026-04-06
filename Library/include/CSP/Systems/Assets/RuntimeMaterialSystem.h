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

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Multiplayer/ComponentBase.h"

#include <functional>
#include <memory>

namespace csp::common
{
class LogSystem;
class NetworkEventData;
}

namespace csp::systems
{

class Material;
class GLTFMaterial;
class AlphaVideoMaterial;

enum class RuntimeMaterialStatus
{
    Missing,
    Loading,
    Resolved
};

enum class RuntimeMaterialTextureSlot
{
    BaseColor,
    MetallicRoughness,
    Normal,
    Occlusion,
    Emissive,
    Color
};

struct RuntimeMaterialState
{
    RuntimeMaterialStatus Status = RuntimeMaterialStatus::Missing;
    csp::common::String Handle;
    csp::common::String MaterialId;
    csp::common::String MaterialPath;
    uint64_t EntityId = 0;
    int32_t ComponentType = 0;
    int32_t ComponentIndex = 0;
    Material* MaterialRef = nullptr;
};

struct RuntimeMaterialPatch
{
    bool HasBaseColorFactor = false;
    csp::common::Vector4 BaseColorFactor = csp::common::Vector4::Zero();

    bool HasMetallicFactor = false;
    float MetallicFactor = 0.0f;

    bool HasRoughnessFactor = false;
    float RoughnessFactor = 0.0f;

    bool HasEmissiveFactor = false;
    csp::common::Vector3 EmissiveFactor = csp::common::Vector3::Zero();

    bool HasEmissiveStrength = false;
    float EmissiveStrength = 0.0f;

    bool HasAlphaCutoff = false;
    float AlphaCutoff = 0.0f;

    bool HasDoubleSided = false;
    bool DoubleSided = false;

    bool HasTint = false;
    csp::common::Vector3 Tint = csp::common::Vector3::Zero();

    bool HasAlphaFactor = false;
    float AlphaFactor = 0.0f;

    bool HasEmissiveIntensity = false;
    float EmissiveIntensity = 0.0f;

    bool HasFresnelFactor = false;
    float FresnelFactor = 0.0f;

    bool HasAlphaMask = false;
    float AlphaMask = 0.0f;

    bool HasIsEmissive = false;
    bool IsEmissive = false;

    bool HasBlendMode = false;
    int32_t BlendMode = 0;

    bool HasReadAlphaFromChannel = false;
    int32_t ReadAlphaFromChannel = 0;
};

struct RuntimeMaterialTexturePatch
{
    bool HasIsSet = false;
    bool IsSet = false;

    bool HasSourceType = false;
    int32_t SourceType = 0;

    bool HasAssetCollectionId = false;
    csp::common::String AssetCollectionId;

    bool HasAssetId = false;
    csp::common::String AssetId;

    bool HasEntityComponentId = false;
    csp::common::String EntityComponentId;

    bool HasUVOffset = false;
    csp::common::Vector2 UVOffset = csp::common::Vector2::Zero();

    bool HasUVScale = false;
    csp::common::Vector2 UVScale = csp::common::Vector2::Zero();

    bool HasUVRotation = false;
    float UVRotation = 0.0f;

    bool HasTexCoord = false;
    int32_t TexCoord = 0;

    bool HasStereoVideoType = false;
    int32_t StereoVideoType = 0;

    bool HasIsStereoFlipped = false;
    bool IsStereoFlipped = false;
};

typedef std::function<void(const csp::common::String& MaterialId, uint64_t EntityId, int32_t ComponentType, int32_t ComponentIndex,
    const csp::common::String& MaterialPath)> RuntimeMaterialChangedCallbackHandler;

class CSP_API RuntimeMaterialSystem
{
public:
    explicit RuntimeMaterialSystem(csp::common::LogSystem& InLogSystem);
    ~RuntimeMaterialSystem();

    Material* Get(const csp::common::String& MaterialId);
    Material* GetForBinding(uint64_t EntityId, csp::multiplayer::ComponentType ComponentType, int32_t ComponentIndex,
        const csp::common::String& MaterialPath, const csp::common::String& MaterialId);
    CSP_EVENT void SetChangedCallback(RuntimeMaterialChangedCallbackHandler Callback);

    CSP_START_IGNORE
    CSP_NO_EXPORT void OnEnterSpace(const csp::common::String& InSpaceId);
    CSP_NO_EXPORT void OnExitSpace();

    CSP_NO_EXPORT RuntimeMaterialState Resolve(const csp::common::String& MaterialId);
    CSP_NO_EXPORT RuntimeMaterialState ResolveForBinding(uint64_t EntityId, csp::multiplayer::ComponentType ComponentType, int32_t ComponentIndex,
        const csp::common::String& MaterialPath, const csp::common::String& MaterialId);
    CSP_NO_EXPORT bool PatchHandle(const csp::common::String& Handle, const RuntimeMaterialPatch& Patch);
    CSP_NO_EXPORT bool PatchTextureHandle(
        const csp::common::String& Handle, RuntimeMaterialTextureSlot Slot, const RuntimeMaterialTexturePatch& Patch);
    CSP_NO_EXPORT bool ResetHandle(const csp::common::String& Handle);
    CSP_NO_EXPORT bool SaveHandle(const csp::common::String& Handle);

#ifdef CSP_TESTS
    CSP_NO_EXPORT void SetGLTFForTesting(const csp::common::String& MaterialId, const csp::common::String& MaterialCollectionId,
        const GLTFMaterial& Material);
    CSP_NO_EXPORT void SetAlphaVideoForTesting(const csp::common::String& MaterialId, const csp::common::String& MaterialCollectionId,
        const AlphaVideoMaterial& Material);
#endif
    CSP_END_IGNORE

private:
    class Impl;
    std::unique_ptr<Impl> ImplData;

    void RefreshAll();
    void RegisterAssetDetailBlobChangedListener();
    void UnregisterAssetDetailBlobChangedListener();
    void OnAssetDetailBlobChanged(const csp::common::NetworkEventData& NetworkEventData);
};

} // namespace csp::systems
