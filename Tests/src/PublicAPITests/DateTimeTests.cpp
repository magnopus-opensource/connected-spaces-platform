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

#if RUN_ALL_UNIT_TESTS || RUN_DATE_TIME_TESTS || 1
CSP_PUBLIC_TEST(CSPEngine, DateTimeTests, UTCStringConversion)
{
	const csp::common::String UTCString("2122-04-30T02:00:00+0000");
	const csp::common::DateTime Date(UTCString);

	auto TimePoint				 = Date.GetTimePoint();
	auto Time					 = std::chrono::system_clock::to_time_t(TimePoint);
	const std::tm* const UTCTime = std::gmtime(&Time);

	EXPECT_EQ(UTCTime->tm_year, 2122 - 1900); // tm_year is years since 1900
	EXPECT_EQ(UTCTime->tm_mon, 3);			  // tm_mon is zero-indexed
	EXPECT_EQ(UTCTime->tm_mday, 30);
	EXPECT_EQ(UTCTime->tm_hour, 2);
	EXPECT_EQ(UTCTime->tm_min, 0);
	EXPECT_EQ(UTCTime->tm_sec, 0);
}
#endif