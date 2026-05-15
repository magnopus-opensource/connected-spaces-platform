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

#include "CSP/Systems/Spatial/PointOfInterestSystem.h"

#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CallHelpers.h"
#include "Services/SpatialDataService/Api.h"
#include "Services/SpatialDataService/Dto.h"
#include "Systems/ResultHelpers.h"
#include "Systems/Spatial/PointOfInterestHelpers.h"

namespace chs = csp::services::generated::spatialdataservice;

namespace csp::systems
{

const char* ENGLISH_LANGUAGE_CODE = "EN";

PointOfInterestSystem::PointOfInterestSystem(csp::common::LogSystem& logSystem)
    : SystemBase(nullptr, nullptr, &logSystem)
    , m_poiApiPtr(nullptr)
{
}

PointOfInterestSystem::PointOfInterestSystem(csp::web::WebClient* inWebClient, csp::common::LogSystem& logSystem)
    : SystemBase(inWebClient, nullptr, &logSystem)
{
    m_poiApiPtr = new chs::PointOfInterestApi(inWebClient);
}

PointOfInterestSystem::~PointOfInterestSystem() { delete (m_poiApiPtr); }

CSP_ASYNC_RESULT void PointOfInterestSystem::CreatePOI(const csp::common::String& title, const csp::common::String& description,
    const csp::common::String& name, const csp::common::Optional<csp::common::Array<csp::common::String>>& tags, EPointOfInterestType /*Type*/,
    const csp::common::String& owner, const csp::systems::GeoLocation& location, const AssetCollection& assetCollection, POIResultCallback callback)
{
    auto poiInfo = std::make_shared<chs::PointOfInterestDto>();

    chs::LocalizedString poiTitle;
    poiTitle.SetValue(title);
    poiTitle.SetLanguageCode(ENGLISH_LANGUAGE_CODE);
    std::vector<std::shared_ptr<chs::LocalizedString>> dtoTitles;
    dtoTitles.push_back(std::make_shared<chs::LocalizedString>(poiTitle));
    poiInfo->SetTitle(dtoTitles);

    chs::LocalizedString poiDescription;
    poiDescription.SetValue(description);
    poiDescription.SetLanguageCode(ENGLISH_LANGUAGE_CODE);
    std::vector<std::shared_ptr<chs::LocalizedString>> dtoDescriptions;
    dtoDescriptions.push_back(std::make_shared<chs::LocalizedString>(poiDescription));
    poiInfo->SetDescription(dtoDescriptions);

    poiInfo->SetName(name);

    if (tags.HasValue())
    {
        std::vector<csp::common::String> dtoTags;
        dtoTags.reserve(tags->Size());

        for (size_t idx = 0; idx < tags->Size(); idx++)
        {
            dtoTags.push_back((*tags)[idx]);
        }

        poiInfo->SetTags(dtoTags);
    }

    const csp::common::String typeString = PointOfInterestHelpers::TypeToString(EPointOfInterestType::DEFAULT);
    poiInfo->SetType(typeString);

    poiInfo->SetOwner(owner);

    auto coordinates = std::make_shared<chs::GeoCoord>();
    coordinates->SetLatitude(location.Latitude);
    coordinates->SetLongitude(location.Longitude);
    poiInfo->SetLocation(coordinates);

    poiInfo->SetPrototypeName(assetCollection.Id);

    csp::services::ResponseHandlerPtr responseHandler = m_poiApiPtr->CreateHandler<POIResultCallback, POIResult, void, chs::PointOfInterestDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::PointOfInterestApi*>(m_poiApiPtr)->poiPost({ poiInfo }, responseHandler);
}

void PointOfInterestSystem::DeletePOI(const PointOfInterest& poi, NullResultCallback callback)
{
    const csp::common::String poiId = poi.Id;

    DeletePOIInternal(poiId, callback);
}

void PointOfInterestSystem::GetPOIsInArea(const csp::systems::GeoLocation& originLocation, const double areaRadius,
    const csp::common::Optional<EPointOfInterestType>& type, POICollectionResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_poiApiPtr->CreateHandler<POICollectionResultCallback, POICollectionResult, void, csp::services::DtoArray<chs::PointOfInterestDto>>(
            callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    // If the user has provided a type of POI to search for, prepare the corresponding search term string.
    // Otherwise, leave the term as null, to search for all POI types.
    std::optional<csp::services::utility::string_t> typeOption = std::nullopt;
    if (type.HasValue())
    {
        typeOption = PointOfInterestHelpers::TypeToString(*type).c_str();
    }

    static_cast<chs::PointOfInterestApi*>(m_poiApiPtr)->poiGet(
        { std::nullopt, std::nullopt, typeOption, std::nullopt, std::nullopt, std::nullopt, originLocation.Longitude, originLocation.Latitude,
            areaRadius, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
            std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt },
        responseHandler);
}

CSP_ASYNC_RESULT void PointOfInterestSystem::CreateSite(const Site& site, SiteResultCallback callback)
{
    auto poiInfo = std::make_shared<chs::PointOfInterestDto>();

    chs::LocalizedString poiTitle;
    poiTitle.SetValue(site.Name);
    poiTitle.SetLanguageCode(ENGLISH_LANGUAGE_CODE);
    std::vector<std::shared_ptr<chs::LocalizedString>> dtoTitles;
    dtoTitles.push_back(std::make_shared<chs::LocalizedString>(poiTitle));
    poiInfo->SetTitle(dtoTitles);

    // the POI Name needs to be unique
    csp::common::String uniqueName = site.Name;
    uniqueName.Append("_");
    uniqueName.Append(site.SpaceId);
    poiInfo->SetName(uniqueName);

    const csp::common::String typeString = PointOfInterestHelpers::TypeToString(EPointOfInterestType::DEFAULT);
    poiInfo->SetType(typeString);

    poiInfo->SetOwner(site.SpaceId);

    auto coordinates = std::make_shared<chs::GeoCoord>();
    coordinates->SetLatitude(site.Location.Latitude);
    coordinates->SetLongitude(site.Location.Longitude);
    poiInfo->SetLocation(coordinates);

    auto dtoSiteTransform = std::make_shared<chs::Transform>();
    auto dtoSiteRotation = std::make_shared<chs::Rotation>();
    dtoSiteRotation->SetX(static_cast<float>(site.Rotation.X));
    dtoSiteRotation->SetY(static_cast<float>(site.Rotation.Y));
    dtoSiteRotation->SetZ(static_cast<float>(site.Rotation.Z));
    dtoSiteRotation->SetW(static_cast<float>(site.Rotation.W));
    dtoSiteTransform->SetRotation(dtoSiteRotation);
    poiInfo->SetPrototypeTransform(dtoSiteTransform);

    poiInfo->SetGroupId(site.SpaceId);

    csp::services::ResponseHandlerPtr responseHandler = m_poiApiPtr->CreateHandler<SiteResultCallback, SiteResult, void, chs::PointOfInterestDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::PointOfInterestApi*>(m_poiApiPtr)->poiPost({ poiInfo }, responseHandler);
}

void PointOfInterestSystem::DeleteSite(const Site& site, NullResultCallback callback) { DeletePOIInternal(site.Id, callback); }

void PointOfInterestSystem::GetSites(const csp::common::String& spaceId, SitesCollectionResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_poiApiPtr->CreateHandler<SitesCollectionResultCallback, SitesCollectionResult, void, csp::services::DtoArray<chs::PointOfInterestDto>>(
            callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    std::vector<csp::common::String> spaceIds({ spaceId });

    static_cast<chs::PointOfInterestApi*>(m_poiApiPtr)->poiGet(
        { std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
            std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
            std::nullopt, std::nullopt, std::nullopt, spaceIds, std::nullopt, std::nullopt },
        responseHandler);
}

void PointOfInterestSystem::DeletePOIInternal(const csp::common::String poiId, NullResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler = m_poiApiPtr->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs::PointOfInterestApi*>(m_poiApiPtr)->poiIdDelete({ poiId }, responseHandler);
}

void PointOfInterestSystem::AddSpaceGeoLocation(const csp::common::String& spaceId, const csp::common::Optional<GeoLocation>& location,
    const csp::common::Optional<float>& orientation, const csp::common::Optional<csp::common::Array<GeoLocation>>& geoFence,
    SpaceGeoLocationResultCallback callback)
{
    auto poiInfo = std::make_shared<chs::PointOfInterestDto>();

    const csp::common::String typeString = PointOfInterestHelpers::TypeToString(EPointOfInterestType::SPACE);
    poiInfo->SetType(typeString);

    chs::LocalizedString poiTitle;
    poiTitle.SetValue(typeString);
    poiTitle.SetLanguageCode(ENGLISH_LANGUAGE_CODE);
    std::vector<std::shared_ptr<chs::LocalizedString>> dtoTitles;
    dtoTitles.push_back(std::make_shared<chs::LocalizedString>(poiTitle));
    poiInfo->SetTitle(dtoTitles);

    // the POI Name needs to be unique
    csp::common::String uniqueName = typeString;
    uniqueName.Append("_");
    uniqueName.Append(spaceId);
    poiInfo->SetName(uniqueName);

    poiInfo->SetGroupId(spaceId);

    if (location.HasValue())
    {
        if (!location->IsValid())
        {
            CSP_LOG_ERROR_FORMAT("Invalid GeoLocation. Latitude(-90<>90): %f, Longitude(-180<>180): %f", location->Latitude, location->Longitude);

            INVOKE_IF_NOT_NULL(callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        auto coordinates = std::make_shared<chs::GeoCoord>();
        coordinates->SetLatitude(location->Latitude);
        coordinates->SetLongitude(location->Longitude);
        poiInfo->SetLocation(coordinates);
    }

    if (orientation.HasValue())
    {
        if (*orientation < 0.0f || *orientation > 360.0f)
        {
            CSP_LOG_ERROR_FORMAT("Invalid Orientation(0-360): %f", *orientation);

            INVOKE_IF_NOT_NULL(callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        poiInfo->SetOrientation(*orientation);
    }

    if (geoFence.HasValue())
    {
        const auto size = geoFence->Size();
        if (size < 4)
        {
            CSP_LOG_ERROR_FORMAT("Invalid GeoFence: Not enough points(4): %d", size);

            INVOKE_IF_NOT_NULL(callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        if (geoFence->operator[](0) != geoFence->operator[](size - 1))
        {
            CSP_LOG_ERROR_MSG("Invalid GeoFence: First and last not the same.");

            INVOKE_IF_NOT_NULL(callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        std::vector<std::shared_ptr<chs::GeoCoord>> geoCoords(geoFence->Size());

        for (size_t i = 0; i < size; ++i)
        {
            const auto& geoFenceLocation = geoFence->operator[](i);

            if (!geoFenceLocation.IsValid())
            {
                CSP_LOG_ERROR_FORMAT("Invalid GeoFence GeoLocation. Latitude(-90<>90): %f, Longitude(-180<>180): %f", geoFenceLocation.Latitude,
                    geoFenceLocation.Longitude);

                INVOKE_IF_NOT_NULL(callback, MakeInvalid<SpaceGeoLocationResult>());

                return;
            }

            auto geoCoord = std::make_shared<chs::GeoCoord>();
            geoCoord->SetLatitude(geoFenceLocation.Latitude);
            geoCoord->SetLongitude(geoFenceLocation.Longitude);
            geoCoords[i] = geoCoord;
        }

        poiInfo->SetGeofence(geoCoords);
    }

    csp::services::ResponseHandlerPtr responseHandler
        = m_poiApiPtr->CreateHandler<SpaceGeoLocationResultCallback, SpaceGeoLocationResult, void, chs::PointOfInterestDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::PointOfInterestApi*>(m_poiApiPtr)->poiPost({ poiInfo }, responseHandler);
}

void PointOfInterestSystem::UpdateSpaceGeoLocation(const csp::common::String& spaceId, const csp::common::String& spaceGeoLocationId,
    const csp::common::Optional<GeoLocation>& location, const csp::common::Optional<float>& orientation,
    const csp::common::Optional<csp::common::Array<GeoLocation>>& geoFence, SpaceGeoLocationResultCallback callback)
{
    auto poiInfo = std::make_shared<chs::PointOfInterestDto>();

    const csp::common::String typeString = PointOfInterestHelpers::TypeToString(EPointOfInterestType::SPACE);
    poiInfo->SetType(typeString);

    chs::LocalizedString poiTitle;
    poiTitle.SetValue(typeString);
    poiTitle.SetLanguageCode(ENGLISH_LANGUAGE_CODE);
    std::vector<std::shared_ptr<chs::LocalizedString>> dtoTitles;
    dtoTitles.push_back(std::make_shared<chs::LocalizedString>(poiTitle));
    poiInfo->SetTitle(dtoTitles);

    // the POI Name needs to be unique
    csp::common::String uniqueName = typeString;
    uniqueName.Append("_");
    uniqueName.Append(spaceId);
    poiInfo->SetName(uniqueName);

    poiInfo->SetGroupId(spaceId);

    if (location.HasValue())
    {
        if (!location->IsValid())
        {
            CSP_LOG_ERROR_FORMAT("Invalid GeoLocation. Latitude(-90<>90): %f, Longitude(-180<>180): %f", location->Latitude, location->Longitude);

            INVOKE_IF_NOT_NULL(callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        auto coordinates = std::make_shared<chs::GeoCoord>();
        coordinates->SetLatitude(location->Latitude);
        coordinates->SetLongitude(location->Longitude);
        poiInfo->SetLocation(coordinates);
    }

    if (orientation.HasValue())
    {
        if (*orientation < 0.0f || *orientation > 360.0f)
        {
            CSP_LOG_ERROR_FORMAT("Invalid Orientation(0-360): %f", *orientation);

            INVOKE_IF_NOT_NULL(callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        poiInfo->SetOrientation(*orientation);
    }

    if (geoFence.HasValue())
    {
        const auto size = geoFence->Size();

        if (size < 4)
        {
            CSP_LOG_ERROR_FORMAT("Invalid GeoFence: Not enough points(4): %d", size);

            INVOKE_IF_NOT_NULL(callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        if (geoFence->operator[](0) != geoFence->operator[](size - 1))
        {
            CSP_LOG_ERROR_MSG("Invalid GeoFence: First and last not the same.");

            INVOKE_IF_NOT_NULL(callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        std::vector<std::shared_ptr<chs::GeoCoord>> geoCoords(geoFence->Size());

        for (size_t i = 0; i < geoFence->Size(); ++i)
        {
            const auto geoFenceLocation = geoFence->operator[](i);

            if (!geoFenceLocation.IsValid())
            {
                CSP_LOG_ERROR_FORMAT("Invalid GeoFence GeoLocation. Latitude(-90<>90): %f, Longitude(-180<>180): %f", geoFenceLocation.Latitude,
                    geoFenceLocation.Longitude);

                INVOKE_IF_NOT_NULL(callback, MakeInvalid<SpaceGeoLocationResult>());

                return;
            }

            auto geoCoord = std::make_shared<chs::GeoCoord>();
            geoCoord->SetLatitude(geoFenceLocation.Latitude);
            geoCoord->SetLongitude(geoFenceLocation.Longitude);
            geoCoords[i] = geoCoord;
        }

        poiInfo->SetGeofence(geoCoords);
    }

    csp::services::ResponseHandlerPtr responseHandler
        = m_poiApiPtr->CreateHandler<SpaceGeoLocationResultCallback, SpaceGeoLocationResult, void, chs::PointOfInterestDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs::PointOfInterestApi*>(m_poiApiPtr)->poiIdPut({ spaceGeoLocationId, poiInfo }, responseHandler);
}

void PointOfInterestSystem::GetSpaceGeoLocation(const csp::common::String& spaceId, SpaceGeoLocationResultCallback callback)
{
    const csp::common::String spacePoiType = PointOfInterestHelpers::TypeToString(EPointOfInterestType::SPACE);

    std::vector<csp::common::String> spaceIds({ spaceId });

    auto limit = 1;

    SpaceGeoLocationCollectionResultCallback collectionCallback = [=](const SpaceGeoLocationCollectionResult result)
    {
        if (result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        SpaceGeoLocationResult geoLocationResult(result.GetResultCode(), result.GetHttpResultCode());

        if (result.GetResultCode() == csp::systems::EResultCode::Success && !result.m_geoLocations.IsEmpty())
        {
            geoLocationResult.m_geoLocation = result.m_geoLocations[0];
            geoLocationResult.m_hasGeoLocation = true;
        }

        INVOKE_IF_NOT_NULL(callback, geoLocationResult);
    };

    csp::services::ResponseHandlerPtr responseHandler
        = m_poiApiPtr->CreateHandler<SpaceGeoLocationCollectionResultCallback, SpaceGeoLocationCollectionResult, void,
            csp::services::DtoArray<chs::PointOfInterestDto>>(collectionCallback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs::PointOfInterestApi*>(m_poiApiPtr)->poiGet(
        { std::nullopt, std::nullopt, spacePoiType, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
            std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
            std::nullopt, std::nullopt, std::nullopt, spaceIds, std::nullopt, limit },
        responseHandler);
}

void PointOfInterestSystem::DeleteSpaceGeoLocation(const csp::common::String& spaceGeoLocationId, NullResultCallback callback)
{
    DeletePOIInternal(spaceGeoLocationId, callback);
}

} // namespace csp::systems
