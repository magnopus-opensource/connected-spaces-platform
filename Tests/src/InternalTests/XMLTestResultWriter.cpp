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

#include "XMLTestResultWriter.h"

#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>

static bool PortableLocaltime(time_t seconds, struct tm* out)
{
#if defined(_MSC_VER)
    return localtime_s(out, &seconds) == 0;
#elif defined(__MINGW32__) || defined(__MINGW64__)
    // MINGW <time.h> provides neither localtime_r nor localtime_s, but uses
    // Windows' localtime(), which has a thread-local tm buffer.
    struct tm* tm_ptr = localtime(&seconds); // NOLINT
    if (tm_ptr == nullptr)
        return false;
    *out = *tm_ptr;
    return true;
#elif defined(__STDC_LIB_EXT1__)
    // Uses localtime_s when available as localtime_r is only available from
    // C23 standard.
    return localtime_s(&seconds, out) != nullptr;
#else
    return localtime_r(&seconds, out) != nullptr;
#endif
}

std::string FormatTimeInMillisAsSeconds(testing::internal::TimeInMillis ms)
{
    ::std::stringstream ss;
    // For the exact N seconds, makes sure output has a trailing decimal point.
    // Sets precision so that we won't have many trailing zeros (e.g., 300 ms
    // will be just 0.3, 410 ms 0.41, and so on)
    ss << std::fixed << std::setprecision(ms % 1000 == 0 ? 0 : (ms % 100 == 0 ? 1 : (ms % 10 == 0 ? 2 : 3))) << std::showpoint;
    ss << (static_cast<double>(ms) * 1e-3);
    return ss.str();
}

std::string FormatEpochTimeInMillisAsIso8601(testing::internal::TimeInMillis ms)
{
    struct tm timeStruct;
    if (!PortableLocaltime(static_cast<time_t>(ms / 1000), &timeStruct))
        return "";
    // YYYY-MM-DDThh:mm:ss.sss
    return testing::internal::StreamableToString(timeStruct.tm_year + 1900) + "-"
        + testing::internal::String::FormatIntWidth2(timeStruct.tm_mon + 1) + "-" + testing::internal::String::FormatIntWidth2(timeStruct.tm_mday)
        + "T" + testing::internal::String::FormatIntWidth2(timeStruct.tm_hour) + ":" + testing::internal::String::FormatIntWidth2(timeStruct.tm_min)
        + ":" + testing::internal::String::FormatIntWidth2(timeStruct.tm_sec) + "."
        + testing::internal::String::FormatIntWidthN(static_cast<int>(ms % 1000), 3);
}

void TestListener::OnTestIterationEnd(const testing::UnitTest& unitTest, int /*iteration*/)
{
    std::stringstream ss;

    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<testsuites tests=\"" << unitTest.reportable_test_count() << "\" failures=\"" << unitTest.failed_test_count()
       << "\" disabled=\"0\" errors=\"0\" time=\"" << FormatTimeInMillisAsSeconds(unitTest.elapsed_time()) << "\" timestamp=\""
       << FormatEpochTimeInMillisAsIso8601(unitTest.start_timestamp()) << "\" name =\"AllTests\">\n";

    int suiteCount = unitTest.total_test_suite_count();

    for (int i = 0; i < suiteCount; ++i)
    {
        const testing::TestSuite* suite = unitTest.GetTestSuite(i);

        ss << "  <testsuite name=\"" << suite->name() << "\" tests=\"" << suite->total_test_count() << "\" failures=\"" << suite->failed_test_count()
           << "\" disabled=\"0\" skipped=\"0\" errors=\"0\" time=\"" << FormatTimeInMillisAsSeconds(suite->elapsed_time()) << "\" timestamp=\""
           << FormatEpochTimeInMillisAsIso8601(suite->start_timestamp()) << "\" classname=\"" << suite->name() << "\">\n";

        int testCount = suite->reportable_test_count();

        for (int j = 0; j < testCount; ++j)
        {
            const testing::TestInfo* test = suite->GetTestInfo(j);
            const testing::TestResult* result = test->result();

            ss << "    <testcase name=\"" << test->name() << "\" status=\"run\" result=\"completed\" time=\""
               << FormatTimeInMillisAsSeconds(result->elapsed_time()) << "\" timestamp=\""
               << FormatEpochTimeInMillisAsIso8601(result->start_timestamp()) << "\" classname=\"" << suite->name() << "\"";

            if (result->Passed())
            {
                ss << " />\n";
            }
            else
            {
                ss << ">\n";
                ss << "      <failure message=\"TODO: Print real error message.\" type=\"\"><![CDATA[TODO: Print real error "
                      "message.]]></failure>\n";
                ss << "    </testcase>\n";
            }
        }
        ss << "  </testsuite>\n";
    }

    ss << "</testsuites>\n";

    std::cout << ss.str();
}
