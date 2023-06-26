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
		MaintenanceInfoResponses = csp::common::Array<MaintenanceInfo>(JsonDoc.Size());
		for (rapidjson::SizeType i = 0; i < JsonDoc.Size(); i++)
		{
			MaintenanceInfoResponses[i].Description		   = JsonDoc[i]["Description"].GetString();
			MaintenanceInfoResponses[i].StartDateTimestamp = JsonDoc[i]["Start"].GetString();
			MaintenanceInfoResponses[i].EndDateTimestamp   = JsonDoc[i]["End"].GetString();
		}

		SortMaintenanceInfos(MaintenanceInfoResponses);
	}
}

const csp::common::Array<MaintenanceInfo>& MaintenanceInfoResult::GetMaintenanceInfoResponses()
{
	return MaintenanceInfoResponses;
}

const MaintenanceInfo& MaintenanceInfoResult::GetLatestMaintenanceInfo()
{
	return MaintenanceInfoResponses[0];
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
