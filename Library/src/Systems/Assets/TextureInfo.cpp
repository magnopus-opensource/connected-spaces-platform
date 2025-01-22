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

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::TextureInfo& Obj)
{
    if (Obj.SourceType == csp::systems::ETextureResourceType::ImageAsset)
    {
        // If the resource type is an ImageAsset, we only need to serialize
        // AssetCollectionId and AssetId
        Serializer.SerializeMember("assetCollectionId", Obj.AssetCollectionId);
        Serializer.SerializeMember("assetId", Obj.AssetId);
    }
    else
    {
        // If the resource type is a Component, we only need to serialize
        // EntityComponentId
        Serializer.SerializeMember("entityComponentId", Obj.EntityComponentId);
    }

    Serializer.SerializeMember("sourceType", static_cast<uint32_t>(Obj.SourceType));
    Serializer.SerializeMember("uvOffset", csp::common::Array<float> { Obj.UVOffset.X, Obj.UVOffset.Y });
    Serializer.SerializeMember("uvRotation", Obj.UVRotation);
    Serializer.SerializeMember("uvScale", csp::common::Array<float> { Obj.UVScale.X, Obj.UVScale.Y });
    Serializer.SerializeMember("texCoord", Obj.TexCoord);
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::TextureInfo& Obj)
{
    // Deserialize the source type first to get the necessary information
    uint32_t SourceType;
    Deserializer.DeserializeMember("sourceType", SourceType);

    Obj.SourceType = static_cast<csp::systems::ETextureResourceType>(SourceType);

    if (Obj.SourceType == csp::systems::ETextureResourceType::ImageAsset)
    {
        // If the resource type is an ImageAsset, we only need to deserialize
        // AssetCollectionId and AssetId
        Deserializer.DeserializeMember("assetCollectionId", Obj.AssetCollectionId);
        Deserializer.DeserializeMember("assetId", Obj.AssetId);
    }
    else
    {
        // If the resource type is a Component, we only need to deserialize
        // EntityComponentId
        Deserializer.DeserializeMember("entityComponentId", Obj.EntityComponentId);
    }

    csp::common::Array<float> UVOffsetArray;
    Deserializer.DeserializeMember("uvOffset", UVOffsetArray);

    Obj.UVOffset = csp::common::Vector2(UVOffsetArray[0], UVOffsetArray[1]);

    Deserializer.DeserializeMember("uvRotation", Obj.UVRotation);

    csp::common::Array<float> UVScaleArray;
    Deserializer.DeserializeMember("uvScale", UVScaleArray);

    Obj.UVScale = csp::common::Vector2(UVScaleArray[0], UVScaleArray[1]);

    Deserializer.DeserializeMember("texCoord", Obj.TexCoord);
}

namespace csp::systems
{
TextureInfo::TextureInfo()
    : AssetCollectionId("")
    , AssetId("")
    , EntityComponentId("")
    , SourceType(ETextureResourceType::ImageAsset)
    , UVOffset(0.f, 0.f)
    , UVRotation(0.f)
    , UVScale(1.f, 1.f)
    , TexCoord(0)
    , Set(true)
{
}

TextureInfo::TextureInfo(const csp::common::String& InAssetCollectionId, const csp::common::String& InAssetId)
    : TextureInfo()
{
    AssetCollectionId = InAssetCollectionId;
    AssetId = InAssetId;
    SourceType = ETextureResourceType::ImageAsset;
}

TextureInfo::TextureInfo(const csp::common::String& ComponentId)
    : TextureInfo()
{
    EntityComponentId = ComponentId;
    SourceType = ETextureResourceType::Component;
}

void TextureInfo::SetCollectionAndAssetId(const csp::common::String& InAssetCollectionId, const csp::common::String& InAssetId)
{
    AssetCollectionId = InAssetCollectionId;
    AssetId = InAssetId;
    SourceType = ETextureResourceType::ImageAsset;
}

const csp::common::String& TextureInfo::GetAssetCollectionId() const { return AssetCollectionId; }

const csp::common::String& TextureInfo::GetAssetId() const { return AssetId; }

void TextureInfo::SetEntityComponentId(const csp::common::String& ComponentId)
{
    EntityComponentId = ComponentId;
    SourceType = ETextureResourceType::Component;
}

const csp::common::String& TextureInfo::GetEntityComponentId() const { return EntityComponentId; }

ETextureResourceType TextureInfo::GetSourceType() const { return SourceType; }

void TextureInfo::SetUVOffset(csp::common::Vector2 Offset) { UVOffset = Offset; }

csp::common::Vector2 TextureInfo::GetUVOffset() const { return UVOffset; }

void TextureInfo::SetUVRotation(float Rotation) { UVRotation = Rotation; }

float TextureInfo::GetUVRotation() const { return UVRotation; }

void TextureInfo::SetUVScale(csp::common::Vector2 Scale) { UVScale = Scale; }

csp::common::Vector2 TextureInfo::GetUVScale() const { return UVScale; }

void TextureInfo::SetTexCoord(int Coord) { TexCoord = Coord; }

int TextureInfo::GetTexCoord() const { return TexCoord; }

bool TextureInfo::IsSet() const { return Set; }

void TextureInfo::SetTexture(bool Value) { Set = Value; }
} // namespace csp::systems
