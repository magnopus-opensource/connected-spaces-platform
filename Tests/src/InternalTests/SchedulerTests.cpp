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

#if !defined(SKIP_INTERNAL_TESTS) || defined(RUN_SCHEDULER_TESTS)
#include "Common/DateTime.h"
#include "Common/Scheduler.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

using namespace std::chrono_literals;

CSP_INTERNAL_TEST(CSPEngine, SchedulerTests, SchedulerTest)
{
    int WaitForTestTimeoutCountMs = 0;
    int KeepAliveInterval = 10000;
    bool ScheduleCallback = false;

    std::chrono::system_clock::time_point test = std::chrono::system_clock::now() + std::chrono::system_clock::duration(5s);
    csp::common::DateTime RefreshTime(test);

    csp::GetScheduler()->ScheduleAt(RefreshTime, [this, &ScheduleCallback]() { ScheduleCallback = true; });

    while (WaitForTestTimeoutCountMs < KeepAliveInterval)
    {
        std::this_thread::sleep_for(20ms);
        WaitForTestTimeoutCountMs += 20;
    }

    EXPECT_TRUE(ScheduleCallback);
}
#endif