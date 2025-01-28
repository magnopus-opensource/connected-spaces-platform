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

#include "CSP/Systems/Analytics/AnalyticsSystemUtils.h"

class EventPayloadImpl;

namespace csp::systems
{

using MetricValue = csp::multiplayer::ReplicatedValue;

/// @ingroup Analytics System
/// @brief Interface for an Analytics Provider
class CSP_API IAnalyticsProvider
{
public:
    CSP_START_IGNORE
    IAnalyticsProvider(const IAnalyticsProvider&) = delete;
    IAnalyticsProvider(IAnalyticsProvider&&) = delete;

    IAnalyticsProvider& operator=(const IAnalyticsProvider&) = delete;
    IAnalyticsProvider& operator=(IAnalyticsProvider&&) = delete;

    virtual ~IAnalyticsProvider() = default;

    virtual void Log(AnalyticsEvent* Event) = 0;
    CSP_END_IGNORE

protected:
    IAnalyticsProvider() = default;
};

} // namespace csp::systems
