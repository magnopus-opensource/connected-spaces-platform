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

csp::systems::EventTicketingVendor VendorNameToEnum(const csp::common::String& VendorName)
{
    csp::systems::EventTicketingVendor Vendor = csp::systems::EventTicketingVendor::Unknown;
    if (VendorName == "eventbrite")
    {
        Vendor = csp::systems::EventTicketingVendor::Eventbrite;
    }
    else
    {
        CSP_LOG_MSG(csp::common::LogLevel::Warning,
            "Encountered an unknown ticketing vendor string when parsing a response from services. Defaulting to 'Unknown'");
    }

    return Vendor;
}

csp::systems::TicketStatus TicketStatusToEnum(const chs::TicketStatus& DtoStatus)
{
    csp::systems::TicketStatus Status = csp::systems::TicketStatus::Unknown;

    if (DtoStatus.GetValue() == chs::TicketStatus::eTicketStatus::PURCHASED)
    {
        Status = csp::systems::TicketStatus::Purchased;
    }
    if (DtoStatus.GetValue() == chs::TicketStatus::eTicketStatus::REDEEMED)
    {
        Status = csp::systems::TicketStatus::Redeemed;
    }
    else
    {
        CSP_LOG_MSG(
            csp::common::LogLevel::Error, "Encountered an unknown ticket status when parsing a response from services. Defaulting to 'Unknown'");
    }

    return Status;
}

void SpaceEventDtoToTicketedEvent(const chs::SpaceEventDto& Dto, csp::systems::TicketedEvent& Event)
{
    Event.Id = Dto.GetId();
    Event.SpaceId = Dto.GetSpaceId();

    if (Dto.HasVendorName())
    {
        Event.Vendor = VendorNameToEnum(Dto.GetVendorName());
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceEventDto missing VendorName");
        Event.Vendor = csp::systems::EventTicketingVendor::Unknown;
    }

    if (Dto.HasVendorEventId())
    {
        Event.VendorEventId = Dto.GetVendorEventId();
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceEventDto missing VendorEventId");
        Event.VendorEventId = "";
    }

    if (Dto.HasVendorEventUri())
    {
        Event.VendorEventUri = Dto.GetVendorEventUri();
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceEventDto missing VendorEventUri");
        Event.VendorEventUri = "";
    }

    if (Dto.HasIsTicketingActive())
    {
        Event.IsTicketingActive = Dto.GetIsTicketingActive();
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceEventDto missing IsTicketingActive");
        Event.IsTicketingActive = false;
    }
}

void SpaceTicketDtoToEventTicket(const chs::SpaceTicketDto& Dto, csp::systems::EventTicket& Ticket)
{
    Ticket.Id = Dto.GetId();
    Ticket.SpaceId = Dto.GetSpaceId();

    if (Dto.HasVendorName())
    {
        Ticket.Vendor = VendorNameToEnum(Dto.GetVendorName());
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceTicketDto missing VendorName");
        Ticket.Vendor = csp::systems::EventTicketingVendor::Unknown;
    }

    if (Dto.HasVendorEventId())
    {
        Ticket.VendorEventId = Dto.GetVendorEventId();
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceTicketDto missing VendorEventId");
        Ticket.VendorEventId = "";
    }

    if (Dto.HasVendorTicketId())
    {
        Ticket.VendorTicketId = Dto.GetVendorTicketId();
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceTicketDto missing VendorTicketId");
        Ticket.VendorTicketId = "";
    }

    if (Dto.HasTicketStatus())
    {
        Ticket.Status = TicketStatusToEnum(*Dto.GetTicketStatus());
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceTicketDto missing UserId");
        Ticket.UserId = "";
    }

    if (Dto.HasUserId())
    {
        Ticket.UserId = Dto.GetUserId();
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceTicketDto missing UserId");
        Ticket.UserId = "";
    }

    if (Dto.HasEmailLower())
    {
        Ticket.Email = Dto.GetEmailLower();
    }
    else
    {
        CSP_LOG_WARN_MSG("SpaceTicketDto missing EmailLower");
        Ticket.Email = "";
    }
}

void VendorInfoDtoToVendorInfo(const chs::VendorProviderInfo& Dto, csp::systems::TicketedEventVendorAuthInfo& VendorInfo)
{
    if (Dto.HasVendorName())
    {
        VendorInfo.Vendor = VendorNameToEnum(Dto.GetVendorName());
    }

    if (Dto.HasClientId())
    {
        VendorInfo.ClientId = Dto.GetClientId();
    }

    if (Dto.HasAuthorizeEndpoint())
    {
        VendorInfo.AuthorizeEndpoint = Dto.GetAuthorizeEndpoint();
    }

    if (Dto.HasOAuthRedirectUrl())
    {
        VendorInfo.OAuthRedirectUrl = Dto.GetOAuthRedirectUrl();
    }
}

namespace csp::systems
{

bool TicketedEvent::operator==(const TicketedEvent& Other) const
{
    return Id == Other.Id && SpaceId == Other.SpaceId && Vendor == Other.Vendor && VendorEventId == Other.VendorEventId
        && VendorEventUri == Other.VendorEventUri && IsTicketingActive == Other.IsTicketingActive;
}

bool TicketedEvent::operator!=(const TicketedEvent& Other) const { return !(*this == Other); }

void TicketedEventResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* Dto = static_cast<chs::SpaceEventDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        Dto->FromJson(Response->GetPayload().GetContent());

        SpaceEventDtoToTicketedEvent(*Dto, Event);
    }
}

TicketedEvent& TicketedEventResult::GetTicketedEvent() { return Event; }

const TicketedEvent& TicketedEventResult::GetTicketedEvent() const { return Event; }

void TicketedEventCollectionResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* TicketedEventCollectionResponse = static_cast<csp::services::DtoArray<chs::SpaceEventDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        TicketedEventCollectionResponse->FromJson(Response->GetPayload().GetContent());

        const std::vector<chs::SpaceEventDto>& DtoArray = TicketedEventCollectionResponse->GetArray();
        Events = csp::common::Array<csp::systems::TicketedEvent>(DtoArray.size());

        for (size_t idx = 0; idx < DtoArray.size(); ++idx)
        {
            SpaceEventDtoToTicketedEvent(DtoArray[idx], Events[idx]);
        }
    }
}

csp::common::Array<TicketedEvent>& TicketedEventCollectionResult::GetTicketedEvents() { return Events; }

const csp::common::Array<TicketedEvent>& TicketedEventCollectionResult::GetTicketedEvents() const { return Events; }

void EventTicketResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* Dto = static_cast<chs::SpaceTicketDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        Dto->FromJson(Response->GetPayload().GetContent());

        SpaceTicketDtoToEventTicket(*Dto, Ticket);
    }
}

bool EventTicket::operator==(const EventTicket& Other) const
{
    return Id == Other.Id && SpaceId == Other.SpaceId && Vendor == Other.Vendor && VendorEventId == Other.VendorEventId
        && VendorTicketId == Other.VendorTicketId && Status == Other.Status && UserId == Other.UserId && Email == Other.Email;
}

bool EventTicket::operator!=(const EventTicket& Other) const { return !(*this == Other); }

EventTicket& EventTicketResult::GetEventTicket() { return Ticket; }

const EventTicket& EventTicketResult::GetEventTicket() const { return Ticket; }

bool TicketedEventVendorAuthInfo::operator==(const TicketedEventVendorAuthInfo& Other) const
{
    return Vendor == Other.Vendor && ClientId == Other.ClientId && AuthorizeEndpoint == Other.AuthorizeEndpoint
        && OAuthRedirectUrl == Other.OAuthRedirectUrl;
}

bool TicketedEventVendorAuthInfo::operator!=(const TicketedEventVendorAuthInfo& Other) const { return !(*this == Other); }

void TicketedEventVendorAuthInfoResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* Dto = static_cast<chs::VendorProviderInfo*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        Dto->FromJson(Response->GetPayload().GetContent());

        VendorInfoDtoToVendorInfo(*Dto, VendorInfo);
    }
}

bool SpaceIsTicketedResult::GetIsTicketedEvent() { return SpaceIsTicketed; }

bool SpaceIsTicketedResult::GetIsTicketedEvent() const { return SpaceIsTicketed; }

void SpaceIsTicketedResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        auto InputText = Response->GetPayload().GetContent();

        rapidjson::Document ResponseJson;
        rapidjson::ParseResult ok = csp::json::ParseWithErrorLogging(ResponseJson, InputText, "SpaceIsTicketedResult::OnResponse");
        if (!ok)
        {
            return;
        }

        // We expect the response to be a JSON object with a set of fields describing key/value pairs of space IDs and bools,
        // where the bool describes if it is ticketed or not
        // We currently however only care about whether the _first_ space is ticketed, because our API only allows clients to
        // reasons about whether a single space is ticketed.

        bool ExpectedResponse = true;
        const auto JsonRootObject = ResponseJson.GetObject();
        const rapidjson::Value::ConstMemberIterator FirstJSONMember = JsonRootObject.MemberBegin();
        if (FirstJSONMember != JsonRootObject.MemberEnd())
        {
            if (FirstJSONMember->value.IsBool())
            {
                SpaceIsTicketed = FirstJSONMember->value.GetBool();
                CSP_LOG_FORMAT(common::LogLevel::VeryVerbose, "We found that the space with ID %s has ticketed status: %s",
                    FirstJSONMember->name.GetString(), SpaceIsTicketed ? "true" : "false");
            }
            else
            {
                ExpectedResponse = false;
            }
        }
        else
        {
            ExpectedResponse = false;
        }

        if (ExpectedResponse == false)
        {
            CSP_LOG_MSG(common::LogLevel::Error,
                "CSP received a response from services in an unexpected format when querying if a space requires a ticket to enter");
        }
    }
}

} // namespace csp::systems
