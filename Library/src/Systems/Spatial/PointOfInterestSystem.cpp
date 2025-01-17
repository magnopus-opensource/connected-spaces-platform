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

PointOfInterestSystem::PointOfInterestSystem()
    : SystemBase(nullptr, nullptr)
    , POIApiPtr(nullptr)
{
}

PointOfInterestSystem::PointOfInterestSystem(csp::web::WebClient* InWebClient)
    : SystemBase(InWebClient, nullptr)
{
    POIApiPtr = CSP_NEW chs::PointOfInterestApi(InWebClient);
}

PointOfInterestSystem::~PointOfInterestSystem() { CSP_DELETE(POIApiPtr); }

CSP_ASYNC_RESULT void PointOfInterestSystem::CreatePOI(const csp::common::String& Title, const csp::common::String& Description,
    const csp::common::String& Name, const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags, EPointOfInterestType Type,
    const csp::common::String& Owner, const csp::systems::GeoLocation& Location, const AssetCollection& AssetCollection, POIResultCallback Callback)
{
    auto POIInfo = std::make_shared<chs::PointOfInterestDto>();

    chs::LocalizedString POITitle;
    POITitle.SetValue(Title);
    POITitle.SetLanguageCode(ENGLISH_LANGUAGE_CODE);
    std::vector<std::shared_ptr<chs::LocalizedString>> DTOTitles;
    DTOTitles.push_back(std::make_shared<chs::LocalizedString>(POITitle));
    POIInfo->SetTitle(DTOTitles);

    chs::LocalizedString POIDescription;
    POIDescription.SetValue(Description);
    POIDescription.SetLanguageCode(ENGLISH_LANGUAGE_CODE);
    std::vector<std::shared_ptr<chs::LocalizedString>> DTODescriptions;
    DTODescriptions.push_back(std::make_shared<chs::LocalizedString>(POIDescription));
    POIInfo->SetDescription(DTODescriptions);

    POIInfo->SetName(Name);

    if (Tags.HasValue())
    {
        std::vector<csp::common::String> DTOTags;
        DTOTags.reserve(Tags->Size());

        for (int idx = 0; idx < Tags->Size(); idx++)
        {
            DTOTags.push_back((*Tags)[idx]);
        }

        POIInfo->SetTags(DTOTags);
    }

    const csp::common::String TypeString = PointOfInterestHelpers::TypeToString(EPointOfInterestType::DEFAULT);
    POIInfo->SetType(TypeString);

    POIInfo->SetOwner(Owner);

    auto Coordinates = std::make_shared<chs::GeoCoord>();
    Coordinates->SetLatitude(Location.Latitude);
    Coordinates->SetLongitude(Location.Longitude);
    POIInfo->SetLocation(Coordinates);

    POIInfo->SetPrototypeName(AssetCollection.Id);

    csp::services::ResponseHandlerPtr ResponseHandler = POIApiPtr->CreateHandler<POIResultCallback, POIResult, void, chs::PointOfInterestDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::PointOfInterestApi*>(POIApiPtr)->apiV1PoiPost(POIInfo, ResponseHandler);
}

void PointOfInterestSystem::DeletePOI(const PointOfInterest& POI, NullResultCallback Callback)
{
    const csp::common::String POIId = POI.Id;

    DeletePOIInternal(POIId, Callback);
}

void PointOfInterestSystem::GetPOIsInArea(const csp::systems::GeoLocation& OriginLocation, const double AreaRadius,
    const csp::common::Optional<EPointOfInterestType>& Type, POICollectionResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = POIApiPtr->CreateHandler<POICollectionResultCallback, POICollectionResult, void, csp::services::DtoArray<chs::PointOfInterestDto>>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    // If the user has provided a type of POI to search for, prepare the corresponding search term string.
    // Otherwise, leave the term as null, to search for all POI types.
    std::optional<csp::services::utility::string_t> TypeOption = std::nullopt;
    if (Type.HasValue())
    {
        TypeOption = PointOfInterestHelpers::TypeToString(*Type).c_str();
    }

    static_cast<chs::PointOfInterestApi*>(POIApiPtr)->apiV1PoiGet(std::nullopt, std::nullopt, TypeOption, std::nullopt, std::nullopt, std::nullopt,
        OriginLocation.Longitude, OriginLocation.Latitude, AreaRadius, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, ResponseHandler);
}

CSP_ASYNC_RESULT void PointOfInterestSystem::CreateSite(const Site& Site, SiteResultCallback Callback)
{
    auto POIInfo = std::make_shared<chs::PointOfInterestDto>();

    chs::LocalizedString POITitle;
    POITitle.SetValue(Site.Name);
    POITitle.SetLanguageCode(ENGLISH_LANGUAGE_CODE);
    std::vector<std::shared_ptr<chs::LocalizedString>> DTOTitles;
    DTOTitles.push_back(std::make_shared<chs::LocalizedString>(POITitle));
    POIInfo->SetTitle(DTOTitles);

    // the POI Name needs to be unique
    csp::common::String uniqueName = Site.Name;
    uniqueName.Append("_");
    uniqueName.Append(Site.SpaceId);
    POIInfo->SetName(uniqueName);

    const csp::common::String TypeString = PointOfInterestHelpers::TypeToString(EPointOfInterestType::DEFAULT);
    POIInfo->SetType(TypeString);

    POIInfo->SetOwner(Site.SpaceId);

    auto Coordinates = std::make_shared<chs::GeoCoord>();
    Coordinates->SetLatitude(Site.Location.Latitude);
    Coordinates->SetLongitude(Site.Location.Longitude);
    POIInfo->SetLocation(Coordinates);

    auto DTOSiteTransform = std::make_shared<chs::Transform>();
    auto DTOSiteRotation = std::make_shared<chs::Rotation>();
    DTOSiteRotation->SetX(static_cast<float>(Site.Rotation.X));
    DTOSiteRotation->SetY(static_cast<float>(Site.Rotation.Y));
    DTOSiteRotation->SetZ(static_cast<float>(Site.Rotation.Z));
    DTOSiteRotation->SetW(static_cast<float>(Site.Rotation.W));
    DTOSiteTransform->SetRotation(DTOSiteRotation);
    POIInfo->SetPrototypeTransform(DTOSiteTransform);

    POIInfo->SetGroupId(Site.SpaceId);

    csp::services::ResponseHandlerPtr ResponseHandler = POIApiPtr->CreateHandler<SiteResultCallback, SiteResult, void, chs::PointOfInterestDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::PointOfInterestApi*>(POIApiPtr)->apiV1PoiPost(POIInfo, ResponseHandler);
}

void PointOfInterestSystem::DeleteSite(const Site& Site, NullResultCallback Callback) { DeletePOIInternal(Site.Id, Callback); }

void PointOfInterestSystem::GetSites(const csp::common::String& SpaceId, SitesCollectionResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = POIApiPtr->CreateHandler<SitesCollectionResultCallback, SitesCollectionResult, void, csp::services::DtoArray<chs::PointOfInterestDto>>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    std::vector<csp::common::String> SpaceID({ SpaceId });

    static_cast<chs::PointOfInterestApi*>(POIApiPtr)->apiV1PoiGet(std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, SpaceID, std::nullopt, std::nullopt, ResponseHandler);
}

void PointOfInterestSystem::DeletePOIInternal(const csp::common::String POIId, NullResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler = POIApiPtr->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs::PointOfInterestApi*>(POIApiPtr)->apiV1PoiIdDelete(POIId, ResponseHandler);
}

void PointOfInterestSystem::AddSpaceGeoLocation(const csp::common::String& SpaceId, const csp::common::Optional<GeoLocation>& Location,
    const csp::common::Optional<float>& Orientation, const csp::common::Optional<csp::common::Array<GeoLocation>>& GeoFence,
    SpaceGeoLocationResultCallback Callback)
{
    auto POIInfo = std::make_shared<chs::PointOfInterestDto>();

    const csp::common::String TypeString = PointOfInterestHelpers::TypeToString(EPointOfInterestType::SPACE);
    POIInfo->SetType(TypeString);

    chs::LocalizedString POITitle;
    POITitle.SetValue(TypeString);
    POITitle.SetLanguageCode(ENGLISH_LANGUAGE_CODE);
    std::vector<std::shared_ptr<chs::LocalizedString>> DTOTitles;
    DTOTitles.push_back(std::make_shared<chs::LocalizedString>(POITitle));
    POIInfo->SetTitle(DTOTitles);

    // the POI Name needs to be unique
    csp::common::String uniqueName = TypeString;
    uniqueName.Append("_");
    uniqueName.Append(SpaceId);
    POIInfo->SetName(uniqueName);

    POIInfo->SetGroupId(SpaceId);

    if (Location.HasValue())
    {
        if (!Location->IsValid())
        {
            CSP_LOG_ERROR_FORMAT("Invalid GeoLocation. Latitude(-90<>90): %f, Longitude(-180<>180): %f", Location->Latitude, Location->Longitude);

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        auto Coordinates = std::make_shared<chs::GeoCoord>();
        Coordinates->SetLatitude(Location->Latitude);
        Coordinates->SetLongitude(Location->Longitude);
        POIInfo->SetLocation(Coordinates);
    }

    if (Orientation.HasValue())
    {
        if (*Orientation < 0.0f || *Orientation > 360.0f)
        {
            CSP_LOG_ERROR_FORMAT("Invalid Orientation(0-360): %f", *Orientation);

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        POIInfo->SetOrientation(*Orientation);
    }

    if (GeoFence.HasValue())
    {
        const auto Size = GeoFence->Size();
        if (Size < 4)
        {
            CSP_LOG_ERROR_FORMAT("Invalid GeoFence: Not enough points(4): %d", Size);

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        if (GeoFence->operator[](0) != GeoFence->operator[](Size - 1))
        {
            CSP_LOG_ERROR_MSG("Invalid GeoFence: First and last not the same.");

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        std::vector<std::shared_ptr<chs::GeoCoord>> GeoCoords(GeoFence->Size());

        for (size_t i = 0; i < Size; ++i)
        {
            const auto& GeoFenceLocation = GeoFence->operator[](i);

            if (!GeoFenceLocation.IsValid())
            {
                CSP_LOG_ERROR_FORMAT("Invalid GeoFence GeoLocation. Latitude(-90<>90): %f, Longitude(-180<>180): %f", GeoFenceLocation.Latitude,
                    GeoFenceLocation.Longitude);

                INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SpaceGeoLocationResult>());

                return;
            }

            auto GeoCoord = std::make_shared<chs::GeoCoord>();
            GeoCoord->SetLatitude(GeoFenceLocation.Latitude);
            GeoCoord->SetLongitude(GeoFenceLocation.Longitude);
            GeoCoords[i] = GeoCoord;
        }

        POIInfo->SetGeofence(GeoCoords);
    }

    csp::services::ResponseHandlerPtr ResponseHandler
        = POIApiPtr->CreateHandler<SpaceGeoLocationResultCallback, SpaceGeoLocationResult, void, chs::PointOfInterestDto>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::PointOfInterestApi*>(POIApiPtr)->apiV1PoiPost(POIInfo, ResponseHandler);
}

void PointOfInterestSystem::UpdateSpaceGeoLocation(const csp::common::String& SpaceId, const csp::common::String& SpaceGeoLocationId,
    const csp::common::Optional<GeoLocation>& Location, const csp::common::Optional<float>& Orientation,
    const csp::common::Optional<csp::common::Array<GeoLocation>>& GeoFence, SpaceGeoLocationResultCallback Callback)
{
    auto POIInfo = std::make_shared<chs::PointOfInterestDto>();

    const csp::common::String TypeString = PointOfInterestHelpers::TypeToString(EPointOfInterestType::SPACE);
    POIInfo->SetType(TypeString);

    chs::LocalizedString POITitle;
    POITitle.SetValue(TypeString);
    POITitle.SetLanguageCode(ENGLISH_LANGUAGE_CODE);
    std::vector<std::shared_ptr<chs::LocalizedString>> DTOTitles;
    DTOTitles.push_back(std::make_shared<chs::LocalizedString>(POITitle));
    POIInfo->SetTitle(DTOTitles);

    // the POI Name needs to be unique
    csp::common::String uniqueName = TypeString;
    uniqueName.Append("_");
    uniqueName.Append(SpaceId);
    POIInfo->SetName(uniqueName);

    POIInfo->SetGroupId(SpaceId);

    if (Location.HasValue())
    {
        if (!Location->IsValid())
        {
            CSP_LOG_ERROR_FORMAT("Invalid GeoLocation. Latitude(-90<>90): %f, Longitude(-180<>180): %f", Location->Latitude, Location->Longitude);

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        auto Coordinates = std::make_shared<chs::GeoCoord>();
        Coordinates->SetLatitude(Location->Latitude);
        Coordinates->SetLongitude(Location->Longitude);
        POIInfo->SetLocation(Coordinates);
    }

    if (Orientation.HasValue())
    {
        if (*Orientation < 0.0f || *Orientation > 360.0f)
        {
            CSP_LOG_ERROR_FORMAT("Invalid Orientation(0-360): %f", *Orientation);

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        POIInfo->SetOrientation(*Orientation);
    }

    if (GeoFence.HasValue())
    {
        const auto Size = GeoFence->Size();

        if (Size < 4)
        {
            CSP_LOG_ERROR_FORMAT("Invalid GeoFence: Not enough points(4): %d", Size);

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        if (GeoFence->operator[](0) != GeoFence->operator[](Size - 1))
        {
            CSP_LOG_ERROR_MSG("Invalid GeoFence: First and last not the same.");

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SpaceGeoLocationResult>());

            return;
        }

        std::vector<std::shared_ptr<chs::GeoCoord>> GeoCoords(GeoFence->Size());

        for (size_t i = 0; i < GeoFence->Size(); ++i)
        {
            const auto GeoFenceLocation = GeoFence->operator[](i);

            if (!GeoFenceLocation.IsValid())
            {
                CSP_LOG_ERROR_FORMAT("Invalid GeoFence GeoLocation. Latitude(-90<>90): %f, Longitude(-180<>180): %f", GeoFenceLocation.Latitude,
                    GeoFenceLocation.Longitude);

                INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SpaceGeoLocationResult>());

                return;
            }

            auto GeoCoord = std::make_shared<chs::GeoCoord>();
            GeoCoord->SetLatitude(GeoFenceLocation.Latitude);
            GeoCoord->SetLongitude(GeoFenceLocation.Longitude);
            GeoCoords[i] = GeoCoord;
        }

        POIInfo->SetGeofence(GeoCoords);
    }

    csp::services::ResponseHandlerPtr ResponseHandler
        = POIApiPtr->CreateHandler<SpaceGeoLocationResultCallback, SpaceGeoLocationResult, void, chs::PointOfInterestDto>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs::PointOfInterestApi*>(POIApiPtr)->apiV1PoiIdPut(SpaceGeoLocationId, POIInfo, ResponseHandler);
}

void PointOfInterestSystem::GetSpaceGeoLocation(const csp::common::String& SpaceId, SpaceGeoLocationResultCallback Callback)
{
    const csp::common::String SpacePOIType = PointOfInterestHelpers::TypeToString(EPointOfInterestType::SPACE);

    std::vector<csp::common::String> SpaceIds({ SpaceId });

    auto Limit = 1;

    SpaceGeoLocationCollectionResultCallback CollectionCallback = [=](const SpaceGeoLocationCollectionResult Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        SpaceGeoLocationResult GeoLocationResult(Result.GetResultCode(), Result.GetHttpResultCode());

        if (Result.GetResultCode() == csp::systems::EResultCode::Success && !Result.GeoLocations.IsEmpty())
        {
            GeoLocationResult.GeoLocation = Result.GeoLocations[0];
            GeoLocationResult.HasGeoLocation = true;
        }

        INVOKE_IF_NOT_NULL(Callback, GeoLocationResult);
    };

    csp::services::ResponseHandlerPtr ResponseHandler
        = POIApiPtr->CreateHandler<SpaceGeoLocationCollectionResultCallback, SpaceGeoLocationCollectionResult, void,
            csp::services::DtoArray<chs::PointOfInterestDto>>(CollectionCallback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs::PointOfInterestApi*>(POIApiPtr)->apiV1PoiGet(std::nullopt, std::nullopt, SpacePOIType, std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, SpaceIds, std::nullopt, Limit, ResponseHandler);
}

void PointOfInterestSystem::DeleteSpaceGeoLocation(const csp::common::String& SpaceGeoLocationId, NullResultCallback Callback)
{
    DeletePOIInternal(SpaceGeoLocationId, Callback);
}

} // namespace csp::systems
