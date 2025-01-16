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

#include "CSP/CSPCommon.h"
#include "Events/Event.h"
#include "Events/EventListener.h"

namespace csp::events
{

class CSP_API EventSystem
{
public:
    EventSystem();
    ~EventSystem();

    static EventSystem& Get();

    /// @brief Create a new event instance
    /// @note The event will be deleted after it has been processed in ProcessEvents
    Event* AllocateEvent(const EventId& Id);

    /// @brief Enqueue an event to be sent later
    /// @note This call is thread safe
    /// @param InEvent
    void EnqueueEvent(const Event* InEvent);

    void RegisterListener(const EventId& Id, EventListener* InListener);
    void UnRegisterListener(const EventId& Id, EventListener* InListener);

    void UnRegisterAllListeners();

    /// @brief Process all queued events and send them to any listeners
    void ProcessEvents();

private:
    // Internal implementation
    class EventSystemImpl* Impl;
};

} // namespace csp::events
