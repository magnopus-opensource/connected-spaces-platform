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

    const auto timeNow = csp::common::DateTime::UtcTimeNow().GetTimePoint();
    return (csp::common::DateTime(StartDateTimestamp).GetTimePoint() <= timeNow && csp::common::DateTime(EndDateTimestamp).GetTimePoint() >= timeNow);
}

void MaintenanceInfoResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        rapidjson::Document jsonDoc;
        rapidjson::ParseResult ok
            = csp::json::ParseWithErrorLogging(jsonDoc, apiResponse->GetResponse()->GetPayload().GetContent(), "MaintenanceInfoResult::OnResponse");
        if (!ok)
        {
            return;
        }

        assert(jsonDoc.IsArray());
        auto localArray = csp::common::Array<MaintenanceInfo>(jsonDoc.Size());

        const auto timeNow = csp::common::DateTime::UtcTimeNow().GetTimePoint();
        uint32_t arrayShrinkage = 0;

        for (rapidjson::SizeType i = 0; i < jsonDoc.Size(); i++)
        {
            // remove old maintenance windows
            if (timeNow <= csp::common::DateTime(jsonDoc[i]["End"].GetString()).GetTimePoint())
            {
                localArray[i - arrayShrinkage].Description = jsonDoc[i]["Description"].GetString();
                localArray[i - arrayShrinkage].StartDateTimestamp = jsonDoc[i]["Start"].GetString();
                localArray[i - arrayShrinkage].EndDateTimestamp = jsonDoc[i]["End"].GetString();
            }
            else
            {
                arrayShrinkage++;
            };
        }

        m_maintenanceInfoResponses = csp::common::Array<MaintenanceInfo>(jsonDoc.Size() - arrayShrinkage);

        for (uint32_t i = 0; i < (jsonDoc.Size() - arrayShrinkage); i++)
        {
            m_maintenanceInfoResponses[i].Description = localArray[i].Description;
            m_maintenanceInfoResponses[i].StartDateTimestamp = localArray[i].StartDateTimestamp;
            m_maintenanceInfoResponses[i].EndDateTimestamp = localArray[i].EndDateTimestamp;
        }

        if (m_maintenanceInfoResponses.Size() == 0)
        {
            CSP_LOG_MSG(common::LogLevel::Verbose, "No future maintenance windows are defined by the services");
        }

        // Sort maintenance windows by latest date
        SortMaintenanceInfos(m_maintenanceInfoResponses);
    }
}

csp::common::Array<MaintenanceInfo>& MaintenanceInfoResult::GetMaintenanceInfoResponses() { return m_maintenanceInfoResponses; }

const csp::common::Array<MaintenanceInfo>& MaintenanceInfoResult::GetMaintenanceInfoResponses() const { return m_maintenanceInfoResponses; }

const MaintenanceInfo& MaintenanceInfoResult::GetLatestMaintenanceInfo() const
{
    if (HasAnyMaintenanceWindows())
    {
        return m_maintenanceInfoResponses[0];
    }
    else
    {
        CSP_LOG_MSG(common::LogLevel::Verbose, "Default maintenance window info is being returned as the latest window.");
        return GetDefaultMaintenanceInfo();
    }
}

bool MaintenanceInfoResult::HasAnyMaintenanceWindows() const { return m_maintenanceInfoResponses.Size() > 0; }

const MaintenanceInfo& MaintenanceInfoResult::GetDefaultMaintenanceInfo() const
{
    static MaintenanceInfo defaultMaintenanceInfo;
    return defaultMaintenanceInfo;
}

void SortMaintenanceInfos(csp::common::Array<MaintenanceInfo>& maintenanceInfos)
{
    const csp::common::DateTime currentTime(csp::common::DateTime::UtcTimeNow());

    std::sort(maintenanceInfos.begin(), maintenanceInfos.end(),
        [currentTime](const MaintenanceInfo& item1, const MaintenanceInfo& item2)
        {
            const csp::common::DateTime item1Time(item1.EndDateTimestamp);
            const csp::common::DateTime item2Time(item2.EndDateTimestamp);

            return item1Time.GetTimePoint() - currentTime.GetTimePoint() < item2Time.GetTimePoint() - currentTime.GetTimePoint();
        });
}

} // namespace csp::systems
