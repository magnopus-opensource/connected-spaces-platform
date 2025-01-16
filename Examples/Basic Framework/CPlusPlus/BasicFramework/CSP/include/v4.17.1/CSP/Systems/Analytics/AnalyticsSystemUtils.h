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
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ReplicatedValue.h"

namespace csp::systems
{
using MetricValue = csp::multiplayer::ReplicatedValue;

class CSP_API AnalyticsEvent
{
    friend class AnalyticsEventInitialiser;

public:
    void AddInt(csp::common::String Key, int64_t Value);
    void AddString(csp::common::String Key, const csp::common::String& Value);
    void AddFloat(csp::common::String Key, float Value);
    void AddBool(csp::common::String Key, bool Value);

    const int64_t GetInt(csp::common::String Key) const;
    const csp::common::String& GetString(csp::common::String Key) const;
    const float GetFloat(csp::common::String Key) const;
    bool GetBool(csp::common::String Key) const;

    const csp::common::String& GetTag() const;

    CSP_START_IGNORE
    const csp::common::Map<csp::common::String, MetricValue>& GetParams() const;
    CSP_END_IGNORE

private:
    AnalyticsEvent(const csp::common::String& Tag);

    csp::common::String Tag;

    csp::common::Map<csp::common::String, MetricValue> Parameters;
};

class CSP_API AnalyticsEventInitialiser
{
public:
    static AnalyticsEvent* Initialise(const csp::common::String Tag);
    static void DeInitialise(AnalyticsEvent* Event);
};

#define INIT_EVENT(T) csp::systems::AnalyticsEventInitialiser::Initialise(T)
#define DEINIT_EVENT(E) csp::systems::AnalyticsEventInitialiser::DeInitialise(E)

} // namespace csp::systems
