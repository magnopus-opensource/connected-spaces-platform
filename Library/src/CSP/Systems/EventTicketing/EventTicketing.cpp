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
#include "Services/AggregationService/Dto.h"

namespace chs = csp::services::generated::aggregationservice;

void SpaceEventDtoToTicketedEvent(const chs::SpaceEventDto& Dto, csp::systems::TicketedEvent& Event)
{
	Event.Id				= Dto.GetId();
	Event.SpaceId			= Dto.GetSpaceId();
	Event.Vendor			= csp::systems::EventTicketingVendor::Eventbrite;
	Event.VendorEventId		= Dto.GetVendorEventId();
	Event.VendorEventUri	= Dto.GetVendorEventUri();
	Event.IsTicketingActive = Dto.GetIsTicketingActive();
}

namespace csp::systems
{

void TicketedEventResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	auto* Dto							   = static_cast<chs::SpaceEventDto*>(ApiResponse->GetDto());
	const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		Dto->FromJson(Response->GetPayload().GetContent());

		SpaceEventDtoToTicketedEvent(*Dto, Event);
	}
}

TicketedEvent& TicketedEventResult::GetTicketedEvent()
{
	return Event;
}

const TicketedEvent& TicketedEventResult::GetTicketedEvent() const
{
	return Event;
}

void TicketedEventCollectionResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	auto* TicketedEventCollectionResponse  = static_cast<csp::services::DtoArray<chs::SpaceEventDto>*>(ApiResponse->GetDto());
	const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		TicketedEventCollectionResponse->FromJson(Response->GetPayload().GetContent());

		const std::vector<chs::SpaceEventDto>& DtoArray = TicketedEventCollectionResponse->GetArray();
		Events											= csp::common::Array<csp::systems::TicketedEvent>(DtoArray.size());

		for (size_t idx = 0; idx < DtoArray.size(); ++idx)
		{
			SpaceEventDtoToTicketedEvent(DtoArray[idx], Events[idx]);
		}
	}
}

csp::common::Array<TicketedEvent>& TicketedEventCollectionResult::GetTicketedEvents()
{
	return Events;
}

const csp::common::Array<TicketedEvent>& TicketedEventCollectionResult::GetTicketedEvents() const
{
	return Events;
}

} // namespace csp::systems
