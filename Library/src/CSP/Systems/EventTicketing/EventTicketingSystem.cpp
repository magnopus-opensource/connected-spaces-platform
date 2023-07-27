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

csp::common::String GetVendorNameString(const csp::systems::EventTicketingVendor& Vendor)
{
	switch (Vendor)
	{
		case csp::systems::EventTicketingVendor::Eventbrite:
			return "eventbrite";
		default:
			FOUNDATION_LOG_WARN_MSG("Unknown ticketed event vendor");
			return "Unknown";
	}
}

namespace csp::systems
{


EventTicketingSystem::EventTicketingSystem(csp::web::WebClient* InWebClient) : SystemBase(InWebClient)
{
	EventTicketingAPI = CSP_NEW chs::TicketedSpaceApi(InWebClient);
}

EventTicketingSystem::~EventTicketingSystem()
{
	CSP_DELETE(EventTicketingAPI);
}

void EventTicketingSystem::CreateTicketedEvent(const csp::common::String& SpaceId,
											   EventTicketingVendor Vendor,
											   const csp::common::String& VendorEventId,
											   const csp::common::String& VendorEventUri,
											   bool IsTicketingActive,
											   TicketedEventResultCallback Callback)
{
	auto Request = std::make_shared<chs::SpaceEventDto>();

	Request->SetSpaceId(SpaceId);
	Request->SetVendorName(GetVendorNameString(Vendor));
	Request->SetVendorEventId(VendorEventId);
	Request->SetVendorEventUri(VendorEventUri);
	Request->SetIsTicketingActive(IsTicketingActive);

	csp::services::ResponseHandlerPtr ResponseHandler
		= EventTicketingAPI->CreateHandler<TicketedEventResultCallback, TicketedEventResult, void, chs::SpaceEventDto>(
			Callback,
			nullptr,
			csp::web::EResponseCodes::ResponseCreated);

	static_cast<chs::TicketedSpaceApi*>(EventTicketingAPI)->apiV1SpacesSpaceIdEventsPost(SpaceId, Request, ResponseHandler);
}

void EventTicketingSystem::GetTicketedEvents(const csp::common::Array<csp::common::String>& SpaceIds,
											 const csp::common::Optional<int>& Skip,
											 const csp::common::Optional<int>& Limit,
											 TicketedEventCollectionResultCallback Callback)
{
	std::vector<csp::common::String> RequestSpaceIds;
	RequestSpaceIds.reserve(SpaceIds.Size());

	for (auto i = 0; i < SpaceIds.Size(); ++i)
	{
		RequestSpaceIds.push_back(SpaceIds[i]);
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= EventTicketingAPI->CreateHandler<TicketedEventCollectionResultCallback,
										   TicketedEventCollectionResult,
										   void,
										   csp::services::DtoArray<chs::SpaceEventDto>>(Callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

	const auto RequestSkip	= Skip.HasValue() ? *Skip : std::optional<int>(std::nullopt);
	const auto RequestLimit = Limit.HasValue() ? *Limit : std::optional<int>(std::nullopt);

	static_cast<chs::TicketedSpaceApi*>(EventTicketingAPI)
		->apiV1SpacesEventsGet(std::nullopt, std::nullopt, RequestSpaceIds, RequestSkip, RequestLimit, ResponseHandler);
}

void EventTicketingSystem::GetVendorAuthoriseInfo(EventTicketingVendor Vendor,
												  const csp::common::String& UserId,
												  TicketedEventVendorAuthoriseInfoCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= EventTicketingAPI
			  ->CreateHandler<TicketedEventVendorAuthoriseInfoCallback, TicketedEventVendorAuthInfoResult, void, chs::VendorProviderInfo>(
				  Callback,
				  nullptr,
				  csp::web::EResponseCodes::ResponseCreated);

	static_cast<chs::TicketedSpaceApi*>(EventTicketingAPI)
		->apiV1VendorsVendorNameUsersUserIdProviderInfoGet(GetVendorNameString(Vendor), UserId, std::nullopt, ResponseHandler);
}

} // namespace csp::systems
