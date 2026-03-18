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

#include <string>
#include <vector>

namespace csp::systems
{
class RuntimeMaterialSystem;
struct RuntimeMaterialState;
struct RuntimeMaterialPatch;
}

namespace csp::multiplayer
{

class RuntimeMaterialScriptInterface
{
public:
    using Vector3 = std::vector<float>;
    using Vector4 = std::vector<float>;

    RuntimeMaterialScriptInterface(csp::systems::RuntimeMaterialSystem* InRuntimeMaterialSystem, const csp::common::String& InMaterialId);
    RuntimeMaterialScriptInterface(csp::systems::RuntimeMaterialSystem* InRuntimeMaterialSystem, uint64_t InEntityId, ComponentType InComponentType,
        int32_t InComponentIndex, const csp::common::String& InMaterialPath, const csp::common::String& InMaterialId);

    std::string GetStatus() const;
    std::string GetMaterialId() const;
    std::string GetMaterialPath() const;
    int32_t GetShaderType() const;
    std::string GetName() const;

    Vector4 GetBaseColorFactor() const;
    void SetBaseColorFactor(Vector4 Value);
    float GetMetallicFactor() const;
    void SetMetallicFactor(float Value);
    float GetRoughnessFactor() const;
    void SetRoughnessFactor(float Value);
    Vector3 GetEmissiveFactor() const;
    void SetEmissiveFactor(Vector3 Value);
    float GetEmissiveStrength() const;
    void SetEmissiveStrength(float Value);
    float GetAlphaCutoff() const;
    void SetAlphaCutoff(float Value);

    Vector3 GetTint() const;
    void SetTint(Vector3 Value);
    float GetAlphaFactor() const;
    void SetAlphaFactor(float Value);
    float GetEmissiveIntensity() const;
    void SetEmissiveIntensity(float Value);
    float GetFresnelFactor() const;
    void SetFresnelFactor(float Value);
    float GetAlphaMask() const;
    void SetAlphaMask(float Value);

    bool GetDoubleSided() const;
    void SetDoubleSided(bool Value);
    bool GetIsEmissive() const;
    void SetIsEmissive(bool Value);
    int32_t GetBlendMode() const;
    void SetBlendMode(int32_t Value);
    int32_t GetReadAlphaFromChannel() const;
    void SetReadAlphaFromChannel(int32_t Value);

    bool Reset();
    bool Save();

private:
    enum class RefKind
    {
        Global,
        Binding
    };

    csp::systems::RuntimeMaterialState Resolve() const;
    bool ApplyPatch(const csp::systems::RuntimeMaterialPatch& Patch);

    csp::systems::RuntimeMaterialSystem* RuntimeMaterialSystem;
    RefKind Kind;
    csp::common::String MaterialId;
    csp::common::String MaterialPath;
    uint64_t EntityId = 0;
    ComponentType ComponentTypeValue = ComponentType::Invalid;
    int32_t ComponentIndex = -1;
};

} // namespace csp::multiplayer
