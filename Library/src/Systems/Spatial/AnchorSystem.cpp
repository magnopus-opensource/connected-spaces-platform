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
#include "CSP/Systems/Spatial/AnchorSystem.h"

#include "CSP/Systems/Assets/AssetCollection.h"
#include "Common/Convert.h"
#include "Services/SpatialDataService/Api.h"
#include "Services/SpatialDataService/Dto.h"

namespace chs = csp::services::generated::spatialdataservice;

namespace csp::systems
{

AnchorSystem::AnchorSystem()
    : SystemBase(nullptr, nullptr)
    , AnchorsAPI(nullptr)
{
}

AnchorSystem::AnchorSystem(csp::web::WebClient* InWebClient)
    : SystemBase(InWebClient, nullptr)
{
    AnchorsAPI = CSP_NEW chs::AnchorsApi(InWebClient);
}

AnchorSystem::~AnchorSystem() { CSP_DELETE(AnchorsAPI); }

void AnchorSystem::CreateAnchor(csp::systems::AnchorProvider ThirdPartyAnchorProvider, const csp::common::String& ThirdPartyAnchorId,
    const csp::common::String& AssetCollectionId, const csp::systems::GeoLocation& Location, const csp::systems::OlyAnchorPosition& Position,
    const csp::systems::OlyRotation& Rotation,
    const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& SpatialKeyValue,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags, AnchorResultCallback Callback)
{
    auto AnchorInfo = std::make_shared<chs::AnchorDto>();

    switch (ThirdPartyAnchorProvider)
    {
    case AnchorProvider::GoogleCloudAnchors:
        AnchorInfo->SetThirdPartyProviderName("GoogleCloudAnchors");
        break;
    default:
        CSP_LOG_WARN_MSG("Unknown third party anchor provider");
        break;
    }

    AnchorInfo->SetThirdPartyAnchorId(ThirdPartyAnchorId);
    AnchorInfo->SetAnchoredPrototypeId(AssetCollectionId);

    auto DTOLocation = std::make_shared<chs::GeoCoord>();
    DTOLocation->SetLatitude(Location.Latitude);
    DTOLocation->SetLongitude(Location.Longitude);
    AnchorInfo->SetLocation(DTOLocation);

    auto DTOPosition = std::make_shared<chs::AnchorPosition>();
    DTOPosition->SetX(Position.X);
    DTOPosition->SetY(Position.Y);
    DTOPosition->SetZ(Position.Z);
    AnchorInfo->SetPosition(DTOPosition);

    auto DTORotation = std::make_shared<chs::AnchorRotation>();
    DTORotation->SetX(Rotation.X);
    DTORotation->SetY(Rotation.Y);
    DTORotation->SetZ(Rotation.Z);
    DTORotation->SetW(Rotation.W);
    AnchorInfo->SetRotation(DTORotation);

    // SpatialData must be set with or without data
    std::map<csp::common::String, csp::common::String> SpatialData;

    if (SpatialKeyValue.HasValue())
    {
        auto* Keys = SpatialKeyValue->Keys();

        for (auto idx = 0; idx < Keys->Size(); ++idx)
        {
            auto Key = Keys->operator[](idx);
            auto Value = SpatialKeyValue->operator[](Key);
            SpatialData.insert(std::pair<csp::common::String, csp::common::String>(Key, Value));
        }
    }

    AnchorInfo->SetSpatialKeyValue(SpatialData);

    if (Tags.HasValue())
    {
        std::vector<csp::common::String> TagsVector;
        TagsVector.reserve(Tags->Size());

        for (size_t idx = 0; idx < Tags->Size(); ++idx)
        {
            TagsVector.push_back((*Tags)[idx]);
        }

        AnchorInfo->SetTags(TagsVector);
    }

    csp::services::ResponseHandlerPtr ResponseHandler = AnchorsAPI->CreateHandler<AnchorResultCallback, AnchorResult, void, chs::AnchorDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::AnchorsApi*>(AnchorsAPI)->apiV1AnchorsPost(AnchorInfo, ResponseHandler);
}

void AnchorSystem::CreateAnchorInSpace(csp::systems::AnchorProvider ThirdPartyAnchorProvider, const csp::common::String& ThirdPartyAnchorId,
    const csp::common::String& SpaceId, uint64_t SpaceEntityId, const csp::common::String& AssetCollectionId,
    const csp::systems::GeoLocation& Location, const csp::systems::OlyAnchorPosition& Position, const csp::systems::OlyRotation& Rotation,
    const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& SpatialKeyValue,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags, AnchorResultCallback Callback)
{
    auto AnchorInfo = std::make_shared<chs::AnchorDto>();

    switch (ThirdPartyAnchorProvider)
    {
    case AnchorProvider::GoogleCloudAnchors:
        AnchorInfo->SetThirdPartyProviderName("GoogleCloudAnchors");
        break;
    default:
        CSP_LOG_WARN_MSG("Unknown third party anchor provider");
        break;
    }

    AnchorInfo->SetThirdPartyAnchorId(ThirdPartyAnchorId);
    AnchorInfo->SetReferenceId(SpaceId);
    AnchorInfo->SetAnchoredMultiplayerObjectId(static_cast<int32_t>(SpaceEntityId));
    AnchorInfo->SetAnchoredPrototypeId(AssetCollectionId);

    auto DTOLocation = std::make_shared<chs::GeoCoord>();
    DTOLocation->SetLatitude(Location.Latitude);
    DTOLocation->SetLongitude(Location.Longitude);
    AnchorInfo->SetLocation(DTOLocation);

    auto DTOPosition = std::make_shared<chs::AnchorPosition>();
    DTOPosition->SetX(Position.X);
    DTOPosition->SetY(Position.Y);
    DTOPosition->SetZ(Position.Z);
    AnchorInfo->SetPosition(DTOPosition);

    auto DTORotation = std::make_shared<chs::AnchorRotation>();
    DTORotation->SetX(Rotation.X);
    DTORotation->SetY(Rotation.Y);
    DTORotation->SetZ(Rotation.Z);
    DTORotation->SetW(Rotation.W);
    AnchorInfo->SetRotation(DTORotation);

    // SpatialData must be set with or without data
    std::map<csp::common::String, csp::common::String> SpatialData;

    if (SpatialKeyValue.HasValue())
    {
        auto* Keys = SpatialKeyValue->Keys();

        for (auto idx = 0; idx < Keys->Size(); ++idx)
        {
            auto Key = Keys->operator[](idx);
            auto Value = SpatialKeyValue->operator[](Key);
            SpatialData.insert(std::pair<csp::common::String, csp::common::String>(Key, Value));
        }
    }

    AnchorInfo->SetSpatialKeyValue(SpatialData);

    if (Tags.HasValue())
    {
        std::vector<csp::common::String> TagsVector;
        TagsVector.reserve(Tags->Size());

        for (size_t idx = 0; idx < Tags->Size(); ++idx)
        {
            TagsVector.push_back((*Tags)[idx]);
        }

        AnchorInfo->SetTags(TagsVector);
    }

    csp::services::ResponseHandlerPtr ResponseHandler = AnchorsAPI->CreateHandler<AnchorResultCallback, AnchorResult, void, chs::AnchorDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::AnchorsApi*>(AnchorsAPI)->apiV1AnchorsPost(AnchorInfo, ResponseHandler);
}

void AnchorSystem::DeleteAnchors(const csp::common::Array<csp::common::String>& AnchorIds, NullResultCallback Callback)
{
    std::vector<csp::common::String> IdsToBeDeleted;
    IdsToBeDeleted.reserve(AnchorIds.Size());

    for (size_t idx = 0; idx < AnchorIds.Size(); idx++)
    {
        IdsToBeDeleted.push_back(AnchorIds[idx]);
    }

    csp::services::ResponseHandlerPtr ResponseHandler = AnchorsAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs::AnchorsApi*>(AnchorsAPI)->apiV1AnchorsDelete(IdsToBeDeleted, ResponseHandler);
}

void AnchorSystem::GetAnchorsInArea(const csp::systems::GeoLocation& OriginLocation, const double AreaRadius,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& SpatialKeys,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& SpatialValues,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags, const csp::common::Optional<bool>& AllTags,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& SpaceIds, const csp::common::Optional<int>& Skip,
    const csp::common::Optional<int>& Limit, AnchorCollectionResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = AnchorsAPI->CreateHandler<AnchorCollectionResultCallback, AnchorCollectionResult, void, csp::services::DtoArray<chs::AnchorDto>>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    std::optional<std::vector<csp::common::String>> AnchorTags;

    if (Tags.HasValue())
    {
        AnchorTags.emplace(std::vector<csp::common::String>());
        AnchorTags->reserve(Tags->Size());

        for (size_t idx = 0; idx < Tags->Size(); ++idx)
        {
            AnchorTags->push_back({ (*Tags)[idx] });
        }
    }

    std::optional<std::vector<csp::common::String>> AnchorSpatialKeys;

    if (SpatialKeys.HasValue())
    {
        AnchorSpatialKeys.emplace(std::vector<csp::common::String>());
        AnchorSpatialKeys->reserve(SpatialKeys->Size());

        for (size_t idx = 0; idx < SpatialKeys->Size(); ++idx)
        {
            AnchorSpatialKeys->push_back({ (*SpatialKeys)[idx] });
        }
    }

    std::optional<std::vector<csp::common::String>> AnchorSpatialValues;

    if (SpatialValues.HasValue())
    {
        AnchorSpatialValues.emplace(std::vector<csp::common::String>());
        AnchorSpatialValues->reserve(SpatialValues->Size());

        for (size_t idx = 0; idx < SpatialValues->Size(); ++idx)
        {
            AnchorSpatialValues->push_back({ (*SpatialValues)[idx] });
        }
    }

    std::optional<std::vector<csp::common::String>> ReferenceIds;

    if (SpaceIds.HasValue())
    {
        ReferenceIds.emplace(std::vector<csp::common::String>());
        ReferenceIds->reserve(SpaceIds->Size());

        for (size_t idx = 0; idx < SpaceIds->Size(); ++idx)
        {
            ReferenceIds->push_back({ (*SpaceIds)[idx] });
        }
    }

    auto AnchorTagsAll = AllTags.HasValue() ? *AllTags : std::optional<bool>(std::nullopt);
    auto AnchorLimit = Limit.HasValue() ? *Limit : std::optional<int>(std::nullopt);
    auto AnchorSkip = Skip.HasValue() ? *Skip : std::optional<int>(std::nullopt);

    static_cast<chs::AnchorsApi*>(AnchorsAPI)
        ->apiV1AnchorsGet(AnchorSpatialKeys, AnchorSpatialValues, OriginLocation.Longitude, OriginLocation.Latitude, AreaRadius, AnchorTags,
            AnchorTagsAll, std::nullopt, std::nullopt, ReferenceIds, std::nullopt, AnchorSkip, AnchorLimit, ResponseHandler,
            csp::common::CancellationToken::Dummy());
}

void AnchorSystem::GetAnchorsInSpace(const csp::common::String& SpaceId, const csp::common::Optional<int>& Skip,
    const csp::common::Optional<int>& Limit, AnchorCollectionResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = AnchorsAPI->CreateHandler<AnchorCollectionResultCallback, AnchorCollectionResult, void, csp::services::DtoArray<chs::AnchorDto>>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    std::vector<csp::common::String> ReferenceIds(1);
    ReferenceIds[0] = SpaceId;

    auto AnchorLimit = Limit.HasValue() ? *Limit : std::optional<int>(std::nullopt);
    auto AnchorSkip = Skip.HasValue() ? *Skip : std::optional<int>(std::nullopt);

    static_cast<chs::AnchorsApi*>(AnchorsAPI)
        ->apiV1AnchorsGet(std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
            std::nullopt, ReferenceIds, std::nullopt, AnchorSkip, AnchorLimit, ResponseHandler, csp::common::CancellationToken::Dummy());
}

void AnchorSystem::GetAnchorsByAssetCollectionId(const csp::common::String& AssetCollectionId, const csp::common::Optional<int>& Skip,
    const csp::common::Optional<int>& Limit, AnchorCollectionResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = AnchorsAPI->CreateHandler<AnchorCollectionResultCallback, AnchorCollectionResult, void, csp::services::DtoArray<chs::AnchorDto>>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    std::vector<csp::common::String> AssetCollectionIds(1);
    AssetCollectionIds[0] = AssetCollectionId;

    auto AnchorLimit = Limit.HasValue() ? *Limit : std::optional<int>(std::nullopt);
    auto AnchorSkip = Skip.HasValue() ? *Skip : std::optional<int>(std::nullopt);

    static_cast<chs::AnchorsApi*>(AnchorsAPI)
        ->apiV1AnchorsGet(std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
            std::nullopt, std::nullopt, AssetCollectionIds, AnchorSkip, AnchorLimit, ResponseHandler, csp::common::CancellationToken::Dummy());
}

void AnchorSystem::CreateAnchorResolution(const csp::common::String& AnchorId, bool SuccessfullyResolved, int ResolveAttempted, double ResolveTime,
    const csp::common::Array<csp::common::String>& Tags, AnchorResolutionResultCallback Callback)
{
    auto AnchorResolutionInfo = std::make_shared<chs::AnchorResolutionDto>();

    AnchorResolutionInfo->SetAnchorId(AnchorId);
    AnchorResolutionInfo->SetSuccessfullyResolved(SuccessfullyResolved);
    AnchorResolutionInfo->SetResolveAttempted(ResolveAttempted);
    AnchorResolutionInfo->SetResolveTime(ResolveTime);
    AnchorResolutionInfo->SetTags(csp::common::Convert(Tags));

    csp::services::ResponseHandlerPtr ResponseHandler = AnchorsAPI->CreateHandler<csp::systems::AnchorResolutionResultCallback,
        csp::systems::AnchorResolutionResult, void, chs::AnchorResolutionDto>(Callback, nullptr);

    static_cast<chs::AnchorsApi*>(AnchorsAPI)->apiV1AnchorResolutionsPost(AnchorResolutionInfo, ResponseHandler);
}

} // namespace csp::systems
