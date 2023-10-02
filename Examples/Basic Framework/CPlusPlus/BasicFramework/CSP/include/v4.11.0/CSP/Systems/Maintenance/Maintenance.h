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

#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Services/WebService.h"


namespace csp::services
{

class ApiResponseBase;

} // namespace csp::services


namespace csp::systems
{

/// @brief Represents a single maintenance window, provides description of the event and a start and end timestamp
class CSP_API MaintenanceInfo
{
public:
	bool IsInsideWindow() const;

	csp::common::String Description;
	csp::common::String StartDateTimestamp;
	csp::common::String EndDateTimestamp;
};

/// @ingroup CSPFoundation
/// @brief Data class used to contain information when a Response is received from Maintenance Window Server
class CSP_API MaintenanceInfoResult : public csp::services::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	CSP_START_IGNORE
	friend class CSPFoundation;
	CSP_END_IGNORE
	/** @endcond */
public:
	MaintenanceInfoResult() {};
	MaintenanceInfoResult(void*) {};
	MaintenanceInfoResult(csp::services::EResultCode ResCode, uint16_t HttpResCode) : csp::services::ResultBase(ResCode, HttpResCode) {};

	/// @brief Retrieves response data from the Maintenance Window Server
	/// @return csp::common::Array<MaintenanceInfo> : return all maintenance information available in date order
	[[nodiscard]] csp::common::Array<MaintenanceInfo>& GetMaintenanceInfoResponses();

	/// @brief Retrieves response data from the Maintenance Window Server
	/// @return csp::common::Array<MaintenanceInfo> : return all maintenance information available in date order
	[[nodiscard]] const csp::common::Array<MaintenanceInfo>& GetMaintenanceInfoResponses() const;

	/// @brief Can be used to determine if any maintenance windows were defined by the services.
	/// @return bool : will return true when `GetMaintenanceInfoResponses` returns a zero-sized array.
	[[nodiscard]] bool HasAnyMaintenanceWindows() const;

	/// @brief Will return info for the future maintenance window closest to the current time, or default window info if none exist.
	/// @return MaintenanceInfo : the closest maintenance window information
	[[nodiscard]] const MaintenanceInfo& GetLatestMaintenanceInfo() const;

	/// @brief Represents a default maintenance window object, which is used when the platform finds no future maintenance windows.
	/// @return MaintenanceInfo : what the platform considers to be a default maintenance window
	[[nodiscard]] const MaintenanceInfo& GetDefaultMaintenanceInfo() const;

	/// @brief Returns an Invalid state MaintenanceInfoResult.
	CSP_NO_EXPORT static MaintenanceInfoResult Invalid();

private:
	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;
	csp::common::Array<MaintenanceInfo> MaintenanceInfoResponses;
};

typedef std::function<void(const MaintenanceInfoResult& Result)> MaintenanceInfoCallback;

void SortMaintenanceInfos(csp::common::Array<MaintenanceInfo>& MaintenanceInfos);

} // namespace csp::systems
