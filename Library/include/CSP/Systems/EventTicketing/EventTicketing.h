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

#include "CSP/Systems/SystemsResult.h"

namespace csp::systems
{

enum class EventTicketingVendor
{
	Eventbrite = 0,
	Unknown
};

/// @ingroup Event Ticketing System
/// @brief Data representation of a ticketed event
class CSP_API TicketedEvent
{
public:
	TicketedEvent() : Vendor(EventTicketingVendor::Unknown), IsTicketingActive(false) {};

	csp::common::String Id;
	csp::common::String SpaceId;
	EventTicketingVendor Vendor;
	csp::common::String VendorEventId;
	csp::common::String VendorEventUri;
	bool IsTicketingActive;
};


/// @ingroup Event Ticketing System
/// @brief Result class holding a TicketedEvent.
class CSP_API TicketedEventResult : public csp::services::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	CSP_END_IGNORE
	/** @endcond */

public:
	/// @brief Get the ticketed event from the result
	/// @return The ticketed event.
	TicketedEvent& GetTicketedEvent();

	/// @brief Get the ticketed event from the result
	/// @return The ticketed event.
	const TicketedEvent& GetTicketedEvent() const;

private:
	TicketedEventResult(void*) {};

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	TicketedEvent Event;
};


// @brief Callback providing a ticketed event result.
typedef std::function<void(const TicketedEventResult& Result)> TicketedEventResultCallback;

} // namespace csp::systems
