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

#include <iomanip>
#include <sstream>
#include <stdio.h>

namespace
{

#ifdef CSP_WINDOWS
#define timegm _mkgmtime
#endif

} // namespace

namespace csp::common
{

// Portable implementation of timegm - since it is not part of the C++ standard
// we cannot assume it is reliably implemented across platforms, and is in fact
// known to have issues with Emscripten, for example.
// Based on https://stackoverflow.com/questions/530519/stdmktime-and-timezone-info.
// Note - we rely on the `Time` parameter that is passed adhering to the cpp spec
// https://en.cppreference.com/w/c/chrono/tm.

time_t CSPTimeGM(struct tm* time)
{
    // note - we use this constant array for calculating days into the year instead of
    // relying on tm::tm_yday, as we expect calling code to more reliably provide values for `tm::tm_mon`
    constexpr long long cumulativeDaysInYear[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

    const long long year = 1900LL + time->tm_year;
    time_t result = (year - 1970) * 365 + cumulativeDaysInYear[time->tm_mon];

    // leap year offsets
    {
        result += (year - 1968) / 4;
        result -= (year - 1900) / 100;
        result += (year - 1600) / 400;

        if ((year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0) && time->tm_mon < 2)
        {
            result--;
        }
    }

    result += static_cast<time_t>(time->tm_mday) - 1;
    result *= 24;
    result += time->tm_hour;
    result *= 60;
    result += time->tm_min;
    result *= 60;
    result += time->tm_sec;

    if (time->tm_isdst == 1)
    {
        result -= 3600;
    }

    return result;
}

DateTime::DateTime(const csp::common::String& dateString)
{
    int year, day, month, hour, minute, second, fraction, offsetHours, offsetMinutes;
    char offsetModifier;
#ifdef _MSC_VER
    sscanf_s(dateString.c_str(), "%d-%d-%dT%d:%d:%d.%d%c%d:%d", &year, &month, &day, &hour, &minute, &second, &fraction, &offsetModifier, 1,
        &offsetHours, &offsetMinutes);
#else
    std::sscanf(DateString.c_str(), "%d-%d-%dT%d:%d:%d.%d%c%d:%d", &Year, &Month, &Day, &Hour, &Minute, &Second, &Fraction, &OffsetModifier,
        &OffsetHours, &OffsetMinutes);
#endif

    std::tm tm = {
        second, // tm_sec
        minute, // tm_min
        hour, // tm_hour
        day, // tm_mday
        month - 1, // tm_mon
        year - 1900, // tm_year
        0, // tm_wday
        0, // tm_yday
        -1, // tm_isdst
#ifndef CSP_WINDOWS
        0, // tm_gmtoff
        nullptr // tm_zone
#endif
    };

    if (offsetModifier == '-')
    {
        offsetHours = 0 - offsetHours;
    }

    // TODO: Use OffsetHours and OffsetMinutes in case CHS decides not to send datetimes in UTC in the future
    const std::time_t time = CSPTimeGM(&tm);
    m_timePoint = std::chrono::system_clock::from_time_t(time);
}

const DateTime::Clock::time_point& DateTime::GetTimePoint() const { return m_timePoint; }

csp::common::String DateTime::GetUtcString() const
{
    std::time_t now = std::chrono::system_clock::to_time_t(m_timePoint);
    std::tm utcTime = *std::gmtime(&now);

    std::ostringstream utcStream;
    utcStream << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");

    std::string utcString = utcStream.str();
    return utcString.c_str();
}

} // namespace csp::common
