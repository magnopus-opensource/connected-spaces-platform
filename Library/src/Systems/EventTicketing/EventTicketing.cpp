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
#include "CSP/Systems/EventTicketing/EventTicketing.h"

#include "Services/AggregationService/Api.h"

namespace chs = csp::services::generated::aggregationservice;

csp::systems::EventTicketingVendor VendorNameToEnum(const csp::common::String& vendorName)
{
    csp::systems::EventTicketingVendor vendor = csp::systems::EventTicketingVendor::Unknown;
    if (vendorName == "eventbrite")
    {
        vendor = csp::systems::EventTicketingVendor::Eventbrite;
    }
    else
    {
        CSP_LOG_MSG(csp::common::LogLevel::Warning,
            "Encountered an unknown ticketing vendor string when parsing a response from services. Defaulting to 'Unknown'");
    }

    return vendor;
}

csp::systems::TicketStatus TicketStatusToEnum(const chs::TicketStatus& dtoStatus)
{
    csp::systems::TicketStatus status = csp::systems::TicketStatus::Unknown;

    if (dtoStatus.GetValue() == chs::TicketStatus::eTicketStatus::PURCHASED)
    {
        status = csp::systems::TicketStatus::Purchased;
    }
    if (dtoStatus.GetValue() == chs::TicketStatus::eTicketStatus::REDEEMED)
    {
        status = csp::systems::TicketStatus::Redeemed;
    }
    else
    {
        CSP_LOG_MSG(
            csp::common::LogLevel::Error, "Encountered an unknown ticket status when parsing a response from services. Defaulting to 'Unknown'");
    }

    return status;
}

void SpaceEventDtoToTicketedEvent(const chs::SpaceEventDto& dto, csp::systems::TicketedEvent& event)
{
    event.Id = dto.GetId();
    event.SpaceId = dto.GetSpaceId();

    if (dto.HasVendorName())
    {
        event.Vendor = VendorNameToEnum(dto.GetVendorName());
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceEventDto missing VendorName");
        event.Vendor = csp::systems::EventTicketingVendor::Unknown;
    }

    if (dto.HasVendorEventId())
    {
        event.VendorEventId = dto.GetVendorEventId();
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceEventDto missing VendorEventId");
        event.VendorEventId = "";
    }

    if (dto.HasVendorEventUri())
    {
        event.VendorEventUri = dto.GetVendorEventUri();
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceEventDto missing VendorEventUri");
        event.VendorEventUri = "";
    }

    if (dto.HasIsTicketingActive())
    {
        event.IsTicketingActive = dto.GetIsTicketingActive();
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceEventDto missing IsTicketingActive");
        event.IsTicketingActive = false;
    }
}

void SpaceTicketDtoToEventTicket(const chs::SpaceTicketDto& dto, csp::systems::EventTicket& ticket)
{
    ticket.Id = dto.GetId();
    ticket.SpaceId = dto.GetSpaceId();

    if (dto.HasVendorName())
    {
        ticket.Vendor = VendorNameToEnum(dto.GetVendorName());
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceTicketDto missing VendorName");
        ticket.Vendor = csp::systems::EventTicketingVendor::Unknown;
    }

    if (dto.HasVendorEventId())
    {
        ticket.VendorEventId = dto.GetVendorEventId();
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceTicketDto missing VendorEventId");
        ticket.VendorEventId = "";
    }

    if (dto.HasVendorTicketId())
    {
        ticket.VendorTicketId = dto.GetVendorTicketId();
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceTicketDto missing VendorTicketId");
        ticket.VendorTicketId = "";
    }

    if (dto.HasTicketStatus())
    {
        ticket.Status = TicketStatusToEnum(*dto.GetTicketStatus());
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceTicketDto missing UserId");
        ticket.UserId = "";
    }

    if (dto.HasUserId())
    {
        ticket.UserId = dto.GetUserId();
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceTicketDto missing UserId");
        ticket.UserId = "";
    }

    if (dto.HasEmailLower())
    {
        ticket.Email = dto.GetEmailLower();
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceTicketDto missing EmailLower");
        ticket.Email = "";
    }
}

void VendorInfoDtoToVendorInfo(const chs::VendorProviderInfo& dto, csp::systems::TicketedEventVendorAuthInfo& vendorInfo)
{
    if (dto.HasVendorName())
    {
        vendorInfo.Vendor = VendorNameToEnum(dto.GetVendorName());
    }

    if (dto.HasClientId())
    {
        vendorInfo.ClientId = dto.GetClientId();
    }

    if (dto.HasAuthorizeEndpoint())
    {
        vendorInfo.AuthorizeEndpoint = dto.GetAuthorizeEndpoint();
    }

    if (dto.HasOAuthRedirectUrl())
    {
        vendorInfo.OAuthRedirectUrl = dto.GetOAuthRedirectUrl();
    }
}

namespace csp::systems
{

bool TicketedEvent::operator==(const TicketedEvent& other) const
{
    return Id == other.Id && SpaceId == other.SpaceId && Vendor == other.Vendor && VendorEventId == other.VendorEventId
        && VendorEventUri == other.VendorEventUri && IsTicketingActive == other.IsTicketingActive;
}

bool TicketedEvent::operator!=(const TicketedEvent& other) const { return !(*this == other); }

void TicketedEventResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* dto = static_cast<chs::SpaceEventDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        dto->FromJson(response->GetPayload().GetContent());

        SpaceEventDtoToTicketedEvent(*dto, m_event);
    }
}

TicketedEvent& TicketedEventResult::GetTicketedEvent() { return m_event; }

const TicketedEvent& TicketedEventResult::GetTicketedEvent() const { return m_event; }

void TicketedEventCollectionResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* ticketedEventCollectionResponse = static_cast<csp::services::DtoArray<chs::SpaceEventDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        ticketedEventCollectionResponse->FromJson(response->GetPayload().GetContent());

        const std::vector<chs::SpaceEventDto>& dtoArray = ticketedEventCollectionResponse->GetArray();
        m_events = csp::common::Array<csp::systems::TicketedEvent>(dtoArray.size());

        for (size_t idx = 0; idx < dtoArray.size(); ++idx)
        {
            SpaceEventDtoToTicketedEvent(dtoArray[idx], m_events[idx]);
        }
    }
}

csp::common::Array<TicketedEvent>& TicketedEventCollectionResult::GetTicketedEvents() { return m_events; }

const csp::common::Array<TicketedEvent>& TicketedEventCollectionResult::GetTicketedEvents() const { return m_events; }

void EventTicketResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* dto = static_cast<chs::SpaceTicketDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        dto->FromJson(response->GetPayload().GetContent());

        SpaceTicketDtoToEventTicket(*dto, m_ticket);
    }
}

bool EventTicket::operator==(const EventTicket& other) const
{
    return Id == other.Id && SpaceId == other.SpaceId && Vendor == other.Vendor && VendorEventId == other.VendorEventId
        && VendorTicketId == other.VendorTicketId && Status == other.Status && UserId == other.UserId && Email == other.Email;
}

bool EventTicket::operator!=(const EventTicket& other) const { return !(*this == other); }

EventTicket& EventTicketResult::GetEventTicket() { return m_ticket; }

const EventTicket& EventTicketResult::GetEventTicket() const { return m_ticket; }

bool TicketedEventVendorAuthInfo::operator==(const TicketedEventVendorAuthInfo& other) const
{
    return Vendor == other.Vendor && ClientId == other.ClientId && AuthorizeEndpoint == other.AuthorizeEndpoint
        && OAuthRedirectUrl == other.OAuthRedirectUrl;
}

bool TicketedEventVendorAuthInfo::operator!=(const TicketedEventVendorAuthInfo& other) const { return !(*this == other); }

void TicketedEventVendorAuthInfoResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* dto = static_cast<chs::VendorProviderInfo*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        dto->FromJson(response->GetPayload().GetContent());

        VendorInfoDtoToVendorInfo(*dto, m_vendorInfo);
    }
}

bool SpaceIsTicketedResult::GetIsTicketedEvent() { return m_spaceIsTicketed; }

bool SpaceIsTicketedResult::GetIsTicketedEvent() const { return m_spaceIsTicketed; }

void SpaceIsTicketedResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        auto inputText = response->GetPayload().GetContent();

        rapidjson::Document responseJson;
        rapidjson::ParseResult ok = csp::json::ParseWithErrorLogging(responseJson, inputText, "SpaceIsTicketedResult::OnResponse");
        if (!ok)
        {
            return;
        }

        // We expect the response to be a JSON object with a set of fields describing key/value pairs of space IDs and bools,
        // where the bool describes if it is ticketed or not
        // We currently however only care about whether the _first_ space is ticketed, because our API only allows clients to
        // reasons about whether a single space is ticketed.

        bool expectedResponse = true;
        const auto jsonRootObject = responseJson.GetObject();
        const rapidjson::Value::ConstMemberIterator firstJsonMember = jsonRootObject.MemberBegin();
        if (firstJsonMember != jsonRootObject.MemberEnd())
        {
            if (firstJsonMember->value.IsBool())
            {
                m_spaceIsTicketed = firstJsonMember->value.GetBool();
                CSP_LOG_FORMAT(common::LogLevel::VeryVerbose, "We found that the space with ID %s has ticketed status: %s",
                    firstJsonMember->name.GetString(), m_spaceIsTicketed ? "true" : "false");
            }
            else
            {
                expectedResponse = false;
            }
        }
        else
        {
            expectedResponse = false;
        }

        if (expectedResponse == false)
        {
            CSP_LOG_MSG(common::LogLevel::Error,
                "CSP received a response from services in an unexpected format when querying if a space requires a ticket to enter");
        }
    }
}

} // namespace csp::systems
