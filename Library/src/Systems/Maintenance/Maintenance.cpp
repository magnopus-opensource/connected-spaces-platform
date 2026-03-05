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
#include "Services/ApiBase/ApiBase.h"
#include "Json/JsonParseHelper.h"

#include <algorithm>

namespace csp::systems
{

bool MaintenanceInfo::IsInsideWindow() const
{
    if (StartDateTimestamp.IsEmpty() || EndDateTimestamp.IsEmpty())
    {
        return false;
    }

    const auto TimeNow = csp::common::DateTime::UtcTimeNow().GetTimePoint();
    return (csp::common::DateTime(StartDateTimestamp).GetTimePoint() <= TimeNow && csp::common::DateTime(EndDateTimestamp).GetTimePoint() >= TimeNow);
}

void MaintenanceInfoResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        rapidjson::Document JsonDoc;
        rapidjson::ParseResult ok
            = csp::json::ParseWithErrorLogging(JsonDoc, ApiResponse->GetResponse()->GetPayload().GetContent(), "MaintenanceInfoResult::OnResponse");
        if (!ok)
        {
            return;
        }

        assert(JsonDoc.IsArray());
        auto LocalArray = csp::common::Array<MaintenanceInfo>(JsonDoc.Size());

        const auto TimeNow = csp::common::DateTime::UtcTimeNow().GetTimePoint();
        uint32_t ArrayShrinkage = 0;

        for (rapidjson::SizeType i = 0; i < JsonDoc.Size(); i++)
        {
            // remove old maintenance windows
            if (TimeNow <= csp::common::DateTime(JsonDoc[i]["End"].GetString()).GetTimePoint())
            {
                LocalArray[i - ArrayShrinkage].Description = JsonDoc[i]["Description"].GetString();
                LocalArray[i - ArrayShrinkage].StartDateTimestamp = JsonDoc[i]["Start"].GetString();
                LocalArray[i - ArrayShrinkage].EndDateTimestamp = JsonDoc[i]["End"].GetString();
            }
            else
            {
                ArrayShrinkage++;
            };
        }

        MaintenanceInfoResponses = csp::common::Array<MaintenanceInfo>(JsonDoc.Size() - ArrayShrinkage);

        for (uint32_t i = 0; i < (JsonDoc.Size() - ArrayShrinkage); i++)
        {
            MaintenanceInfoResponses[i].Description = LocalArray[i].Description;
            MaintenanceInfoResponses[i].StartDateTimestamp = LocalArray[i].StartDateTimestamp;
            MaintenanceInfoResponses[i].EndDateTimestamp = LocalArray[i].EndDateTimestamp;
        }

        if (MaintenanceInfoResponses.Size() == 0)
        {
            CSP_LOG_MSG(common::LogLevel::Verbose, "No future maintenance windows are defined by the services");
        }

        // Sort maintenance windows by latest date
        SortMaintenanceInfos(MaintenanceInfoResponses);
    }
}

csp::common::Array<MaintenanceInfo>& MaintenanceInfoResult::GetMaintenanceInfoResponses() { return MaintenanceInfoResponses; }

const csp::common::Array<MaintenanceInfo>& MaintenanceInfoResult::GetMaintenanceInfoResponses() const { return MaintenanceInfoResponses; }

const MaintenanceInfo& MaintenanceInfoResult::GetLatestMaintenanceInfo() const
{
    if (HasAnyMaintenanceWindows())
    {
        return MaintenanceInfoResponses[0];
    }
    else
    {
        CSP_LOG_MSG(common::LogLevel::Verbose, "Default maintenance window info is being returned as the latest window.");
        return GetDefaultMaintenanceInfo();
    }
}

bool MaintenanceInfoResult::HasAnyMaintenanceWindows() const { return MaintenanceInfoResponses.Size() > 0; }

const MaintenanceInfo& MaintenanceInfoResult::GetDefaultMaintenanceInfo() const
{
    static MaintenanceInfo DefaultMaintenanceInfo;
    return DefaultMaintenanceInfo;
}

void SortMaintenanceInfos(csp::common::Array<MaintenanceInfo>& MaintenanceInfos)
{
    const csp::common::DateTime CurrentTime(csp::common::DateTime::UtcTimeNow());

    std::sort(MaintenanceInfos.begin(), MaintenanceInfos.end(),
        [CurrentTime](const MaintenanceInfo& Item1, const MaintenanceInfo& Item2)
        {
            const csp::common::DateTime Item1Time(Item1.EndDateTimestamp);
            const csp::common::DateTime Item2Time(Item2.EndDateTimestamp);

            return Item1Time.GetTimePoint() - CurrentTime.GetTimePoint() < Item2Time.GetTimePoint() - CurrentTime.GetTimePoint();
        });
}

} // namespace csp::systems
