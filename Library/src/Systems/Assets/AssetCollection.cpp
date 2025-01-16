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

#include "CSP/Systems/Log/LogSystem.h"
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
    else if (DTOPrototypeType == "Charity")
        return csp::systems::EAssetCollectionType::FOUNDATION_INTERNAL;
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
        assert(false && "Unsupported Prototype Type!");
        return csp::systems::EAssetCollectionType::DEFAULT;
    }
}

void PrototypeDtoToAssetCollection(const chs::PrototypeDto& Dto, csp::systems::AssetCollection& AssetCollection)
{
    AssetCollection.Id = Dto.GetId();
    AssetCollection.Name = Dto.GetName();

    if (Dto.HasType())
    {
        AssetCollection.Type = ConvertDTOPrototypeType(Dto.GetType());
    }

    if (Dto.HasTags())
    {
        auto& Tags = Dto.GetTags();
        AssetCollection.Tags = csp::common::Array<csp::common::String>(Tags.size());

        for (int i = 0; i < Tags.size(); ++i)
        {
            AssetCollection.Tags[i] = Tags[i];
        }
    }

    if (Dto.HasMetadata())
    {
        auto& Metadata = Dto.GetMetadata();

        for (auto& Pair : Metadata)
        {
            auto& Metadata = AssetCollection.GetMetadataMutable();
            Metadata[Pair.first] = Pair.second;
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

    AssetCollection.CreatedBy = Dto.GetCreatedBy();
    AssetCollection.CreatedAt = Dto.GetCreatedAt();
    AssetCollection.UpdatedBy = Dto.GetUpdatedBy();
    AssetCollection.UpdatedAt = Dto.GetUpdatedAt();

    if (Dto.HasHighlander())
    {
        AssetCollection.IsUnique = Dto.GetHighlander();
    }

    if (Dto.HasOrganizationId())
    {
        AssetCollection.OrganizationId = Dto.GetOrganizationId();
    }
}

} // namespace

namespace csp::systems
{

AssetCollection::AssetCollection()
    : Type(EAssetCollectionType::DEFAULT)
    , IsUnique(false)
{
    Metadata = CSP_NEW csp::common::Map<csp::common::String, csp::common::String>();
}

AssetCollection::AssetCollection(const AssetCollection& Other)
    : AssetCollection()
{
    *this = Other;
}

AssetCollection::~AssetCollection() { CSP_DELETE(Metadata); }

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
    OrganizationId = Other.OrganizationId;

    *Metadata = *Other.Metadata;

    return *this;
}

csp::common::Map<csp::common::String, csp::common::String>& AssetCollection::GetMetadataMutable() { return *Metadata; }

const csp::common::Map<csp::common::String, csp::common::String>& AssetCollection::GetMetadataImmutable() const { return *Metadata; }

AssetCollection& AssetCollectionResult::GetAssetCollection() { return AssetCollection; }

const AssetCollection& AssetCollectionResult::GetAssetCollection() const { return AssetCollection; }

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

    auto* ProfileDataResponse = static_cast<csp::services::DtoArray<chs::PrototypeDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        ProfileDataResponse->FromJson(Response->GetPayload().GetContent());
        FillResultTotalCount(Response->GetPayload().GetContent());

        const std::vector<chs::PrototypeDto>& PrototypeArray = ProfileDataResponse->GetArray();
        AssetCollections = csp::common::Array<csp::systems::AssetCollection>(PrototypeArray.size());

        for (size_t i = 0; i < PrototypeArray.size(); ++i)
        {
            PrototypeDtoToAssetCollection(PrototypeArray[i], AssetCollections[i]);
        }
    }
}

void AssetCollectionsResult::FillResultTotalCount(const csp::common::String& JsonContent)
{
    rapidjson::Document JsonDoc;

    ResultTotalCount = 0;

    if (JsonContent.c_str() != nullptr)
    {
        JsonDoc.Parse(JsonContent.c_str());

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
}
} // namespace csp::systems
