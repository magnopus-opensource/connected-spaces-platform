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
#include "CSP/Systems/EventTicketing/EventTicketingSystem.h"

#include "CSP/Systems/Users/UserSystem.h"
#include "Services/AggregationService/Api.h"
#include "Services/AggregationService/Dto.h"

namespace chs = csp::services::generated::aggregationservice;

csp::common::String GetVendorNameString(const csp::systems::EventTicketingVendor& vendor)
{
    switch (vendor)
    {
    case csp::systems::EventTicketingVendor::Eventbrite:
        return "eventbrite";
    default:
        CSP_LOG_WARN_MSG("Unknown ticketed event vendor");
        return "Unknown";
    }
}

namespace csp::systems
{

EventTicketingSystem::EventTicketingSystem(csp::web::WebClient* inWebClient, csp::common::LogSystem& logSystem)
    : SystemBase(inWebClient, nullptr, &logSystem)
{
    m_eventTicketingApi = new chs::TicketedSpaceApi(inWebClient);
}

EventTicketingSystem::~EventTicketingSystem() { delete (m_eventTicketingApi); }

void EventTicketingSystem::CreateTicketedEvent(const csp::common::String& spaceId, EventTicketingVendor vendor,
    const csp::common::String& vendorEventId, const csp::common::String& vendorEventUri, bool isTicketingActive, TicketedEventResultCallback callback)
{
    auto request = std::make_shared<chs::SpaceEventDto>();

    request->SetVendorName(GetVendorNameString(vendor));
    request->SetVendorEventId(vendorEventId);
    request->SetVendorEventUri(vendorEventUri);
    request->SetIsTicketingActive(isTicketingActive);

    csp::services::ResponseHandlerPtr responseHandler
        = m_eventTicketingApi->CreateHandler<TicketedEventResultCallback, TicketedEventResult, void, chs::SpaceEventDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::TicketedSpaceApi*>(m_eventTicketingApi)->spacesSpaceIdEventsPost({ spaceId, request }, responseHandler);
}

void EventTicketingSystem::UpdateTicketedEvent(const csp::common::String& spaceId, const csp::common::String& eventId,
    const csp::common::Optional<EventTicketingVendor>& vendor, const csp::common::Optional<csp::common::String>& vendorEventId,
    const csp::common::Optional<csp::common::String>& vendorEventUri, const csp::common::Optional<bool>& isTicketingActive,
    TicketedEventResultCallback callback)
{
    auto request = std::make_shared<chs::SpaceEventDto>();

    if (vendor.HasValue())
    {
        request->SetVendorName(GetVendorNameString(*vendor));
    }

    if (vendorEventId.HasValue())
    {
        request->SetVendorEventId(*vendorEventId);
    }

    if (vendorEventUri.HasValue())
    {
        request->SetVendorEventUri(*vendorEventUri);
    }

    if (isTicketingActive.HasValue())
    {
        request->SetIsTicketingActive(*isTicketingActive);
    }

    csp::services::ResponseHandlerPtr responseHandler
        = m_eventTicketingApi->CreateHandler<TicketedEventResultCallback, TicketedEventResult, void, chs::SpaceEventDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::TicketedSpaceApi*>(m_eventTicketingApi)->spacesSpaceIdEventsEventIdPut({ spaceId, eventId, request }, responseHandler);
}

void EventTicketingSystem::GetTicketedEvents(const csp::common::Array<csp::common::String>& spaceIds, const csp::common::Optional<int>& skip,
    const csp::common::Optional<int>& limit, TicketedEventCollectionResultCallback callback)
{
    std::vector<csp::common::String> requestSpaceIds;
    requestSpaceIds.reserve(spaceIds.Size());

    for (size_t i = 0; i < spaceIds.Size(); ++i)
    {
        requestSpaceIds.push_back(spaceIds[i]);
    }

    csp::services::ResponseHandlerPtr responseHandler
        = m_eventTicketingApi->CreateHandler<TicketedEventCollectionResultCallback, TicketedEventCollectionResult, void,
            csp::services::DtoArray<chs::SpaceEventDto>>(callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    const auto requestSkip = skip.HasValue() ? *skip : std::optional<int>(std::nullopt);
    const auto requestLimit = limit.HasValue() ? *limit : std::optional<int>(std::nullopt);

    static_cast<chs::TicketedSpaceApi*>(m_eventTicketingApi)
        ->spacesEventsGet(
            {
                std::nullopt, // VendorEventIds
                std::nullopt, // VendorName
                requestSpaceIds, // SpaceIds
                std::nullopt, // UserIds
                std::nullopt, // IsTicketingActive
                requestSkip, // Skip
                requestLimit // Limit
            },
            responseHandler);
}

void EventTicketingSystem::SubmitEventTicket(const csp::common::String& spaceId, EventTicketingVendor vendor,
    const csp::common::String& vendorEventId, const csp::common::String& vendorTicketId,
    const csp::common::Optional<csp::common::String>& onBehalfOfUserId, EventTicketResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_eventTicketingApi->CreateHandler<EventTicketResultCallback, EventTicketResult, void, chs::SpaceTicketDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    std::optional<csp::common::String> requestOnBehalfOfUserId = std::nullopt;
    if (onBehalfOfUserId.HasValue())
    {
        requestOnBehalfOfUserId = *onBehalfOfUserId;
    }

    static_cast<chs::TicketedSpaceApi*>(m_eventTicketingApi)
        ->spacesSpaceIdVendorsVendorNameEventsVendorEventIdTicketsVendorTicketIdPut(
            { spaceId, GetVendorNameString(vendor), vendorEventId, vendorTicketId, requestOnBehalfOfUserId }, responseHandler);
}

void EventTicketingSystem::GetVendorAuthorizeInfo(
    EventTicketingVendor vendor, const csp::common::String& userId, TicketedEventVendorAuthorizeInfoCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler = m_eventTicketingApi->CreateHandler<TicketedEventVendorAuthorizeInfoCallback,
        TicketedEventVendorAuthInfoResult, void, chs::VendorProviderInfo>(callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::TicketedSpaceApi*>(m_eventTicketingApi)
        ->vendorsVendorNameUsersUserIdProvider_infoGet({ GetVendorNameString(vendor), userId, std::nullopt }, responseHandler);
}

void EventTicketingSystem::GetIsSpaceTicketed(const csp::common::String& spaceId, SpaceIsTicketedResultCallback callback)
{

    csp::services::ResponseHandlerPtr responseHandler
        = m_eventTicketingApi->CreateHandler<SpaceIsTicketedResultCallback, SpaceIsTicketedResult, void, csp::services::DtoArray<chs::StringDataPage>>(
            callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    std::vector<csp::common::String> requestSpaceId;
    requestSpaceId.push_back(spaceId);

    static_cast<chs::TicketedSpaceApi*>(m_eventTicketingApi)->spacesTicketedGet({ requestSpaceId }, responseHandler);
}

} // namespace csp::systems
