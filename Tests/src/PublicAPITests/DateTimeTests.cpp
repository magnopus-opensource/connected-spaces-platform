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

#include "Common/DateTime.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <chrono>

#if RUN_ALL_UNIT_TESTS || RUN_DATE_TIME_TESTS || RUN_DATE_TIME_STRING_CONVERSION_TESTS
CSP_PUBLIC_TEST(CSPEngine, DateTimeTests, UTCStringConversion)
{
    {
        const csp::common::String UTCString("1999-06-12T08:24:21+00:00");
        const csp::common::DateTime Date(UTCString);

        auto TimePoint = Date.GetTimePoint();
        auto Time = std::chrono::system_clock::to_time_t(TimePoint);
        const std::tm* const UTCTime = std::gmtime(&Time);

        EXPECT_EQ(UTCTime->tm_year, 1999 - 1900);
        EXPECT_EQ(UTCTime->tm_mon, 5);
        EXPECT_EQ(UTCTime->tm_mday, 12);
        EXPECT_EQ(UTCTime->tm_hour, 8);
        EXPECT_EQ(UTCTime->tm_min, 24);
        EXPECT_EQ(UTCTime->tm_sec, 21);
    }

    {
        // testing the very start of a leap year
        const csp::common::String UTCString("2004-01-01T00:00:00+00:00");
        const csp::common::DateTime Date(UTCString);

        auto TimePoint = Date.GetTimePoint();
        auto Time = std::chrono::system_clock::to_time_t(TimePoint);
        const std::tm* const UTCTime = std::gmtime(&Time);

        EXPECT_EQ(UTCTime->tm_year, 2004 - 1900);
        EXPECT_EQ(UTCTime->tm_mon, 0);
        EXPECT_EQ(UTCTime->tm_mday, 1);
        EXPECT_EQ(UTCTime->tm_hour, 0);
        EXPECT_EQ(UTCTime->tm_min, 0);
        EXPECT_EQ(UTCTime->tm_sec, 00);
    }

    {
        // testing the very end of a year
        const csp::common::String UTCString("1999-12-31T23:59:59+00:00");
        const csp::common::DateTime Date(UTCString);

        auto TimePoint = Date.GetTimePoint();
        auto Time = std::chrono::system_clock::to_time_t(TimePoint);
        const std::tm* const UTCTime = std::gmtime(&Time);

        EXPECT_EQ(UTCTime->tm_year, 1999 - 1900);
        EXPECT_EQ(UTCTime->tm_mon, 11);
        EXPECT_EQ(UTCTime->tm_mday, 31);
        EXPECT_EQ(UTCTime->tm_hour, 23);
        EXPECT_EQ(UTCTime->tm_min, 59);
        EXPECT_EQ(UTCTime->tm_sec, 59);
    }

    {
        // this UTC date is out of the int32-representable range since the 1980 epoch.
        // We expect this to pass, as we expect our code to work with 64bit date/time representations.
        const csp::common::String UTCString("2122-04-30T02:30:54+00:00");
        const csp::common::DateTime Date(UTCString);

        auto TimePoint = Date.GetTimePoint();
        auto Time = std::chrono::system_clock::to_time_t(TimePoint);
        const std::tm* const UTCTime = std::gmtime(&Time);

        EXPECT_EQ(UTCTime->tm_year, 2122 - 1900); // tm_year is years since 1900
        EXPECT_EQ(UTCTime->tm_mon, 3); // tm_mon is zero-indexed
        EXPECT_EQ(UTCTime->tm_mday, 30);
        EXPECT_EQ(UTCTime->tm_hour, 2);
        EXPECT_EQ(UTCTime->tm_min, 30);
        EXPECT_EQ(UTCTime->tm_sec, 54);
    }
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_DATE_TIME_TESTS || RUN_DATE_TIME_COMPARSION
CSP_PUBLIC_TEST(CSPEngine, DateTimeTests, Comparison)
{
    using namespace std::chrono_literals;

    const csp::common::DateTime TimeNow = csp::common::DateTime::UtcTimeNow();
    const std::chrono::system_clock::time_point TimeFuture = TimeNow.GetTimePoint() + std::chrono::system_clock::duration(5min);

    const csp::common::DateTime CurrentDateTime(TimeNow);
    const csp::common::DateTime FutureDateTime(TimeFuture);
    ASSERT_GE(FutureDateTime, CurrentDateTime);
}
#endif