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

#include "CSP/Systems/Maintenance/Maintenance.h"

#include "Common/Algorithm.h"
#include "Services/ApiBase/ApiBase.h"


namespace csp::systems
{

InsideMaintenanceInfo::InsideMaintenanceInfo(bool InIsInsideMaintenanceWindow, const MaintenanceInfo& InsideMaintenanceData)
{
	IsInsideMaintenanceWindow = InIsInsideMaintenanceWindow;
	Description				  = InsideMaintenanceData.Description;
	StartDateTimestamp		  = InsideMaintenanceData.StartDateTimestamp;
	EndDateTimestamp		  = InsideMaintenanceData.EndDateTimestamp;
}

MaintenanceInfoResult MaintenanceInfoResult::Invalid()
{
	static MaintenanceInfoResult result(csp::services::EResultCode::Failed, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseBadRequest));
	return result;
}

void MaintenanceInfoResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		rapidjson::Document JsonDoc;

		JsonDoc.Parse(ApiResponse->GetResponse()->GetPayload().GetContent());
		assert(JsonDoc.IsArray());
		auto LocalArray = csp::common::Array<MaintenanceInfo>(JsonDoc.Size());

		const auto TimeNow		= csp::common::DateTime::UtcTimeNow().GetTimePoint();
		uint32_t ArrayShrinkage = 0;

		for (rapidjson::SizeType i = 0; i < JsonDoc.Size(); i++)
		{
			// remove old maintenance windows
			if (TimeNow <= csp::common::DateTime(JsonDoc[i]["End"].GetString()).GetTimePoint())
			{
				LocalArray[i - ArrayShrinkage].Description		  = JsonDoc[i]["Description"].GetString();
				LocalArray[i - ArrayShrinkage].StartDateTimestamp = JsonDoc[i]["Start"].GetString();
				LocalArray[i - ArrayShrinkage].EndDateTimestamp	  = JsonDoc[i]["End"].GetString();
			}
			else
			{
				ArrayShrinkage++;
			};
		}

		MaintenanceInfoResponses = csp::common::Array<MaintenanceInfo>(JsonDoc.Size() - ArrayShrinkage);

		for (uint32_t i = 0; i < (JsonDoc.Size() - ArrayShrinkage); i++)
		{
			MaintenanceInfoResponses[i].Description		   = LocalArray[i].Description;
			MaintenanceInfoResponses[i].StartDateTimestamp = LocalArray[i].StartDateTimestamp;
			MaintenanceInfoResponses[i].EndDateTimestamp   = LocalArray[i].EndDateTimestamp;
		}

		if (MaintenanceInfoResponses.Size() == 0)
		{
			FOUNDATION_LOG_WARN_MSG("No maintenance windows are defined by the services");
		}

		// Sort maintenance windows by latest date
		SortMaintenanceInfos(MaintenanceInfoResponses);
	}
}

csp::common::Array<MaintenanceInfo>& MaintenanceInfoResult::GetMaintenanceInfoResponses()
{
	return MaintenanceInfoResponses;
}

const csp::common::Array<MaintenanceInfo>& MaintenanceInfoResult::GetMaintenanceInfoResponses() const
{
	return MaintenanceInfoResponses;
}

const MaintenanceInfo& MaintenanceInfoResult::GetLatestMaintenanceInfo() const
{
	if (HasAnyMaintenanceWindows())
	{
		return MaintenanceInfoResponses[0];
	}
	else
	{
		FOUNDATION_LOG_WARN_MSG("When asked to get the latest maintenance window info, there were none defined. A default maintenance window info is "
								"being returned instead.");
		return GetDefaultMaintenanceInfo();
	}
}

bool MaintenanceInfoResult::HasAnyMaintenanceWindows() const
{
	return MaintenanceInfoResponses.Size() > 0;
}

const MaintenanceInfo& MaintenanceInfoResult::GetDefaultMaintenanceInfo()
{
	static MaintenanceInfo DefaultMaintenanceInfo;
	return DefaultMaintenanceInfo;
}

void SortMaintenanceInfos(csp::common::Array<MaintenanceInfo>& MaintenanceInfos)
{
	const csp::common::DateTime CurrentTime(csp::common::DateTime::UtcTimeNow());

	csp::common::Sort(MaintenanceInfos,
					  [CurrentTime](const MaintenanceInfo& Item1, const MaintenanceInfo& Item2)
					  {
						  const csp::common::DateTime Item1Time(Item1.EndDateTimestamp);
						  const csp::common::DateTime Item2Time(Item2.EndDateTimestamp);

						  return Item1Time.GetTimePoint() - CurrentTime.GetTimePoint() < Item2Time.GetTimePoint() - CurrentTime.GetTimePoint();
					  });
}

InsideMaintenanceInfo& InsideMaintenanceInfoResult::GetInsideMaintenanceInfo()
{
	return InsideMaintenanceInfoResponse;
}

const InsideMaintenanceInfo& InsideMaintenanceInfoResult::GetInsideMaintenanceInfo() const
{
	return InsideMaintenanceInfoResponse;
}

void InsideMaintenanceInfoResult::SetInsideMaintenanceInfo(bool InIsInsideWindow, const MaintenanceInfo& InMaintenanceInfo)
{
	InsideMaintenanceInfoResponse = InsideMaintenanceInfo(InIsInsideWindow, InMaintenanceInfo);
}

InsideMaintenanceInfoResult InsideMaintenanceInfoResult::Invalid()
{
	static InsideMaintenanceInfoResult result(csp::services::EResultCode::Failed,
											  static_cast<uint16_t>(csp::web::EResponseCodes::ResponseBadRequest));
	return result;
}

} // namespace csp::systems
