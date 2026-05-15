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

#include "CSP/Systems/Assets/TextureInfo.h"

#include "CSP/Common/Array.h"

#include "Json/JsonSerializer.h"

void ToJson(csp::json::JsonSerializer& serializer, const csp::systems::TextureInfo& obj)
{
    if (obj.m_sourceType == csp::systems::ETextureResourceType::ImageAsset)
    {
        // If the resource type is an ImageAsset, we only need to serialize
        // AssetCollectionId and AssetId
        serializer.SerializeMember("assetCollectionId", obj.m_assetCollectionId);
        serializer.SerializeMember("assetId", obj.m_assetId);
    }
    else
    {
        // If the resource type is a Component, we only need to serialize
        // EntityComponentId
        serializer.SerializeMember("entityComponentId", obj.m_entityComponentId);
    }

    serializer.SerializeMember("sourceType", static_cast<uint32_t>(obj.m_sourceType));
    serializer.SerializeMember("uvOffset", csp::common::Array<float> { obj.m_uvOffset.X, obj.m_uvOffset.Y });
    serializer.SerializeMember("uvRotation", obj.m_uvRotation);
    serializer.SerializeMember("uvScale", csp::common::Array<float> { obj.m_uvScale.X, obj.m_uvScale.Y });
    serializer.SerializeMember("texCoord", obj.m_texCoord);
    serializer.SerializeMember("stereoVideoType", static_cast<uint32_t>(obj.m_stereoVideoType));
    serializer.SerializeMember("isStereoFlipped", obj.m_isStereoFlipped);
}

void FromJson(const csp::json::JsonDeserializer& deserializer, csp::systems::TextureInfo& obj)
{
    // Deserialize the source type first to get the necessary information
    uint32_t sourceType;
    deserializer.SafeDeserializeMember("sourceType", sourceType);

    obj.m_sourceType = static_cast<csp::systems::ETextureResourceType>(sourceType);

    if (obj.m_sourceType == csp::systems::ETextureResourceType::ImageAsset)
    {
        // If the resource type is an ImageAsset, we only need to deserialize
        // AssetCollectionId and AssetId
        deserializer.SafeDeserializeMember("assetCollectionId", obj.m_assetCollectionId);
        deserializer.SafeDeserializeMember("assetId", obj.m_assetId);
    }
    else
    {
        // If the resource type is a Component, we only need to deserialize
        // EntityComponentId
        deserializer.SafeDeserializeMember("entityComponentId", obj.m_entityComponentId);
    }

    csp::common::Array<float> uvOffsetArray;
    deserializer.SafeDeserializeMember("uvOffset", uvOffsetArray);

    obj.m_uvOffset = csp::common::Vector2(uvOffsetArray[0], uvOffsetArray[1]);

    deserializer.SafeDeserializeMember("uvRotation", obj.m_uvRotation);

    csp::common::Array<float> uvScaleArray;
    deserializer.SafeDeserializeMember("uvScale", uvScaleArray);

    obj.m_uvScale = csp::common::Vector2(uvScaleArray[0], uvScaleArray[1]);

    deserializer.SafeDeserializeMember("texCoord", obj.m_texCoord);

    uint32_t stereoVideoTypeValue;
    deserializer.SafeDeserializeMember("stereoVideoType", stereoVideoTypeValue);
    obj.m_stereoVideoType = static_cast<csp::multiplayer::StereoVideoType>(stereoVideoTypeValue);

    deserializer.SafeDeserializeMember("isStereoFlipped", obj.m_isStereoFlipped);
}

namespace csp::systems
{
TextureInfo::TextureInfo()
    : m_assetCollectionId("")
    , m_assetId("")
    , m_entityComponentId("")
    , m_sourceType(ETextureResourceType::ImageAsset)
    , m_uvOffset(0.f, 0.f)
    , m_uvRotation(0.f)
    , m_uvScale(1.f, 1.f)
    , m_stereoVideoType(csp::multiplayer::StereoVideoType::None)
    , m_isStereoFlipped(false)
    , m_texCoord(0)
    , m_set(true)
{
}

TextureInfo::TextureInfo(const csp::common::String& inAssetCollectionId, const csp::common::String& inAssetId)
    : TextureInfo()
{
    m_assetCollectionId = inAssetCollectionId;
    m_assetId = inAssetId;
    m_sourceType = ETextureResourceType::ImageAsset;
}

TextureInfo::TextureInfo(const csp::common::String& componentId)
    : TextureInfo()
{
    m_entityComponentId = componentId;
    m_sourceType = ETextureResourceType::Component;
}

void TextureInfo::SetCollectionAndAssetId(const csp::common::String& inAssetCollectionId, const csp::common::String& inAssetId)
{
    m_assetCollectionId = inAssetCollectionId;
    m_assetId = inAssetId;
    m_sourceType = ETextureResourceType::ImageAsset;
}

const csp::common::String& TextureInfo::GetAssetCollectionId() const { return m_assetCollectionId; }

const csp::common::String& TextureInfo::GetAssetId() const { return m_assetId; }

void TextureInfo::SetEntityComponentId(const csp::common::String& componentId)
{
    m_entityComponentId = componentId;
    m_sourceType = ETextureResourceType::Component;
}

const csp::common::String& TextureInfo::GetEntityComponentId() const { return m_entityComponentId; }

ETextureResourceType TextureInfo::GetSourceType() const { return m_sourceType; }

void TextureInfo::SetUVOffset(csp::common::Vector2 offset) { m_uvOffset = offset; }

csp::common::Vector2 TextureInfo::GetUVOffset() const { return m_uvOffset; }

void TextureInfo::SetUVRotation(float rotation) { m_uvRotation = rotation; }

float TextureInfo::GetUVRotation() const { return m_uvRotation; }

void TextureInfo::SetUVScale(csp::common::Vector2 scale) { m_uvScale = scale; }

csp::common::Vector2 TextureInfo::GetUVScale() const { return m_uvScale; }

void TextureInfo::SetTexCoord(int coord) { m_texCoord = coord; }

int TextureInfo::GetTexCoord() const { return m_texCoord; }

bool TextureInfo::IsSet() const { return m_set; }

void TextureInfo::SetTexture(bool value) { m_set = value; }

csp::multiplayer::StereoVideoType TextureInfo::GetStereoVideoType() const { return m_stereoVideoType; }

void TextureInfo::SetStereoVideoType(csp::multiplayer::StereoVideoType value) { m_stereoVideoType = value; }

bool TextureInfo::GetIsStereoFlipped() const { return m_isStereoFlipped; }

void TextureInfo::SetIsStereoFlipped(bool value) { m_isStereoFlipped = value; }
} // namespace csp::systems
