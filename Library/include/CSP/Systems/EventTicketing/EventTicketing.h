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
/// @brief Data representation of a third party vendor for ticketed events
class CSP_API TicketedEventVendorAuthInfo
{
public:
	TicketedEventVendorAuthInfo() : Vendor(EventTicketingVendor::Unknown)
	{
	}

	EventTicketingVendor Vendor;
	csp::common::String ClientId;
	csp::common::String AuthorizeEndpoint;
	csp::common::String OAuthRedirectUrl;
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
	/// @brief Get the ticketed event from the result.
	/// @return The ticketed event.
	TicketedEvent& GetTicketedEvent();

	/// @brief Get the ticketed event from the result.
	/// @return The ticketed event.
	const TicketedEvent& GetTicketedEvent() const;

private:
	TicketedEventResult(void*) {};

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	TicketedEvent Event;
};

/// @ingroup Event Ticketing System
/// @brief Result class holding a collection (array) of TicketedEvents.
class CSP_API TicketedEventCollectionResult : public csp::services::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	CSP_END_IGNORE
	/** @endcond */

public:
	/// @brief Gets the array of ticketed events from the result.
	/// @return The array of ticketed events.
	csp::common::Array<TicketedEvent>& GetTicketedEvents();

	/// @brief Gets the array of ticketed events from the result.
	/// @return The array of ticketed events.
	const csp::common::Array<TicketedEvent>& GetTicketedEvents() const;

private:
	TicketedEventCollectionResult(void*) {};

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	csp::common::Array<TicketedEvent> Events;
};

/// @ingroup Event Ticketing System
/// @brief Result class holding a collection (array) of TicketedEvents.
class CSP_API SpaceIsTicketedResult : public csp::services::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	CSP_END_IGNORE
	/** @endcond */

public:
	/// @brief Gets the ticketed status of the space from the result.
	/// @return A bool describing if the space is ticketed.
	bool GetIsTicketedEvent();

	/// @brief Gets the ticketed status of the space from the result.
	/// @return A bool describing if the space is ticketed.
	const bool GetIsTicketedEvent() const;

private:
	SpaceIsTicketedResult(void*) : SpaceIsTicketed(false) {};

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	bool SpaceIsTicketed;
};

/// @ingroup Event Ticketing System
/// @brief Result class providing the oauth2 information required to start authenticating with a ticketed event vendor.
class CSP_API TicketedEventVendorAuthInfoResult : public csp::services::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	CSP_END_IGNORE
	/** @endcond */

public:
	TicketedEventVendorAuthInfo GetVendorAuthInfo() const
	{
		return VendorInfo;
	}

private:
	TicketedEventVendorAuthInfoResult(void*) {};

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	TicketedEventVendorAuthInfo VendorInfo;
};

// @brief Callback providing a ticketed event result.
typedef std::function<void(const TicketedEventResult& Result)> TicketedEventResultCallback;

// @brief Callback providing a ticketed event collection result.
typedef std::function<void(const TicketedEventCollectionResult& Result)> TicketedEventCollectionResultCallback;

typedef std::function<void(const SpaceIsTicketedResult& Result)> SpaceIsTicketedResultCallback;


// @brief Callback providing the ticketed event vendor information necessary for authenticating with the vendor's platform.
typedef std::function<void(const TicketedEventVendorAuthInfoResult& Result)> TicketedEventVendorAuthorizeInfoCallback;

} // namespace csp::systems
