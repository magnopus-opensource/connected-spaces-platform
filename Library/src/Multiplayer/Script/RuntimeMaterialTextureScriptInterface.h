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
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Systems/Assets/RuntimeMaterialSystem.h"

#include <string>
#include <vector>

namespace csp::systems
{
class RuntimeMaterialSystem;
struct RuntimeMaterialState;
struct RuntimeMaterialTexturePatch;
class TextureInfo;
}

namespace csp::multiplayer
{

class RuntimeMaterialTextureScriptInterface
{
public:
    using Vector2 = std::vector<float>;

    RuntimeMaterialTextureScriptInterface(csp::systems::RuntimeMaterialSystem* InRuntimeMaterialSystem,
        csp::systems::RuntimeMaterialTextureSlot InSlot, const csp::common::String& InMaterialId);
    RuntimeMaterialTextureScriptInterface(csp::systems::RuntimeMaterialSystem* InRuntimeMaterialSystem,
        csp::systems::RuntimeMaterialTextureSlot InSlot, uint64_t InEntityId, ComponentType InComponentType, int32_t InComponentIndex,
        const csp::common::String& InMaterialPath, const csp::common::String& InMaterialId);

    bool GetIsSet() const;
    int32_t GetSourceType() const;
    std::string GetAssetCollectionId() const;
    std::string GetAssetId() const;
    std::string GetEntityComponentId() const;

    Vector2 GetUVOffset() const;
    void SetUVOffset(Vector2 Value);
    void SetUVOffsetXY(float X, float Y);
    Vector2 GetUVScale() const;
    void SetUVScale(Vector2 Value);
    void SetUVScaleXY(float X, float Y);
    float GetUVRotation() const;
    void SetUVRotation(float Value);
    int32_t GetTexCoord() const;
    void SetTexCoord(int32_t Value);
    int32_t GetStereoVideoType() const;
    void SetStereoVideoType(int32_t Value);
    bool GetIsStereoFlipped() const;
    void SetIsStereoFlipped(bool Value);

    bool SetAssetSource(const std::string& AssetCollectionId, const std::string& AssetId);
    bool SetComponentSource(const std::string& ComponentId);
    bool Clear();

private:
    enum class RefKind
    {
        Global,
        Binding
    };

    csp::systems::RuntimeMaterialState Resolve() const;
    bool ResolveTextureInfo(csp::systems::TextureInfo& OutTextureInfo) const;
    bool ApplyPatch(const csp::systems::RuntimeMaterialTexturePatch& Patch);

    csp::systems::RuntimeMaterialSystem* RuntimeMaterialSystem;
    csp::systems::RuntimeMaterialTextureSlot Slot;
    RefKind Kind;
    csp::common::String MaterialId;
    csp::common::String MaterialPath;
    uint64_t EntityId = 0;
    ComponentType ComponentTypeValue = ComponentType::Invalid;
    int32_t ComponentIndex = -1;
};

} // namespace csp::multiplayer
