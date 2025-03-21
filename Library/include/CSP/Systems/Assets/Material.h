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
#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Common/Array.h"
#include "CSP/Systems/WebService.h"
#include "CSP/Systems/Assets/TextureInfo.h"

namespace csp::systems
{
class Material;

}
void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::Material& Obj);


namespace csp::services
{

class ApiResponseBase;

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{
/// @brief Enum representing the shader type of a material.
enum class EShaderType
{
    Standard = 0,
    AlphaVideo = 1
};

/// @brief Defines how to alpha value is interpreted
/// The alpha value is taken from the fourth component of the base color for metallic-roughness material model
enum class EAlphaMode
{
    Opaque,
    Mask,
    Blend
};

/// @brief Defines how to alpha value is interpreted
/// The alpha value is taken from the fourth component of the base color for metallic-roughness material model, unless the shader supports EColorChannel.
enum class EBlendMode
{
    Normal = 0,
    Additive = 1,
};

/// @brief Defines where the alpha value is read from
/// The alpha value is usually taken from the fourth component of the base color but this allows is to be read from another channel
enum class EColorChannel
{
    R = 0,
    G = 1,
    B = 2,
    A = 3
};


/// @ingroup Asset System
/// @brief Base class for a material.
class CSP_API Material
{
public:
    /// @brief Gets the user-defined name of the material
    /// @return csp::common::String&
    const csp::common::String& GetName() const;

    /// @brief Gets the shader type of the material
    /// @return csp::systems::EShaderType
    const csp::systems::EShaderType GetShaderType() const;

    /// @brief Gets the version of the material
    /// @return int
    const int GetVersion() const;

    /// @brief Gets the collection id for the material
    /// @return const csp::common::String&
    const csp::common::String& GetMaterialCollectionId() const;

    /// @brief Gets the id for the material
    /// @return const csp::common::String&
    const csp::common::String& GetMaterialId() const;

    /// @brief Constructor which links the material to an asset
    /// @param Name const csp::common::String& : The name of the material.
    /// @param MaterialCollectionId const csp::common::String& : The asset collection where the material info is stored
    /// @param MaterialId const csp::common::String& : The asset where the material info is stored
    Material(const csp::common::String& Name, const csp::common::String& MaterialCollectionId, const csp::common::String& MaterialId);
    Material(const csp::common::String& Name, const csp::common::String& MaterialCollectionId, const csp::common::String& MaterialId, const EShaderType& InType, const int InVersion);

    virtual ~Material() = default;

    Material() = default;
    csp::common::String Name;
    EShaderType Type;
    int Version;

protected:
    csp::common::String CollectionId;
    csp::common::String Id;

private:
    friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::Material& Obj);
};




/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to download material data.
class CSP_API MaterialResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retreives the Material from the result.
    const Material& GetMaterial() const;

    CSP_NO_EXPORT MaterialResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    MaterialResult(void*) {};

    void SetMaterial(Material& Material);

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    Material* Material;
};

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to download a collection of material data.
class CSP_API MaterialsResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retreives the Material from the result.
    const csp::common::Array<csp::systems::Material*>* GetMaterials() const;

    CSP_NO_EXPORT MaterialsResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    MaterialsResult(void*) {};

    void SetMaterials(const csp::common::Array<csp::systems::Material*>& Materials);

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<Material*> Materials;
};

/// @brief Callback containing material data.
/// @param Result MaterialResult : result class
typedef std::function<void(const MaterialResult& Result)> MaterialResultCallback;

/// @brief Callback containing a collection of material data.
/// @param Result Array<MaterialResult> : result class
typedef std::function<void(const MaterialsResult& Result)> MaterialsResultCallback;

} // namespace csp::systems
