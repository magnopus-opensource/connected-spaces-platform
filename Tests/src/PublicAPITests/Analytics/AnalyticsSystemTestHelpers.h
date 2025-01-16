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

#include "CSP/Systems/Analytics/AnalyticsProvider.h"
#include "CSP/Systems/Analytics/AnalyticsSystemUtils.h"

#include <chrono>

class TestAnalyticsProvider : public csp::systems::IAnalyticsProvider
{
public:
    const std::vector<csp::systems::AnalyticsEvent>& GetMetrics() const { return Metrics; }

protected:
    void Log(csp::systems::AnalyticsEvent* Event) override { Metrics.push_back(*Event); }

private:
    std::vector<csp::systems::AnalyticsEvent> Metrics;
};

static constexpr const std::chrono::milliseconds TestWaitTime { 250 };