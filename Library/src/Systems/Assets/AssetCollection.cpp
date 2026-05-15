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
#include "CSP/Systems/Assets/AssetCollection.h"
#include "Debug/Logging.h"
#include "Json/JsonParseHelper.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/PrototypeService/Dto.h"

#include <assert.h>
#include <charconv>

namespace chs = csp::services::generated::prototypeservice;

namespace
{

csp::systems::EAssetCollectionType ConvertDTOPrototypeType(const csp::common::String& dtoPrototypeType)
{
    if (dtoPrototypeType == "Default")
        return csp::systems::EAssetCollectionType::DEFAULT;
    else if (dtoPrototypeType == "FoundationInternal")
        return csp::systems::EAssetCollectionType::FOUNDATION_INTERNAL;
    else if (dtoPrototypeType == "CommentContainer")
        return csp::systems::EAssetCollectionType::COMMENT_CONTAINER;
    else if (dtoPrototypeType == "Comment")
        return csp::systems::EAssetCollectionType::COMMENT;
    else if (dtoPrototypeType == "SpaceThumbnail")
        return csp::systems::EAssetCollectionType::SPACE_THUMBNAIL;
    else
    {
        CSP_LOG_FORMAT(csp::common::LogLevel::Error, "Encountered unknown prototype type whilst processing an asset collection DTO: %s",
            dtoPrototypeType.c_str());
        return csp::systems::EAssetCollectionType::DEFAULT;
    }
}
} // namespace

namespace csp::systems
{

// Templated function capable of handling different PrototypeDto types whose schema match.
// Currently known to be compatible with both chs::PrototypeDto and chs::CopiedPrototypeDto.
template <class PrototypeDto> void AssetCollectionFromDtoOfType(const PrototypeDto& dto, csp::systems::AssetCollection& assetCollection)
{
    if (dto.HasId())
    {
        assetCollection.Id = dto.GetId();
    }

    if (dto.HasName())
    {
        assetCollection.Name = dto.GetName();
    }

    if (dto.HasType())
    {
        assetCollection.Type = ConvertDTOPrototypeType(dto.GetType());
    }

    if (dto.HasTags())
    {
        auto& tags = dto.GetTags();
        assetCollection.Tags = csp::common::Array<csp::common::String>(tags.size());

        for (size_t i = 0; i < tags.size(); ++i)
        {
            assetCollection.Tags[i] = tags[i];
        }
    }

    if (dto.HasMetadata())
    {
        auto& metadata = dto.GetMetadata();

        for (auto& pair : metadata)
        {
            auto& metadataMutable = assetCollection.GetMetadataMutable();
            metadataMutable[pair.first] = pair.second;
        }
    }

    if (dto.HasPointOfInterestId())
    {
        assetCollection.PointOfInterestId = dto.GetPointOfInterestId();
    }

    if (dto.HasParentId())
    {
        assetCollection.ParentId = dto.GetParentId();
    }

    if (dto.HasGroupIds())
    {
        auto& groupIds = dto.GetGroupIds();

        if (groupIds.size() > 0)
        {
            assetCollection.SpaceId = groupIds[0];
        }
    }

    if (dto.HasCreatedBy())
    {
        assetCollection.CreatedBy = dto.GetCreatedBy();
    }

    if (dto.HasCreatedAt())
    {
        assetCollection.CreatedAt = dto.GetCreatedAt();
    }

    if (dto.HasUpdatedBy())
    {
        assetCollection.UpdatedBy = dto.GetUpdatedBy();
    }

    if (dto.HasUpdatedAt())
    {
        assetCollection.UpdatedAt = dto.GetUpdatedAt();
    }

    if (dto.HasHighlander())
    {
        assetCollection.IsUnique = dto.GetHighlander();
    }
}

void PrototypeDtoToAssetCollection(const chs::PrototypeDto& dto, csp::systems::AssetCollection& assetCollection)
{
    AssetCollectionFromDtoOfType<chs::PrototypeDto>(dto, assetCollection);
}

AssetCollection::AssetCollection()
    : Type(EAssetCollectionType::DEFAULT)
    , IsUnique(false)
{
    m_metadata = new csp::common::Map<csp::common::String, csp::common::String>();
}

AssetCollection::AssetCollection(const AssetCollection& other)
    : AssetCollection()
{
    *this = other;
}

AssetCollection::~AssetCollection() { delete (m_metadata); }

AssetCollection& AssetCollection::operator=(const AssetCollection& other)
{
    Id = other.Id;
    Name = other.Name;
    Type = other.Type;
    Tags = other.Tags;
    PointOfInterestId = other.PointOfInterestId;
    ParentId = other.ParentId;
    SpaceId = other.SpaceId;
    CreatedBy = other.CreatedBy;
    CreatedAt = other.CreatedAt;
    UpdatedBy = other.UpdatedBy;
    UpdatedAt = other.UpdatedAt;
    IsUnique = other.IsUnique;
    Version = other.Version;

    *m_metadata = *other.m_metadata;

    return *this;
}

bool AssetCollection::operator==(const AssetCollection& other) const
{
    return Id == other.Id && Name == other.Name && Type == other.Type && Tags == other.Tags && PointOfInterestId == other.PointOfInterestId
        && ParentId == other.ParentId && SpaceId == other.SpaceId && CreatedBy == other.CreatedBy && CreatedAt == other.CreatedAt
        && UpdatedBy == other.UpdatedBy && UpdatedAt == other.UpdatedAt && IsUnique == other.IsUnique && Version == other.Version
        && *m_metadata == *other.m_metadata;
}

bool AssetCollection::operator!=(const AssetCollection& other) const { return !(*this == other); }

csp::common::Map<csp::common::String, csp::common::String>& AssetCollection::GetMetadataMutable() { return *m_metadata; }

const csp::common::Map<csp::common::String, csp::common::String>& AssetCollection::GetMetadataImmutable() const { return *m_metadata; }

AssetCollection& AssetCollectionResult::GetAssetCollection() { return m_assetCollection; }

const AssetCollection& AssetCollectionResult::GetAssetCollection() const { return m_assetCollection; }

void AssetCollectionResult::SetAssetCollection(const systems::AssetCollection& collection) { m_assetCollection = collection; }

void AssetCollectionResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* prototypeResponse = static_cast<chs::PrototypeDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        prototypeResponse->FromJson(response->GetPayload().GetContent());

        PrototypeDtoToAssetCollection(*prototypeResponse, m_assetCollection);
    }
}

csp::common::Array<AssetCollection>& AssetCollectionsResult::GetAssetCollections() { return m_assetCollections; }

const csp::common::Array<AssetCollection>& AssetCollectionsResult::GetAssetCollections() const { return m_assetCollections; }

uint64_t AssetCollectionsResult::GetTotalCount() const { return m_resultTotalCount; }

void AssetCollectionsResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* prototypeDto = static_cast<csp::services::DtoArray<chs::PrototypeDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        prototypeDto->FromJson(response->GetPayload().GetContent());
        FillResultTotalCount(response->GetPayload().GetContent());

        const std::vector<chs::PrototypeDto>& prototypeArray = prototypeDto->GetArray();
        m_assetCollections = csp::common::Array<csp::systems::AssetCollection>(prototypeArray.size());

        for (size_t i = 0; i < prototypeArray.size(); ++i)
        {
            PrototypeDtoToAssetCollection(prototypeArray[i], m_assetCollections[i]);
        }
    }
}

void AssetCollectionsResult::FillResultTotalCount(const csp::common::String& jsonContent)
{
    assert(jsonContent.c_str());

    rapidjson::Document jsonDoc;
    rapidjson::ParseResult ok = csp::json::ParseWithErrorLogging(jsonDoc, jsonContent, "AssetCollectionsResult::FillResultTotalCount");
    if (!ok)
    {
        return;
    }
    
    m_resultTotalCount = 0;
    if (jsonDoc.IsArray())
    {
        m_resultTotalCount = jsonDoc.GetArray().Size();
    }
    else if (jsonDoc.HasMember("itemTotalCount"))
    {
        rapidjson::Value& val = jsonDoc["itemTotalCount"];
        const auto totalCountStr = csp::web::JsonObjectToString(val);

        uint64_t convertedTotalCount = 0;
        const auto result = std::from_chars(totalCountStr.c_str(), totalCountStr.c_str() + totalCountStr.Length(), convertedTotalCount);

        if (result.ec == std::errc())
        {
            m_resultTotalCount = convertedTotalCount;
        }
    }
}

uint64_t AssetCollectionCountResult::GetCount() const { return m_count; }
void AssetCollectionCountResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    const auto* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        m_count = std::stoull(response->GetPayload().GetContent().c_str());
    }
}

void AssetCollectionsCopyResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    // Copy-prototype operation results come back in the form of an array of prototypes.
    // The payload for this is _nearly_ identical in form to what is returned from a standard
    // request for an array of prototypes, but it does use different DTO types.

    // So we specialize AssetCollectionsResult::OnResponse, and populate
    // the array of returned asset collections using this other set of DTO types.

    auto* copyResultDto = static_cast<chs::CopyPrototypesResult*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        copyResultDto->FromJson(response->GetPayload().GetContent());

        const std::vector<std::shared_ptr<chs::CopiedPrototypeDto>>& copiedPrototypesArray = copyResultDto->GetPrototypes();
        m_assetCollections = csp::common::Array<csp::systems::AssetCollection>(copiedPrototypesArray.size());

        // Prototype duplication results aren't paginated, so the total count is just the number of copied prototypes.
        m_resultTotalCount = copiedPrototypesArray.size();

        for (size_t i = 0; i < copiedPrototypesArray.size(); ++i)
        {
            AssetCollectionFromDtoOfType<chs::CopiedPrototypeDto>(*copiedPrototypesArray[i], m_assetCollections[i]);
        }
    }
}

} // namespace csp::systems
