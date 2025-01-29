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

namespace csp::systems
{
class TextureInfo;
}

namespace csp::json
{
class JsonSerializer;
class JsonDeserializer;
} // namespace csp::json

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::TextureInfo& Obj);
void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::TextureInfo& Obj);

namespace csp::systems
{

/// @brief Enum representing the source type of the texture
/// If Component is set, the data of the texture comes from the specified media component
/// If ImageAsset is set, th data of the texture comes from the specified asset
enum class ETextureResourceType
{
    Component,
    ImageAsset
};

/// @ingroup Asset System
/// @brief Data class which represents a texture.
class CSP_API TextureInfo
{
public:
    /// @brief Default constructor with SourceType set to ETextureResourceType::ImageAsset by default
    TextureInfo();

    /// @brief Constructor which associates a texture with an asset, using ETextureResourceType::ImageAsset as it's source type
    /// @param AssetCollectionId const csp::common::String& : The asset collection where the texture info is stored
    /// @param AssetId const csp::common::String& : The asset where the texture info is stored
    TextureInfo(const csp::common::String& AssetCollectionId, const csp::common::String& AssetId);

    /// @brief Constructor which associates a texture with a media component, using ETextureResourceType::Component as it's source type
    /// @param ComponentId const csp::common::String& : The component id used as the texture data source
    TextureInfo(const csp::common::String& ComponentId);

    /// @brief Sets the textures asset collection id and asset id
    /// Uses ETextureResourceType::ImageAsset as it's source type
    /// @param AssetCollectionId const csp::common::String& : The asset collection where the texture info is stored
    /// @param AssetId const csp::common::String& : The asset where the texture info is stored
    void SetCollectionAndAssetId(const csp::common::String& AssetCollectionId, const csp::common::String& AssetId);

    /// @brief Gets the asset collection id for the texture
    /// @return csp::common::String&
    const csp::common::String& GetAssetCollectionId() const;

    /// @brief Gets the asset id for the texture
    /// @return csp::common::String&
    const csp::common::String& GetAssetId() const;

    /// @brief Sets the textures entity component id
    /// Format: {entity.id}-{component.id}
    /// This can be obtained from ComponentBase::GetUniqueComponentId
    /// @param ComponentId const csp::common::String&
    void SetEntityComponentId(const csp::common::String& ComponentId);

    /// @brief Gets the entity component id
    /// @return csp::common::String&
    const csp::common::String& GetEntityComponentId() const;

    /// @brief Gets the source type of this texture
    /// This is set internally by constructors and SetCollectionAndAssetId/SetEntityComponentId
    /// @return ETextureResourceType
    ETextureResourceType GetSourceType() const;

    /// @brief Sets the uv offset of the texture
    /// @param Offset csp::common::Vector2
    void SetUVOffset(csp::common::Vector2 Offset);

    /// @brief Gets the uv offset of the texture
    /// @return csp::common::Vector2
    csp::common::Vector2 GetUVOffset() const;

    /// @brief Gets the uv rotation of the texture
    /// @param Rotation float
    void SetUVRotation(float Rotation);

    /// @brief Gets the uv rotation of the texture
    /// @return float
    float GetUVRotation() const;

    /// @brief Sets the uv scale of the texture
    /// @param Scale csp::common::Vector2
    void SetUVScale(csp::common::Vector2 Scale);

    /// @brief Gets the uv scale of the texture
    /// @return csp::common::Vector2
    csp::common::Vector2 GetUVScale() const;

    /// @brief Sets The set index of texture's TEXCOORD attribute
    /// used for texture coordinate mapping.
    /// @param Coord int
    void SetTexCoord(int Coord);

    /// @brief Gets the index of texture's TEXCOORD attribute
    /// @return int
    int GetTexCoord() const;

    /// @brief True if this texture has been set for the current material
    /// @return bool
    bool IsSet() const;

    /// @brief Sets if this texture is active for the current material
    /// This is true by default when a new texture is created
    /// @param Value bool
    void SetTexture(bool Value);

private:
    csp::common::String AssetCollectionId;
    csp::common::String AssetId;
    csp::common::String EntityComponentId;

    ETextureResourceType SourceType;

    csp::common::Vector2 UVOffset;
    float UVRotation;
    csp::common::Vector2 UVScale;

    int TexCoord;

    bool Set;

    friend void ::ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::TextureInfo& Obj);
    friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::TextureInfo& Obj);
};

} // namespace csp::systems
