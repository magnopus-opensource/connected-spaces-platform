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
	csp::common::String Description;
	csp::common::String StartDateTimestamp;
	csp::common::String EndDateTimestamp;
};

/// @brief Represents thee latest maintenance window, provides description of the event and a start and end timestamp and whether the user is
/// currently inside the window
class CSP_API InsideMaintenanceInfo : public MaintenanceInfo
{
public:
	explicit InsideMaintenanceInfo(void*) {};
	InsideMaintenanceInfo() = default;
	InsideMaintenanceInfo(bool InIsInsideMaintenanceWindow, const MaintenanceInfo& InsideMaintenanceData);
	bool IsInsideMaintenanceWindow;
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
	[[nodiscard]] const csp::common::Array<MaintenanceInfo>& GetMaintenanceInfoResponses();

	/// @brief Retrieves response data from the Maintenance Window Server
	/// @return MaintenanceInfo : return the closest maintenance information
	[[nodiscard]] const MaintenanceInfo& GetLatestMaintenanceInfo();

	/// @brief Returns an Invalid state MaintenanceInfoResult.
	CSP_NO_EXPORT static MaintenanceInfoResult Invalid();

private:
	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;
	csp::common::Array<MaintenanceInfo> MaintenanceInfoResponses;
};

/// @ingroup CSPFoundation
/// @brief Data class used to contain the latest maintenance window and a check for being inside the latest maintenance window received from
/// Maintenance Window Server
class CSP_API InsideMaintenanceInfoResult : public csp::services::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	CSP_START_IGNORE
	friend class CSPFoundation;
	friend class MaintenanceSystem;
	CSP_END_IGNORE
	/** @endcond */
public:
	InsideMaintenanceInfoResult() {};
	InsideMaintenanceInfoResult(void*) {};
	InsideMaintenanceInfoResult(csp::services::EResultCode ResCode, uint16_t HttpResCode) : csp::services::ResultBase(ResCode, HttpResCode) {};

	/// @brief Retrieves response data from the Maintenance Window Server
	/// @return InsideMaintenanceInfo : return check for being inside a maintenance window
	[[nodiscard]] InsideMaintenanceInfo& GetInsideMaintenanceInfo();

	/// @brief Returns an Invalid state InsideMaintenanceInfoResult.
	CSP_NO_EXPORT static InsideMaintenanceInfoResult Invalid();

private:
	void SetInsideMaintenanceInfo(bool InIsInsideWindow, const MaintenanceInfo& InMaintenanceInfo);
	InsideMaintenanceInfo InsideMaintenanceInfoResponse;
};

typedef std::function<void(MaintenanceInfoResult& Result)> MaintenanceInfoCallback;
typedef std::function<void(InsideMaintenanceInfoResult& Result)> InsideMaintenanceWindowCallback;

void SortMaintenanceInfos(csp::common::Array<MaintenanceInfo>& MaintenanceInfos);

} // namespace csp::systems
