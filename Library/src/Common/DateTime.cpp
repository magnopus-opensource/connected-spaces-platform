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

time_t CSPTimeGM(struct tm* Time)
{
    // note - we use this constant array for calculating days into the year instead of
    // relying on tm::tm_yday, as we expect calling code to more reliably provide values for `tm::tm_mon`
    constexpr long long CumulativeDaysInYear[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

    const long long Year = 1900LL + Time->tm_year;
    time_t Result = (Year - 1970) * 365 + CumulativeDaysInYear[Time->tm_mon];

    // leap year offsets
    {
        Result += (Year - 1968) / 4;
        Result -= (Year - 1900) / 100;
        Result += (Year - 1600) / 400;

        if ((Year % 4) == 0 && ((Year % 100) != 0 || (Year % 400) == 0) && Time->tm_mon < 2)
        {
            Result--;
        }
    }

    Result += static_cast<time_t>(Time->tm_mday) - 1;
    Result *= 24;
    Result += Time->tm_hour;
    Result *= 60;
    Result += Time->tm_min;
    Result *= 60;
    Result += Time->tm_sec;

    if (Time->tm_isdst == 1)
    {
        Result -= 3600;
    }

    return Result;
}

DateTime::DateTime(const csp::common::String& DateString)
{
    int Year, Day, Month, Hour, Minute, Second, Fraction, OffsetHours, OffsetMinutes;
    char OffsetModifier;
#ifdef _MSC_VER
    int _ = sscanf_s(DateString.c_str(), "%d-%d-%dT%d:%d:%d.%d%c%d:%d", &Year, &Month, &Day, &Hour, &Minute, &Second, &Fraction, &OffsetModifier, 1,
        &OffsetHours, &OffsetMinutes);
#else
    int _ = std::sscanf(DateString.c_str(), "%d-%d-%dT%d:%d:%d.%d%c%d:%d", &Year, &Month, &Day, &Hour, &Minute, &Second, &Fraction, &OffsetModifier,
        &OffsetHours, &OffsetMinutes);
#endif

    std::tm TM = { Second, Minute, Hour, Day, Month - 1, Year - 1900 };
    TM.tm_isdst = -1;

    if (OffsetModifier == '-')
    {
        OffsetHours = 0 - OffsetHours;
    }

    // TODO: Use OffsetHours and OffsetMinutes in case CHS decides not to send datetimes in UTC in the future
    const std::time_t Time = CSPTimeGM(&TM);
    TimePoint = std::chrono::system_clock::from_time_t(Time);
}

const DateTime::Clock::time_point& DateTime::GetTimePoint() const { return TimePoint; }

} // namespace csp::common
