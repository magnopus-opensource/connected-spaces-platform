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

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"

#include <chrono>
#include <ctime>
#include <string>


PRAGMA_WARNING_PUSH()
PRAGMA_WARNING_IGNORE_MSVC(4996) // gmtime/localtime unsafe warnings


namespace csp::common
{

/// @brief Represents date and time.
class DateTime
{
public:
	using Clock = std::chrono::system_clock;

	/// @brief Constructs an empty DateTime.
	DateTime()
	{
	}

	/// @brief Constructs a DateTime from a timepoint.
	/// @param InTimePoint Clock::time_point : Timepoint to constuct DateTime from
	explicit DateTime(Clock::time_point InTimePoint) : TimePoint(InTimePoint)
	{
	}

	/// @ brief Constructs a DateTime from a date string.
	///
	/// The string parameter is expected to be of the ISO 8601 format YYYY-MM-DDThh:mm:ss.sTZD (eg 1997-07-16T19:20:30.45+01:00)
	/// where:
	/// YYYY = four-digit year
	/// MM   = two-digit month (01=January, etc.)
	/// DD   = two-digit day of month (01 through 31)
	/// hh   = two digits of hour (00 through 23) (am/pm NOT allowed)
	/// mm   = two digits of minute (00 through 59)
	/// ss   = two digits of second (00 through 59)
	/// s    = one or more digits representing a decimal fraction of a second
	/// TZD  = time zone designator (Z or +hh:mm or -hh:mm)
	///
	/// @param DateString const csp::common::String& : Date string to constuct DateTime from.
	explicit DateTime(const String& DateString);

	/// @brief Constructs a DateTime with the current utc time.
	static DateTime TimeNow()
	{
		return DateTime(Clock::now());
	}

	/// @brief Constructs a Datetime with the current time.
	/// @return DateTime
	static DateTime UtcTimeNow()
	{
		DateTime DateTimeNow;

		time_t Now = time(0);
		tm* Gmtm   = gmtime(&Now);

		if (Gmtm != nullptr)
		{
			std::string TimeString;
			TimeString = std::to_string(1900 + Gmtm->tm_year);
			TimeString += "-";
			TimeString += std::to_string(Gmtm->tm_mon + 1);
			TimeString += "-";
			TimeString += std::to_string(Gmtm->tm_mday);
			TimeString += "T";
			TimeString += std::to_string(Gmtm->tm_hour);
			TimeString += ":";
			TimeString += std::to_string(Gmtm->tm_min);
			TimeString += ":";
			TimeString += std::to_string(Gmtm->tm_sec);
			TimeString += ".";
			TimeString += "0+";
			TimeString += "00:00";

			DateTimeNow = DateTime(String(TimeString.c_str()));
		}

		return DateTimeNow;
	}

	/// @brief Gets the time zone, represented as 1 is UTC+1, 2 is UTC+2 etc.
	/// @return int representing timezone offset
	static int GetTimeZone()
	{
		time_t Now = time(0);
		struct tm* Lctm;
		Lctm = localtime(&Now);
		struct tm* Gmtm;
		Gmtm = gmtime(&Now);

		return Lctm->tm_hour - Gmtm->tm_hour;
	}

	/// @brief Checks if this DateTime represents the epoch time (0 seconds since epoch).
	/// @return bool
	bool IsEpoch() const
	{
		return TimePoint.time_since_epoch().count() == 0;
	}

	/// @brief Checks if this DateTime is more recent than the Other DateTime.
	/// @param const DateTime& Other : DateTime to check against
	/// @return bool : Returns true if current DateTime is greater or equal than given DateTime
	bool operator>=(const DateTime& Other) const
	{
		return TimePoint >= Other.TimePoint;
	}

	/// @brief Gets the time_point from the DateTime.
	/// @return Clock::time_point&
	const Clock::time_point& GetTimePoint() const;

private:
	Clock::time_point TimePoint;
};

} // namespace csp::common

PRAGMA_WARNING_POP()
