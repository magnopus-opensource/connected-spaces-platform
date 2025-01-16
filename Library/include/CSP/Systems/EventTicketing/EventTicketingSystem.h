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

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

namespace csp::systems
{

/// @ingroup Event Ticketing System
/// @brief System that allows creation and management of ticketed events for spaces.
class CSP_API EventTicketingSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    friend void csp::memory::Delete<EventTicketingSystem>(EventTicketingSystem* Ptr);
    /** @endcond */
    CSP_END_IGNORE

public:
    /// @brief Creates a ticketed event for the given space.
    /// @param SpaceId csp::common::String : ID of the space to create the event for.
    /// @param Vendor csp::systems::EventTicketingVendor : Enum representing the vendor that the event is created with.
    /// @param VendorEventId csp::common::String : Specifies the event ID in the vendors system.
    /// @param VendorEventUri csp::common::String : Specifies the URI for the event in the vendors system.
    /// @param IsTicketingActive bool : Specifies whether ticketing is currently active for this event.
    /// @param Callback TicketedEventResultCallback : Callback providing the TicketedEvent once created.
    CSP_ASYNC_RESULT void CreateTicketedEvent(const csp::common::String& SpaceId, EventTicketingVendor Vendor,
        const csp::common::String& VendorEventId, const csp::common::String& VendorEventUri, bool IsTicketingActive,
        TicketedEventResultCallback Callback);

    /// @brief Updates a ticketed event in the given space.
    ///
    /// All parameters should be provided even if they are not new values. Empty values will overwrite
    /// existing values to be empty.
    ///
    /// @param SpaceId csp::common::String : ID of the space the event belongs to.
    /// @param EventId csp::common::String : ID of the Event to update.
    /// @param Vendor csp::systems::EventTicketingVendor : Optional enum representing the vendor that the event should be updated with.
    /// @param VendorEventId csp::common::String : Optional value to update the event ID in the vendors system with.
    /// @param VendorEventUri csp::common::String : Optional value to update the URI for the event in the vendors system with.
    /// @param IsTicketingActive bool : Optional value to update whether ticketing is currently active for this event.
    /// @param Callback TicketedEventResultCallback : Callback providing the TicketedEvent once created.
    CSP_ASYNC_RESULT void UpdateTicketedEvent(const csp::common::String& SpaceId, const csp::common::String& EventId,
        const csp::common::Optional<EventTicketingVendor>& Vendor, const csp::common::Optional<csp::common::String>& VendorEventId,
        const csp::common::Optional<csp::common::String>& VendorEventUri, const csp::common::Optional<bool>& IsTicketingActive,
        TicketedEventResultCallback Callback);

    /// @brief Creates a ticketed event for the given space.
    /// @param SpaceIds csp::common::Array<csp::common::String> : IDs of the spaces to get the events for.
    /// @param Skip csp::common::Optional<int> : Optional number of results that will be skipped from the result.
    /// @param Limit csp::common::Optional<int> : Optional maximum number of results to be retrieved.
    /// @param Callback TicketedEventCollectionResultCallback : Callback providing the TicketedEvents for the space.
    CSP_ASYNC_RESULT void GetTicketedEvents(const csp::common::Array<csp::common::String>& SpaceIds, const csp::common::Optional<int>& Skip,
        const csp::common::Optional<int>& Limit, TicketedEventCollectionResultCallback Callback);

    /// @brief Submits a ticket from a vendor for the given event.
    ///
    /// @param SpaceId csp::common::String : ID of the space the event belongs to.
    /// @param Vendor csp::systems::EventTicketingVendor : Enum representing the vendor that the event is created with.
    /// @param VendorEventId csp::common::String : Specifies the event ID in the vendors system.
    /// @param VendorTicketId csp::common::String : Specifies the ticket ID in the vendors system.
    /// @param OnBehalfOfUserId csp::common::Optional<csp::common::String> : Optionally submit the ticket for another user. Requires super user
    /// permissions.
    /// @param Callback EventTicketResultCallback : Callback providing the TicketedEvent once created.
    CSP_ASYNC_RESULT void SubmitEventTicket(const csp::common::String& SpaceId, EventTicketingVendor Vendor, const csp::common::String& VendorEventId,
        const csp::common::String& VendorTicketId, const csp::common::Optional<csp::common::String>& OnBehalfOfUserId,
        EventTicketResultCallback Callback);

    /// @brief  Looks up the basic info required by a client to initiate an oauth2 flow with the specified vendor.
    /// @param Vendor EventTicketingVendor : The vendor type to retrieve info for.
    /// @param UserId csp::common::String : The ID of the user to obtain authentication info for.
    /// @param Callback TicketedEventVendorInfoResultCallback : Callback providing the oauth2 information.
    CSP_ASYNC_RESULT void GetVendorAuthorizeInfo(
        EventTicketingVendor Vendor, const csp::common::String& UserId, TicketedEventVendorAuthorizeInfoCallback Callback);

    /// @brief Gets the ticketed status of a space given by ID.
    /// @param SpaceId const csp::common::String& : The space ID to check the status for.
    /// @param Callback SpaceIsTicketedResultCallback : Callback providing the result of the query.
    CSP_ASYNC_RESULT void GetIsSpaceTicketed(const csp::common::String& SpaceId, SpaceIsTicketedResultCallback Callback);

private:
    EventTicketingSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT EventTicketingSystem(csp::web::WebClient* InWebClient);
    ~EventTicketingSystem();

    csp::services::ApiBase* EventTicketingAPI;
};

} // namespace csp::systems
