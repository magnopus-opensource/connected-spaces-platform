/*
 * Copyright 2025 Magnopus LLC

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
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Systems/Assets/TextureInfo.h"
#include "CSP/Systems/WebService.h"

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
/// @details This enum is to be used in conjunction with materials, which are managed via the AssetSystem.
enum class EShaderType
{
    Standard = 0,
    AlphaVideo = 1
};

/// @brief Defines how the alpha value of a material is interpreted.
/// @details The alpha value is taken from the fourth component of the base color for metallic-roughness material model.
/// This enum is to be used in conjunction with materials, which are managed via the AssetSystem.
enum class EAlphaMode
{
    Opaque,
    Mask,
    Blend
};

/// @brief Defines how the alpha value of a material is interpreted.
/// @details The alpha value is taken from the fourth component of the base color for metallic-roughness material model, unless the shader supports
/// EColorChannel.
/// This enum is to be used in conjunction with materials, which are managed via the AssetSystem.
enum class EBlendMode
{
    Normal = 0,
    Additive = 1,
};

/// @brief Defines where the alpha value is read from.
/// @details The alpha value is usually taken from the fourth component of the base color but this allows is to be read from another channel.
/// This enum is to be used in conjunction with materials, which are managed via the AssetSystem.
enum class EColorChannel
{
    R = 0,
    G = 1,
    B = 2,
    A = 3
};

/// @ingroup Asset System.
/// @brief Base class for a material.
class CSP_API Material
{
public:
    /// @brief Gets the user-defined name of the material.
    /// @return const common::String& : Returns the material name.
    const csp::common::String& GetName() const;

    /// @brief Gets the shader type of the material.
    /// @return csp::systems::EShaderType : Returns the shader type.
    const csp::systems::EShaderType GetShaderType() const;

    /// @brief Gets the version of the material.
    /// @return int : Returns the version of the material.
    const int GetVersion() const;

    /// @brief Gets the collection id for the material.
    /// @return const csp::common::String& : Returns the collection id.
    const csp::common::String& GetMaterialCollectionId() const;

    /// @brief Gets the id for the material.
    /// @return const csp::common::String& : Returns the material id.
    const csp::common::String& GetMaterialId() const;

    /// @brief Constructs a material bound to an AssetCollection and Asset.
    /// @param Name const csp::common::String& : The name of the material.
    /// @param MaterialCollectionId const csp::common::String& : The asset collection which references the associated material asset.
    /// @param MaterialId const csp::common::String& : The asset where the material info is stored.
    Material(const csp::common::String& Name, const csp::common::String& MaterialCollectionId, const csp::common::String& MaterialId);

    /// @brief Constructs a versioned material bound to an AssetCollection and Asset.
    /// @param Name const csp::common::String& : The name of the material.
    /// @param MaterialCollectionId const csp::common::String& : The asset collection which references the associated material asset.
    /// @param MaterialId const csp::common::String& : The asset where the material info is stored.
    /// @param Type EShaderType : The material shader type.
    /// @param Version const int : The material version.
    Material(const csp::common::String& Name, const csp::common::String& MaterialCollectionId, const csp::common::String& MaterialId,
        EShaderType Type, const int Version);

    virtual ~Material() = default;

    Material() = default;

protected:
    // Copy assignment
    Material& operator=(const Material& other) = default;
    // copy ctor
    Material(const Material& other) = default;

    // Move assignment
    Material& operator=(Material&& other) = default;
    // Move ctor
    Material(Material&& other) = default;

    csp::common::String Name;
    EShaderType Type;
    int Version;
    csp::common::String CollectionId;
    csp::common::String Id;

private:
    friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::Material& Obj);
};

/// @ingroup Asset System
/// @brief Result data class that contains downloaded material data.
class CSP_API MaterialResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the Material from the result.
    /// @return const Material* : Returns a pointer to the Material object. The caller should take ownership of the pointer.
    const Material* GetMaterial() const;

    /// @brief Retrieves the Material from the result.
    /// @return Material* : Returns a pointer to the Material object. The caller should take ownership of the pointer.
    Material* GetMaterial();

    CSP_NO_EXPORT MaterialResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    MaterialResult(void*) {};

    /// The result object is taking ownership of the pointer to the Material.
    void SetMaterial(Material* Material);

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    Material* Material;
};

/// @ingroup Asset System
/// @brief Result data class that contains a collection of downloaded material data.
class CSP_API MaterialsResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retreives an Array of Materials from the result.
    /// @return const Array<Material*>* : Returns a const pointer to an Array of Material class pointers. The caller should take ownership of the
    /// pointer.
    const csp::common::Array<csp::systems::Material*>* GetMaterials() const;

    /// @brief Retreives an Array of Materials from the result.
    /// @return Array<Material*>* : Returns a pointer to an Array of Material class pointers. The caller should take ownership of the pointer.
    csp::common::Array<csp::systems::Material*>* GetMaterials();

    CSP_NO_EXPORT MaterialsResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    MaterialsResult(void*) {};

    /// The result object is taking ownership of the Material pointers in the array.
    void SetMaterials(const csp::common::Array<csp::systems::Material*>& Materials);

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<Material*> Materials;
};

/// @brief Callback containing requested material data.
/// @param Result const MaterialResult& : Material result class.
typedef std::function<void(const MaterialResult& Result)> MaterialResultCallback;

/// @brief Callback containing a collection of requested material data.
/// @param Result const MaterialsResult& : Material result class containing a collection of materials.
typedef std::function<void(const MaterialsResult& Result)> MaterialsResultCallback;

} // namespace csp::systems
