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

#include "AnalyticsProvider.h"
#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"

#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

namespace csp::systems
{

class AnalyticsSystemImpl;

/// @ingroup Analytics System
/// @brief Public facing system that allows interfacing with an analytics provider.
/// Offers methods for sending events to the provider
/// Events are added to a queue to be processewd on a different thread
/// If events are unable to be send to the provider, then they will be held in the queue
class CSP_API AnalyticsSystem
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    friend void csp::memory::Delete<AnalyticsSystem>(AnalyticsSystem* Ptr);
    /** @endcond */
    CSP_END_IGNORE

public:
    CSP_START_IGNORE
    AnalyticsSystem(const AnalyticsSystem&) = delete;
    AnalyticsSystem(AnalyticsSystem&&) = delete;

    AnalyticsSystem& operator=(const AnalyticsSystem&) = delete;
    AnalyticsSystem& operator=(AnalyticsSystem&&) = delete;
    CSP_END_IGNORE

    /// @brief Send an event
    /// @param Event AnalyticsEvent
    void Log(AnalyticsEvent* Event);

    CSP_START_IGNORE
    void RegisterProvider(IAnalyticsProvider* Provider);
    void DeregisterProvider(IAnalyticsProvider* Provider);
    CSP_END_IGNORE

    CSP_START_IGNORE
    static const int QueueSize = 1024;
    CSP_END_IGNORE

private:
    AnalyticsSystem();
    ~AnalyticsSystem();

    AnalyticsSystemImpl* Impl;
};
} // namespace csp::systems
