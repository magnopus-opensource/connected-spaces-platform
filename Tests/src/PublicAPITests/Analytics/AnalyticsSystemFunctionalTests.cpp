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

#include "AnalyticsSystemTestHelpers.h"
#include "CSP/Systems/Analytics/AnalyticsProvider.h"
#include "CSP/Systems/Analytics/AnalyticsProviderGoogleUA.h"
#include "CSP/Systems/Analytics/AnalyticsSystem.h"
#include "CSP/Systems/Analytics/AnalyticsSystemUtils.h"
#include "CSP/Systems/SystemsManager.h"
#include "Systems/Analytics/Analytics.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

// All the analytics tests will be reviewed, and the disabled tests reenabled, as part of OF-1538.

#if RUN_ALL_UNIT_TESTS || RUN_ANALYTICSSYSTEM_TESTS || RUN_ANALYTICSSYSTEM_MACRO_LOG_METRIC_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, AnalyticsSystemTests, MacroLogMetricTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    csp::systems::AnalyticsSystem* System = SystemsManager.GetAnalyticsSystem();

    // Create our test provider
    TestAnalyticsProvider Provider;

    System->RegisterProvider(&Provider);

    // Create metric value
    const csp::common::String TestMetricTag = "TestTag";
    const int TestMetricValue = 10;

    csp::systems::AnalyticsEvent* Event = INIT_EVENT(TestMetricTag);
    Event->AddInt("Value", TestMetricValue);

    // Send metric
    CSP_ANALYTICS_LOG_EVENT(Event);

    // Call tick to process analytics events
    csp::CSPFoundation::Tick();

    const std::vector<csp::systems::AnalyticsEvent>& Metrics = Provider.GetMetrics();

    EXPECT_EQ(Metrics.size(), 1);
    EXPECT_EQ(Metrics[0].GetTag(), TestMetricTag);
    EXPECT_EQ(Metrics[0].GetInt("Value"), TestMetricValue);

    System->DeregisterProvider(&Provider);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANALYTICSSYSTEM_TESTS || RUN_ANALYTICSSYSTEM_UA_TEST
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, UATest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    csp::systems::AnalyticsSystem* System = SystemsManager.GetAnalyticsSystem();

    csp::systems::AnalyticsProviderGoogleUA Provider("11111", "22222");

    System->RegisterProvider(&Provider);

    {
        csp::systems::AnalyticsEvent* Event = INIT_EVENT("session_start");
        System->Log(Event);
    }

    // Call tick to process analytics events
    csp::CSPFoundation::Tick();

    {
        csp::systems::AnalyticsEvent* Event = INIT_EVENT("object_interact_start");
        Event->AddString("object_name", "some_object");
        System->Log(Event);
    }

    csp::CSPFoundation::Tick();

    {
        csp::systems::AnalyticsEvent* Event = INIT_EVENT("object_interact_end");
        Event->AddString("object_name", "some_object");
        System->Log(Event);
    }

    {
        csp::systems::AnalyticsEvent* Event = INIT_EVENT("chat_start");
        Event->AddString("chat_type", "video");
        System->Log(Event);
    }

    csp::CSPFoundation::Tick();

    {
        csp::systems::AnalyticsEvent* Event = INIT_EVENT("chat_end");
        Event->AddString("chat_type", "video");
        System->Log(Event);
    }

    {
        csp::systems::AnalyticsEvent* Event = INIT_EVENT("session_end");
        System->Log(Event);
    }

    csp::CSPFoundation::Tick();
}
#endif