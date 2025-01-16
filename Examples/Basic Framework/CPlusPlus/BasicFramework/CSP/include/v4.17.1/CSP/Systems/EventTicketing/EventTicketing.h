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
#include "CSP/Systems/WebService.h"

namespace csp::systems
{

/// @ingroup Event Ticketing System
/// @brief Enum representing the third party vendor used for ticketing.
enum class EventTicketingVendor
{
    Eventbrite = 0,
    Unknown
};

/// @ingroup Event Ticketing System
/// @brief Enum representing the status of a ticket where purchased means a ticket has not yet been redeemd
///        and redeemed means the ticket has been submitted already.
enum class TicketStatus
{
    Purchased = 0,
    Redeemed,
    Unknown
};

/// @ingroup Event Ticketing System
/// @brief Data representation of a ticketed event
class CSP_API TicketedEvent
{
public:
    TicketedEvent()
        : Vendor(EventTicketingVendor::Unknown)
        , IsTicketingActive(false) {};

    /// @brief CHS ID of the event resource.
    csp::common::String Id;
    /// @brief  ID of the space the event belongs to.
    csp::common::String SpaceId;
    /// @brief 3rd party vendor mangaging the event.
    EventTicketingVendor Vendor;
    /// @brief ID within the 3rd party vendor of the event.
    csp::common::String VendorEventId;
    /// @brief URI to load the event in the 3rd party.
    csp::common::String VendorEventUri;
    /// @brief Specifies whether ticketing is currently turned on for the space.
    bool IsTicketingActive;
};

/// @ingroup Event Ticketing System
/// @brief Data representation of a ticket for an event
class CSP_API EventTicket
{
public:
    EventTicket()
        : Vendor(EventTicketingVendor::Unknown)
        , Status(TicketStatus::Unknown) {};

    /// @brief CHS ID of the ticket resource.
    csp::common::String Id;
    /// @brief  ID of the space the ticket belongs to.
    csp::common::String SpaceId;
    /// @brief Third party vendor mangaging the ticket.
    EventTicketingVendor Vendor;
    /// @brief ID within the third party vendor of the event the ticket is for.
    csp::common::String VendorEventId;
    /// @brief ID within the third party vendor of the ticket.
    csp::common::String VendorTicketId;
    /// @brief Current status of the ticket.
    csp::systems::TicketStatus Status;
    /// @brief ID of the user associated with this ticket.
    csp::common::String UserId;
    /// @brief Email address of the user associated with this ticket. The email associated to the 3rd party ticket should match the email of the CHS
    /// user.
    csp::common::String Email;
};

/// @ingroup Event Ticketing System
/// @brief Data representation of a third party vendor for ticketed events
class CSP_API TicketedEventVendorAuthInfo
{
public:
    TicketedEventVendorAuthInfo()
        : Vendor(EventTicketingVendor::Unknown)
    {
    }

    /// @brief Third party vendor to get auth info for.
    EventTicketingVendor Vendor;
    /// @brief Application client ID with the third party vendor.
    csp::common::String ClientId;
    /// @brief URI of third party vendor authorize endpoint.
    csp::common::String AuthorizeEndpoint;
    /// @brief CHS URL the third party vendor can provide the OAuth code to.
    csp::common::String OAuthRedirectUrl;
};

/// @ingroup Event Ticketing System
/// @brief Result class holding a TicketedEvent.
class CSP_API TicketedEventResult : public csp::systems::ResultBase
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
class CSP_API TicketedEventCollectionResult : public csp::systems::ResultBase
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
/// @brief Result class holding a ticket for an event.
class CSP_API EventTicketResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Get the ticketed event from the result.
    /// @return The ticketed event.
    EventTicket& GetEventTicket();

    /// @brief Get the ticketed event from the result.
    /// @return The ticketed event.
    const EventTicket& GetEventTicket() const;

private:
    EventTicketResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    EventTicket Ticket;
};

/// @ingroup Event Ticketing System
/// @brief Result class holding a collection (array) of TicketedEvents.
class CSP_API SpaceIsTicketedResult : public csp::systems::ResultBase
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
    SpaceIsTicketedResult(void*)
        : SpaceIsTicketed(false) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    bool SpaceIsTicketed;
};

/// @ingroup Event Ticketing System
/// @brief Result class providing the oauth2 information required to start authenticating with a ticketed event vendor.
class CSP_API TicketedEventVendorAuthInfoResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    TicketedEventVendorAuthInfo GetVendorAuthInfo() const { return VendorInfo; }

private:
    TicketedEventVendorAuthInfoResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    TicketedEventVendorAuthInfo VendorInfo;
};

// @brief Callback providing a ticketed event result.
typedef std::function<void(const TicketedEventResult& Result)> TicketedEventResultCallback;

// @brief Callback providing a ticketed event collection result.
typedef std::function<void(const TicketedEventCollectionResult& Result)> TicketedEventCollectionResultCallback;

// @brief Callback providing a ticketed event result.
typedef std::function<void(const EventTicketResult& Result)> EventTicketResultCallback;

// @brief Callback providing a ticket event status for a space, from an endpoint result.
typedef std::function<void(const SpaceIsTicketedResult& Result)> SpaceIsTicketedResultCallback;

// @brief Callback providing the ticketed event vendor information necessary for authenticating with the vendor's platform.
typedef std::function<void(const TicketedEventVendorAuthInfoResult& Result)> TicketedEventVendorAuthorizeInfoCallback;

} // namespace csp::systems
