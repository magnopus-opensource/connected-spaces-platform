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
    : SystemBase(nullptr, nullptr, nullptr)
    , m_anchorsApi(nullptr)
{
}

AnchorSystem::AnchorSystem(csp::web::WebClient* inWebClient, csp::common::LogSystem& logSystem)
    : SystemBase(inWebClient, nullptr, &logSystem)
{
    m_anchorsApi = new chs::AnchorsApi(inWebClient);
}

AnchorSystem::~AnchorSystem() { delete (m_anchorsApi); }

void AnchorSystem::CreateAnchor(csp::systems::AnchorProvider thirdPartyAnchorProvider, const csp::common::String& thirdPartyAnchorId,
    const csp::common::String& assetCollectionId, const csp::systems::GeoLocation& location, const csp::systems::OlyAnchorPosition& position,
    const csp::systems::OlyRotation& rotation,
    const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& spatialKeyValue,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags, AnchorResultCallback callback)
{
    auto anchorInfo = std::make_shared<chs::AnchorDto>();

    switch (thirdPartyAnchorProvider)
    {
    case AnchorProvider::GoogleCloudAnchors:
        anchorInfo->SetThirdPartyProviderName("GoogleCloudAnchors");
        break;
    default:
        CSP_LOG_WARN_MSG("Unknown third party anchor provider");
        break;
    }

    anchorInfo->SetThirdPartyAnchorId(thirdPartyAnchorId);
    anchorInfo->SetAnchoredPrototypeId(assetCollectionId);

    auto dtoLocation = std::make_shared<chs::GeoCoord>();
    dtoLocation->SetLatitude(location.Latitude);
    dtoLocation->SetLongitude(location.Longitude);
    anchorInfo->SetLocation(dtoLocation);

    auto dtoPosition = std::make_shared<chs::AnchorPosition>();
    dtoPosition->SetX(position.X);
    dtoPosition->SetY(position.Y);
    dtoPosition->SetZ(position.Z);
    anchorInfo->SetPosition(dtoPosition);

    auto dtoRotation = std::make_shared<chs::AnchorRotation>();
    dtoRotation->SetX(rotation.X);
    dtoRotation->SetY(rotation.Y);
    dtoRotation->SetZ(rotation.Z);
    dtoRotation->SetW(rotation.W);
    anchorInfo->SetRotation(dtoRotation);

    // SpatialData must be set with or without data
    std::map<csp::common::String, csp::common::String> spatialData;

    if (spatialKeyValue.HasValue())
    {
        auto* keys = spatialKeyValue->Keys();

        for (size_t idx = 0; idx < keys->Size(); ++idx)
        {
            auto key = keys->operator[](idx);
            auto value = spatialKeyValue->operator[](key);
            spatialData.insert(std::pair<csp::common::String, csp::common::String>(key, value));
        }
    }

    anchorInfo->SetSpatialKeyValue(spatialData);

    if (tags.HasValue())
    {
        std::vector<csp::common::String> tagsVector;
        tagsVector.reserve(tags->Size());

        for (size_t idx = 0; idx < tags->Size(); ++idx)
        {
            tagsVector.push_back((*tags)[idx]);
        }

        anchorInfo->SetTags(tagsVector);
    }

    csp::services::ResponseHandlerPtr responseHandler = m_anchorsApi->CreateHandler<AnchorResultCallback, AnchorResult, void, chs::AnchorDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::AnchorsApi*>(m_anchorsApi)->anchorsPost({ anchorInfo }, responseHandler);
}

void AnchorSystem::CreateAnchorInSpace(csp::systems::AnchorProvider thirdPartyAnchorProvider, const csp::common::String& thirdPartyAnchorId,
    const csp::common::String& spaceId, uint64_t spaceEntityId, const csp::common::String& assetCollectionId,
    const csp::systems::GeoLocation& location, const csp::systems::OlyAnchorPosition& position, const csp::systems::OlyRotation& rotation,
    const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& spatialKeyValue,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags, AnchorResultCallback callback)
{
    auto anchorInfo = std::make_shared<chs::AnchorDto>();

    switch (thirdPartyAnchorProvider)
    {
    case AnchorProvider::GoogleCloudAnchors:
        anchorInfo->SetThirdPartyProviderName("GoogleCloudAnchors");
        break;
    default:
        CSP_LOG_WARN_MSG("Unknown third party anchor provider");
        break;
    }

    anchorInfo->SetThirdPartyAnchorId(thirdPartyAnchorId);
    anchorInfo->SetReferenceId(spaceId);
    anchorInfo->SetAnchoredMultiplayerObjectId(static_cast<int32_t>(spaceEntityId));
    anchorInfo->SetAnchoredPrototypeId(assetCollectionId);

    auto dtoLocation = std::make_shared<chs::GeoCoord>();
    dtoLocation->SetLatitude(location.Latitude);
    dtoLocation->SetLongitude(location.Longitude);
    anchorInfo->SetLocation(dtoLocation);

    auto dtoPosition = std::make_shared<chs::AnchorPosition>();
    dtoPosition->SetX(position.X);
    dtoPosition->SetY(position.Y);
    dtoPosition->SetZ(position.Z);
    anchorInfo->SetPosition(dtoPosition);

    auto dtoRotation = std::make_shared<chs::AnchorRotation>();
    dtoRotation->SetX(rotation.X);
    dtoRotation->SetY(rotation.Y);
    dtoRotation->SetZ(rotation.Z);
    dtoRotation->SetW(rotation.W);
    anchorInfo->SetRotation(dtoRotation);

    // SpatialData must be set with or without data
    std::map<csp::common::String, csp::common::String> spatialData;

    if (spatialKeyValue.HasValue())
    {
        auto* keys = spatialKeyValue->Keys();

        for (size_t idx = 0; idx < keys->Size(); ++idx)
        {
            auto key = keys->operator[](idx);
            auto value = spatialKeyValue->operator[](key);
            spatialData.insert(std::pair<csp::common::String, csp::common::String>(key, value));
        }
    }

    anchorInfo->SetSpatialKeyValue(spatialData);

    if (tags.HasValue())
    {
        std::vector<csp::common::String> tagsVector;
        tagsVector.reserve(tags->Size());

        for (size_t idx = 0; idx < tags->Size(); ++idx)
        {
            tagsVector.push_back((*tags)[idx]);
        }

        anchorInfo->SetTags(tagsVector);
    }

    csp::services::ResponseHandlerPtr responseHandler = m_anchorsApi->CreateHandler<AnchorResultCallback, AnchorResult, void, chs::AnchorDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::AnchorsApi*>(m_anchorsApi)->anchorsPost({ anchorInfo }, responseHandler);
}

void AnchorSystem::DeleteAnchors(const csp::common::Array<csp::common::String>& anchorIds, NullResultCallback callback)
{
    std::vector<csp::common::String> idsToBeDeleted;
    idsToBeDeleted.reserve(anchorIds.Size());

    for (size_t idx = 0; idx < anchorIds.Size(); idx++)
    {
        idsToBeDeleted.push_back(anchorIds[idx]);
    }

    csp::services::ResponseHandlerPtr responseHandler = m_anchorsApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs::AnchorsApi*>(m_anchorsApi)->anchorsDelete({ idsToBeDeleted }, responseHandler);
}

void AnchorSystem::GetAnchorsInArea(const csp::systems::GeoLocation& originLocation, const double areaRadius,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& spatialKeys,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& spatialValues,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags, const csp::common::Optional<bool>& allTags,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& spaceIds, const csp::common::Optional<int>& skip,
    const csp::common::Optional<int>& limit, AnchorCollectionResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_anchorsApi->CreateHandler<AnchorCollectionResultCallback, AnchorCollectionResult, void, csp::services::DtoArray<chs::AnchorDto>>(
            callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    std::optional<std::vector<csp::common::String>> anchorTags;

    if (tags.HasValue())
    {
        anchorTags.emplace(std::vector<csp::common::String>());
        anchorTags->reserve(tags->Size());

        for (size_t idx = 0; idx < tags->Size(); ++idx)
        {
            anchorTags->push_back({ (*tags)[idx] });
        }
    }

    std::optional<std::vector<csp::common::String>> anchorSpatialKeys;

    if (spatialKeys.HasValue())
    {
        anchorSpatialKeys.emplace(std::vector<csp::common::String>());
        anchorSpatialKeys->reserve(spatialKeys->Size());

        for (size_t idx = 0; idx < spatialKeys->Size(); ++idx)
        {
            anchorSpatialKeys->push_back({ (*spatialKeys)[idx] });
        }
    }

    std::optional<std::vector<csp::common::String>> anchorSpatialValues;

    if (spatialValues.HasValue())
    {
        anchorSpatialValues.emplace(std::vector<csp::common::String>());
        anchorSpatialValues->reserve(spatialValues->Size());

        for (size_t idx = 0; idx < spatialValues->Size(); ++idx)
        {
            anchorSpatialValues->push_back({ (*spatialValues)[idx] });
        }
    }

    std::optional<std::vector<csp::common::String>> referenceIds;

    if (spaceIds.HasValue())
    {
        referenceIds.emplace(std::vector<csp::common::String>());
        referenceIds->reserve(spaceIds->Size());

        for (size_t idx = 0; idx < spaceIds->Size(); ++idx)
        {
            referenceIds->push_back({ (*spaceIds)[idx] });
        }
    }

    auto anchorTagsAll = allTags.HasValue() ? *allTags : std::optional<bool>(std::nullopt);
    auto anchorLimit = limit.HasValue() ? *limit : std::optional<int>(std::nullopt);
    auto anchorSkip = skip.HasValue() ? *skip : std::optional<int>(std::nullopt);

    static_cast<chs::AnchorsApi*>(m_anchorsApi)
        ->anchorsGet({ anchorSpatialKeys, anchorSpatialValues, originLocation.Longitude, originLocation.Latitude, areaRadius, anchorTags,
                         anchorTagsAll, std::nullopt, std::nullopt, referenceIds, std::nullopt, anchorSkip, anchorLimit },
            responseHandler, csp::common::CancellationToken::Dummy());
}

void AnchorSystem::GetAnchorsInSpace(const csp::common::String& spaceId, const csp::common::Optional<int>& skip,
    const csp::common::Optional<int>& limit, AnchorCollectionResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_anchorsApi->CreateHandler<AnchorCollectionResultCallback, AnchorCollectionResult, void, csp::services::DtoArray<chs::AnchorDto>>(
            callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    std::vector<csp::common::String> referenceIds(1);
    referenceIds[0] = spaceId;

    auto anchorLimit = limit.HasValue() ? *limit : std::optional<int>(std::nullopt);
    auto anchorSkip = skip.HasValue() ? *skip : std::optional<int>(std::nullopt);

    static_cast<chs::AnchorsApi*>(m_anchorsApi)
        ->anchorsGet({ std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                         referenceIds, std::nullopt, anchorSkip, anchorLimit },
            responseHandler, csp::common::CancellationToken::Dummy());
}

void AnchorSystem::GetAnchorsByAssetCollectionId(const csp::common::String& assetCollectionId, const csp::common::Optional<int>& skip,
    const csp::common::Optional<int>& limit, AnchorCollectionResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_anchorsApi->CreateHandler<AnchorCollectionResultCallback, AnchorCollectionResult, void, csp::services::DtoArray<chs::AnchorDto>>(
            callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    std::vector<csp::common::String> assetCollectionIds(1);
    assetCollectionIds[0] = assetCollectionId;

    auto anchorLimit = limit.HasValue() ? *limit : std::optional<int>(std::nullopt);
    auto anchorSkip = skip.HasValue() ? *skip : std::optional<int>(std::nullopt);

    static_cast<chs::AnchorsApi*>(m_anchorsApi)
        ->anchorsGet({ std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                         std::nullopt, assetCollectionIds, anchorSkip, anchorLimit },
            responseHandler, csp::common::CancellationToken::Dummy());
}

void AnchorSystem::CreateAnchorResolution(const csp::common::String& anchorId, bool successfullyResolved, int resolveAttempted, double resolveTime,
    const csp::common::Array<csp::common::String>& tags, AnchorResolutionResultCallback callback)
{
    auto anchorResolutionInfo = std::make_shared<chs::AnchorResolutionDto>();

    anchorResolutionInfo->SetAnchorId(anchorId);
    anchorResolutionInfo->SetSuccessfullyResolved(successfullyResolved);
    anchorResolutionInfo->SetResolveAttempted(resolveAttempted);
    anchorResolutionInfo->SetResolveTime(resolveTime);
    anchorResolutionInfo->SetTags(csp::common::Convert(tags));

    csp::services::ResponseHandlerPtr responseHandler = m_anchorsApi->CreateHandler<csp::systems::AnchorResolutionResultCallback,
        csp::systems::AnchorResolutionResult, void, chs::AnchorResolutionDto>(callback, nullptr);

    static_cast<chs::AnchorsApi*>(m_anchorsApi)->anchor_resolutionsPost({ anchorResolutionInfo }, responseHandler);
}

} // namespace csp::systems
