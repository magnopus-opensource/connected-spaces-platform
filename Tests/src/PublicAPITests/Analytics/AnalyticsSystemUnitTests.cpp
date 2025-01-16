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
#include "TestHelpers.h"

#include "gtest/gtest.h"

// All the analytics tests will be reviewed, and the disabled tests reenabled, as part of OF-1538.

#if RUN_ALL_UNIT_TESTS || RUN_ANALYTICSSYSTEM_UNIT_TESTS || RUN_ANALYTICSSYSTEM_LOG_METRIC_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, AnalyticsSystemUnitTests, LogMetricTest)
{
    auto* AnalyticsSystem = csp::systems::SystemsManager::Get().GetAnalyticsSystem();

    // Create our test provider
    TestAnalyticsProvider Provider;

    AnalyticsSystem->RegisterProvider(&Provider);

    // Create metric value
    const csp::common::String TestMetricTag = "TestTag";
    const int TestMetricValue = 10;

    csp::systems::AnalyticsEvent* Event = INIT_EVENT(TestMetricTag);
    Event->AddInt("Value", TestMetricValue);

    // Send metric
    AnalyticsSystem->Log(Event);

    // Call tick to process analytics events
    csp::CSPFoundation::Tick();

    const std::vector<csp::systems::AnalyticsEvent>& Metrics = Provider.GetMetrics();

    EXPECT_EQ(Metrics.size(), 1);
    EXPECT_EQ(Metrics[0].GetTag(), TestMetricTag);
    EXPECT_EQ(Metrics[0].GetInt("Value"), TestMetricValue);

    AnalyticsSystem->DeregisterProvider(&Provider);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANALYTICSSYSTEM_UNIT_TESTS || RUN_ANALYTICSSYSTEM_LOG_MULTIPLE_METRIC_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, AnalyticsSystemUnitTests, LogMultipleMetricTest)
{
    auto* AnalyticsSystem = csp::systems::SystemsManager::Get().GetAnalyticsSystem();

    // Create our test provider
    TestAnalyticsProvider Provider;

    AnalyticsSystem->RegisterProvider(&Provider);

    // Create metric values
    const csp::common::String TestMetricTag1 = "TestTag";
    const int TestMetricValue1 = 10;

    const csp::common::String TestMetricTag2 = "TestTag2";
    const int TestMetricValue2 = 20;

    const csp::common::String TestMetricTag3 = "TestTag3";
    const int TestMetricValue3 = 30;

    csp::systems::AnalyticsEvent* Event = INIT_EVENT(TestMetricTag1);
    Event->AddInt("Value", TestMetricValue1);

    csp::systems::AnalyticsEvent* Event2 = INIT_EVENT(TestMetricTag2);
    Event2->AddInt("Value", TestMetricValue2);

    csp::systems::AnalyticsEvent* Event3 = INIT_EVENT(TestMetricTag3);
    Event3->AddInt("Value", TestMetricValue3);

    // Send metrics
    AnalyticsSystem->Log(Event);
    AnalyticsSystem->Log(Event2);
    AnalyticsSystem->Log(Event3);

    // Call tick to process analytics events
    csp::CSPFoundation::Tick();

    const std::vector<csp::systems::AnalyticsEvent>& Metrics = Provider.GetMetrics();

    EXPECT_EQ(Metrics.size(), 3);

    EXPECT_EQ(Metrics[0].GetTag(), TestMetricTag1);
    EXPECT_EQ(Metrics[0].GetInt("Value"), TestMetricValue1);

    EXPECT_EQ(Metrics[1].GetTag(), TestMetricTag2);
    EXPECT_EQ(Metrics[1].GetInt("Value"), TestMetricValue2);

    EXPECT_EQ(Metrics[2].GetTag(), TestMetricTag3);
    EXPECT_EQ(Metrics[2].GetInt("Value"), TestMetricValue3);

    AnalyticsSystem->DeregisterProvider(&Provider);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANALYTICSSYSTEM_UNIT_TESTS || RUN_ANALYTICSSYSTEM_DEREGISTER_PROVIDER_TEST
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemUnitTests, DeregisterProviderTest)
{
    auto* AnalyticsSystem = csp::systems::SystemsManager::Get().GetAnalyticsSystem();

    // Create our test provider
    TestAnalyticsProvider Provider;

    AnalyticsSystem->RegisterProvider(&Provider);

    // Create metric value
    const csp::common::String TestMetricTag = "TestTag";
    const int TestMetricValue = 10;

    csp::systems::AnalyticsEvent* Event = INIT_EVENT(TestMetricTag);
    Event->AddInt("Value", TestMetricValue);

    AnalyticsSystem->DeregisterProvider(&Provider);

    // Send metric
    AnalyticsSystem->Log(Event);

    // Call tick to process analytics events
    csp::CSPFoundation::Tick();

    const std::vector<csp::systems::AnalyticsEvent>& Metrics = Provider.GetMetrics();

    EXPECT_EQ(Metrics.size(), 0);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANALYTICSSYSTEM_UNIT_TESTS || RUN_ANALYTICSSYSTEM_MULTIPLE_THREADS_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, AnalyticsSystemUnitTests, MultipleThreadsTest)
{
    auto* AnalyticsSystem = csp::systems::SystemsManager::Get().GetAnalyticsSystem();

    // Create our test provider
    TestAnalyticsProvider Provider;

    AnalyticsSystem->RegisterProvider(&Provider);

    const int ThreadCount = 5;
    bool Start = false;

    std::vector<std::thread> Threads;

    for (int i = 0; i < ThreadCount; i++)
    {
        Threads.push_back(std::thread { [&Start, AnalyticsSystem]()
            {
                while (!Start)
                {
                }

                // Create metric value
                const csp::common::String TestMetricTag = "TestTag";
                const int TestMetricValue = 10;

                csp::systems::AnalyticsEvent* Event = INIT_EVENT(TestMetricTag);
                Event->AddInt("Value", TestMetricValue);

                // Send metric
                AnalyticsSystem->Log(Event);
            } });
    }

    // Send event on all threads
    Start = true;

    // Wait for threads to complete
    for (auto& Thread : Threads)
    {
        Thread.join();
    }

    // Call tick to process analytics events
    csp::CSPFoundation::Tick();

    const std::vector<csp::systems::AnalyticsEvent>& Metrics = Provider.GetMetrics();

    EXPECT_EQ(Metrics.size(), ThreadCount);

    AnalyticsSystem->DeregisterProvider(&Provider);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANALYTICSSYSTEM_UNIT_TESTS || RUN_ANALYTICSSYSTEM_UA_PARAMS_TEST
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemUnitTests, UAParamsTest)
{
    const csp::common::String TestClientId = "TestClientId";
    const csp::common::String TestProperty = "TestProperty";

    const csp::common::String TestMetricTag = "TestTag";
    const int TestMetricValue1 = 10;
    const csp::common::String TestMetricValue2 = "TestValue2";

    csp::systems::AnalyticsEvent* Event = INIT_EVENT(TestMetricTag);
    Event->AddInt("Value1", TestMetricValue1);
    Event->AddString("Value2", TestMetricValue2);

    const csp::common::String ExpectedEventString = "v=1&tid=TestProperty&cid=TestClientId&t=event&ec=event&ea=TestTag&ev=10&el=TestValue2";
    csp::common::String EventString = csp::systems::CreateUAEventString(TestClientId, TestProperty, Event);

    EXPECT_EQ(ExpectedEventString, EventString);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANALYTICSSYSTEM_UNIT_TESTS || RUN_ANALYTICSSYSTEM_UA_INT_PARAM_TEST
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemUnitTests, UAIntParamTest)
{
    const csp::common::String TestClientId = "TestClientId";
    const csp::common::String TestProperty = "TestProperty";

    const csp::common::String TestMetricTag = "TestTag";
    const int TestMetricValue1 = 10;

    csp::systems::AnalyticsEvent* Event = INIT_EVENT(TestMetricTag);
    Event->AddInt("Value1", TestMetricValue1);

    const csp::common::String ExpectedEventString = "v=1&tid=TestProperty&cid=TestClientId&t=event&ec=event&ea=TestTag&ev=10";
    csp::common::String EventString = csp::systems::CreateUAEventString(TestClientId, TestProperty, Event);

    EXPECT_EQ(ExpectedEventString, EventString);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANALYTICSSYSTEM_UNIT_TESTS || RUN_ANALYTICSSYSTEM_UA_STRING_PARAM_TEST
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemUnitTests, UAStringParamTest)
{
    const csp::common::String TestClientId = "TestClientId";
    const csp::common::String TestProperty = "TestProperty";

    const csp::common::String TestMetricTag = "TestTag";
    const csp::common::String TestMetricValue1 = "TestValue2";

    csp::systems::AnalyticsEvent* Event = INIT_EVENT(TestMetricTag);
    Event->AddString("Value1", TestMetricValue1);

    const csp::common::String ExpectedEventString = "v=1&tid=TestProperty&cid=TestClientId&t=event&ec=event&ea=TestTag&el=TestValue2";
    csp::common::String EventString = csp::systems::CreateUAEventString(TestClientId, TestProperty, Event);

    EXPECT_EQ(ExpectedEventString, EventString);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANALYTICSSYSTEM_UNIT_TESTS || RUN_ANALYTICSSYSTEM_UA_INVALID_PARAM_TEST
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemUnitTests, UAInvalidParamTest)
{
    const csp::common::String TestClientId = "TestClientId";
    const csp::common::String TestProperty = "TestProperty";

    const csp::common::String TestMetricTag = "TestTag";
    const bool TestMetricValue1 = true;

    csp::systems::AnalyticsEvent* Event = INIT_EVENT(TestMetricTag);
    Event->AddBool("Value1", TestMetricValue1);

    const csp::common::String ExpectedEventString = "";
    csp::common::String EventString = csp::systems::CreateUAEventString(TestClientId, TestProperty, Event);

    EXPECT_EQ(ExpectedEventString, EventString);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANALYTICSSYSTEM_UNIT_TESTS || RUN_ANALYTICSSYSTEM_UA_TOO_MANY_PARAM_TEST
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemUnitTests, UATooManyParamTest)
{
    const csp::common::String TestClientId = "TestClientId";
    const csp::common::String TestProperty = "TestProperty";

    const csp::common::String TestMetricTag = "TestTag";
    const int TestMetricValue1 = 10;
    const csp::common::String TestMetricValue2 = "TestValue2";
    const csp::common::String TestMetricValue3 = "TestValue3";

    csp::systems::AnalyticsEvent* Event = INIT_EVENT(TestMetricTag);
    Event->AddInt("Value1", TestMetricValue1);
    Event->AddString("Value2", TestMetricValue2);
    Event->AddString("Value3", TestMetricValue3);

    const csp::common::String ExpectedEventString = "";
    csp::common::String EventString = csp::systems::CreateUAEventString(TestClientId, TestProperty, Event);

    EXPECT_EQ(ExpectedEventString, EventString);
}
#endif
