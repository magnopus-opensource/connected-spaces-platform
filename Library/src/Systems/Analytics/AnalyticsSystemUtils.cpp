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
#include "CSP/Systems/Analytics/AnalyticsSystemUtils.h"

#include "Memory/Memory.h"

namespace csp::systems
{

void AnalyticsEvent::AddInt(csp::common::String Key, int64_t Value)
{
    MetricValue Metric;
    Metric.SetInt(Value);

    Parameters[Key] = Metric;
}

void AnalyticsEvent::AddString(csp::common::String Key, const csp::common::String& Value)
{
    MetricValue Metric;
    Metric.SetString(Value);

    Parameters[Key] = Metric;
}

void AnalyticsEvent::AddFloat(csp::common::String Key, float Value)
{
    MetricValue Metric;
    Metric.SetFloat(Value);

    Parameters[Key] = Metric;
}

void AnalyticsEvent::AddBool(csp::common::String Key, bool Value)
{
    MetricValue Metric;
    Metric.SetBool(Value);

    Parameters[Key] = Metric;
}

const int64_t AnalyticsEvent::GetInt(csp::common::String Key) const { return Parameters[Key].GetInt(); }

const csp::common::String& AnalyticsEvent::GetString(csp::common::String Key) const { return Parameters[Key].GetString(); }

const float AnalyticsEvent::GetFloat(csp::common::String Key) const { return Parameters[Key].GetFloat(); }

bool AnalyticsEvent::GetBool(csp::common::String Key) const { return Parameters[Key].GetBool(); }

const csp::common::String& AnalyticsEvent::GetTag() const { return Tag; }

const csp::common::Map<csp::common::String, MetricValue>& AnalyticsEvent::GetParams() const { return Parameters; }

AnalyticsEvent::AnalyticsEvent(const csp::common::String& Tag)
    : Tag { Tag }
{
}

AnalyticsEvent* AnalyticsEventInitialiser::Initialise(const csp::common::String Tag) { return CSP_NEW AnalyticsEvent(Tag); }

void AnalyticsEventInitialiser::DeInitialise(AnalyticsEvent* Event) { CSP_DELETE(Event); }
} // namespace csp::systems
