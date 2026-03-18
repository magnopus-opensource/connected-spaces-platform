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

csp::systems::EAssetCollectionType ConvertDTOPrototypeType(const csp::common::String& DTOPrototypeType)
{
    if (DTOPrototypeType == "Default")
        return csp::systems::EAssetCollectionType::DEFAULT;
    else if (DTOPrototypeType == "FoundationInternal")
        return csp::systems::EAssetCollectionType::FOUNDATION_INTERNAL;
    else if (DTOPrototypeType == "CommentContainer")
        return csp::systems::EAssetCollectionType::COMMENT_CONTAINER;
    else if (DTOPrototypeType == "Comment")
        return csp::systems::EAssetCollectionType::COMMENT;
    else if (DTOPrototypeType == "SpaceThumbnail")
        return csp::systems::EAssetCollectionType::SPACE_THUMBNAIL;
    else
    {
        CSP_LOG_FORMAT(csp::common::LogLevel::Error, "Encountered unknown prototype type whilst processing an asset collection DTO: %s",
            DTOPrototypeType.c_str());
        return csp::systems::EAssetCollectionType::DEFAULT;
    }
}
} // namespace

namespace csp::systems
{

// Templated function capable of handling different PrototypeDto types whose schema match.
// Currently known to be compatible with both chs::PrototypeDto and chs::CopiedPrototypeDto.
template <class PrototypeDto> void AssetCollectionFromDtoOfType(const PrototypeDto& Dto, csp::systems::AssetCollection& AssetCollection)
{
    if (Dto.HasId())
    {
        AssetCollection.Id = Dto.GetId();
    }

    if (Dto.HasName())
    {
        AssetCollection.Name = Dto.GetName();
    }

    if (Dto.HasType())
    {
        AssetCollection.Type = ConvertDTOPrototypeType(Dto.GetType());
    }

    if (Dto.HasTags())
    {
        auto& Tags = Dto.GetTags();
        AssetCollection.Tags = csp::common::Array<csp::common::String>(Tags.size());

        for (size_t i = 0; i < Tags.size(); ++i)
        {
            AssetCollection.Tags[i] = Tags[i];
        }
    }

    if (Dto.HasMetadata())
    {
        auto& Metadata = Dto.GetMetadata();

        for (auto& Pair : Metadata)
        {
            auto& MetadataMutable = AssetCollection.GetMetadataMutable();
            MetadataMutable[Pair.first] = Pair.second;
        }
    }

    if (Dto.HasPointOfInterestId())
    {
        AssetCollection.PointOfInterestId = Dto.GetPointOfInterestId();
    }

    if (Dto.HasParentId())
    {
        AssetCollection.ParentId = Dto.GetParentId();
    }

    if (Dto.HasGroupIds())
    {
        auto& GroupIds = Dto.GetGroupIds();

        if (GroupIds.size() > 0)
        {
            AssetCollection.SpaceId = GroupIds[0];
        }
    }

    if (Dto.HasCreatedBy())
    {
        AssetCollection.CreatedBy = Dto.GetCreatedBy();
    }

    if (Dto.HasCreatedAt())
    {
        AssetCollection.CreatedAt = Dto.GetCreatedAt();
    }

    if (Dto.HasUpdatedBy())
    {
        AssetCollection.UpdatedBy = Dto.GetUpdatedBy();
    }

    if (Dto.HasUpdatedAt())
    {
        AssetCollection.UpdatedAt = Dto.GetUpdatedAt();
    }

    if (Dto.HasHighlander())
    {
        AssetCollection.IsUnique = Dto.GetHighlander();
    }
}

void PrototypeDtoToAssetCollection(const chs::PrototypeDto& Dto, csp::systems::AssetCollection& AssetCollection)
{
    AssetCollectionFromDtoOfType<chs::PrototypeDto>(Dto, AssetCollection);
}

AssetCollection::AssetCollection()
    : Type(EAssetCollectionType::DEFAULT)
    , IsUnique(false)
{
    Metadata = new csp::common::Map<csp::common::String, csp::common::String>();
}

AssetCollection::AssetCollection(const AssetCollection& Other)
    : AssetCollection()
{
    *this = Other;
}

AssetCollection::~AssetCollection() { delete (Metadata); }

AssetCollection& AssetCollection::operator=(const AssetCollection& Other)
{
    Id = Other.Id;
    Name = Other.Name;
    Type = Other.Type;
    Tags = Other.Tags;
    PointOfInterestId = Other.PointOfInterestId;
    ParentId = Other.ParentId;
    SpaceId = Other.SpaceId;
    CreatedBy = Other.CreatedBy;
    CreatedAt = Other.CreatedAt;
    UpdatedBy = Other.UpdatedBy;
    UpdatedAt = Other.UpdatedAt;
    IsUnique = Other.IsUnique;
    Version = Other.Version;

    *Metadata = *Other.Metadata;

    return *this;
}

bool AssetCollection::operator==(const AssetCollection& Other) const
{
    return Id == Other.Id && Name == Other.Name && Type == Other.Type && Tags == Other.Tags && PointOfInterestId == Other.PointOfInterestId
        && ParentId == Other.ParentId && SpaceId == Other.SpaceId && CreatedBy == Other.CreatedBy && CreatedAt == Other.CreatedAt
        && UpdatedBy == Other.UpdatedBy && UpdatedAt == Other.UpdatedAt && IsUnique == Other.IsUnique && Version == Other.Version
        && *Metadata == *Other.Metadata;
}

bool AssetCollection::operator!=(const AssetCollection& Other) const { return !(*this == Other); }

csp::common::Map<csp::common::String, csp::common::String>& AssetCollection::GetMetadataMutable() { return *Metadata; }

const csp::common::Map<csp::common::String, csp::common::String>& AssetCollection::GetMetadataImmutable() const { return *Metadata; }

AssetCollection& AssetCollectionResult::GetAssetCollection() { return AssetCollection; }

const AssetCollection& AssetCollectionResult::GetAssetCollection() const { return AssetCollection; }

void AssetCollectionResult::SetAssetCollection(const systems::AssetCollection& Collection) { AssetCollection = Collection; }

void AssetCollectionResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* PrototypeResponse = static_cast<chs::PrototypeDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        PrototypeResponse->FromJson(Response->GetPayload().GetContent());

        PrototypeDtoToAssetCollection(*PrototypeResponse, AssetCollection);
    }
}

csp::common::Array<AssetCollection>& AssetCollectionsResult::GetAssetCollections() { return AssetCollections; }

const csp::common::Array<AssetCollection>& AssetCollectionsResult::GetAssetCollections() const { return AssetCollections; }

uint64_t AssetCollectionsResult::GetTotalCount() const { return ResultTotalCount; }

void AssetCollectionsResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* PrototypeDto = static_cast<csp::services::DtoArray<chs::PrototypeDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        PrototypeDto->FromJson(Response->GetPayload().GetContent());
        FillResultTotalCount(Response->GetPayload().GetContent());

        const std::vector<chs::PrototypeDto>& PrototypeArray = PrototypeDto->GetArray();
        AssetCollections = csp::common::Array<csp::systems::AssetCollection>(PrototypeArray.size());

        for (size_t i = 0; i < PrototypeArray.size(); ++i)
        {
            PrototypeDtoToAssetCollection(PrototypeArray[i], AssetCollections[i]);
        }
    }
}

void AssetCollectionsResult::FillResultTotalCount(const csp::common::String& JsonContent)
{
    assert(JsonContent.c_str());

    rapidjson::Document JsonDoc;
    rapidjson::ParseResult ok = csp::json::ParseWithErrorLogging(JsonDoc, JsonContent, "AssetCollectionsResult::FillResultTotalCount");
    if (!ok)
    {
        return;
    }
    
    ResultTotalCount = 0;
    if (JsonDoc.IsArray())
    {
        ResultTotalCount = JsonDoc.GetArray().Size();
    }
    else if (JsonDoc.HasMember("itemTotalCount"))
    {
        rapidjson::Value& Val = JsonDoc["itemTotalCount"];
        const auto TotalCountStr = csp::web::JsonObjectToString(Val);

        uint64_t ConvertedTotalCount = 0;
        const auto result = std::from_chars(TotalCountStr.c_str(), TotalCountStr.c_str() + TotalCountStr.Length(), ConvertedTotalCount);

        if (result.ec == std::errc())
        {
            ResultTotalCount = ConvertedTotalCount;
        }
    }
}

uint64_t AssetCollectionCountResult::GetCount() const { return Count; }
void AssetCollectionCountResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    const auto* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        Count = std::stoull(Response->GetPayload().GetContent().c_str());
    }
}

void AssetCollectionsCopyResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    // Copy-prototype operation results come back in the form of an array of prototypes.
    // The payload for this is _nearly_ identical in form to what is returned from a standard
    // request for an array of prototypes, but it does use different DTO types.

    // So we specialize AssetCollectionsResult::OnResponse, and populate
    // the array of returned asset collections using this other set of DTO types.

    auto* CopyResultDto = static_cast<chs::CopyPrototypesResult*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        CopyResultDto->FromJson(Response->GetPayload().GetContent());

        const std::vector<std::shared_ptr<chs::CopiedPrototypeDto>>& CopiedPrototypesArray = CopyResultDto->GetPrototypes();
        AssetCollections = csp::common::Array<csp::systems::AssetCollection>(CopiedPrototypesArray.size());

        // Prototype duplication results aren't paginated, so the total count is just the number of copied prototypes.
        ResultTotalCount = CopiedPrototypesArray.size();

        for (size_t i = 0; i < CopiedPrototypesArray.size(); ++i)
        {
            AssetCollectionFromDtoOfType<chs::CopiedPrototypeDto>(*CopiedPrototypesArray[i], AssetCollections[i]);
        }
    }
}

} // namespace csp::systems
