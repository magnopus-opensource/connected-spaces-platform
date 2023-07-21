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
#pragma once

#include "CSP/Common/Optional.h"
#include "CSP/Systems/EventTicketing/EventTicketing.h"
#include "CSP/Systems/SystemBase.h"

namespace csp::systems
{

/// @ingroup Event Ticketing System
/// @brief System that allows creation and management of ticketed events for spaces.
class CSP_API CSP_NO_DISPOSE EventTicketingSystem : public SystemBase
{
	/** @cond DO_NOT_DOCUMENT */
	friend class SystemsManager;
	/** @endcond */

public:
	~EventTicketingSystem();

	/// @brief Creates a ticketed event for the given space.
	/// @param SpaceId csp::common::String : ID of the space to create the event for.
	/// @param Vendor csp::systems::EventTicketingVendor : Enum representing the vendor that the event is created with.
	/// @param VendorEventId csp::common::String : Specifies the event ID in the vendors system.
	/// @param VendorEventUri csp::common::String : Specifies the URI for the event in the vendors system.
	/// @param IsTicketingActive bool : Specifies whether ticketing is currently active for this event.
	/// @param Callback TicketedEventResultCallback : Callback providing the TicketedEvent once created.
	CSP_ASYNC_RESULT void CreateTicketedEvent(const csp::common::String& SpaceId,
											  EventTicketingVendor Vendor,
											  const csp::common::String& VendorEventId,
											  const csp::common::String& VendorEventUri,
											  bool IsTicketingActive,
											  TicketedEventResultCallback Callback);

	/// @brief Creates a ticketed event for the given space.
	/// @param SpaceId csp::common::String : ID of the space to create the event for.
	/// @param Callback TicketedEventCollectionResultCallback : Callback providing the TicketedEvents for the space.
	CSP_ASYNC_RESULT void GetTicketedEvents(const csp::common::String& SpaceId,
											const csp::common::Optional<int>& Skip,
											const csp::common::Optional<int>& Limit,
											TicketedEventCollectionResultCallback Callback);

private:
	EventTicketingSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
	CSP_NO_EXPORT EventTicketingSystem(csp::web::WebClient* InWebClient);

	csp::services::ApiBase* EventTicketingAPI;
};

} // namespace csp::systems
