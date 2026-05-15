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

#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Systems/Maintenance/MaintenanceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Common/DateTime.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include <chrono>

using namespace std::chrono;
using namespace std::chrono_literals;

#include "gtest/gtest.h"

using namespace csp::systems;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

csp::common::String CreateTimeString(system_clock::time_point tp)
{
    std::time_t nowC = std::chrono::system_clock::to_time_t(tp);
    std::tm* gmtm = std::gmtime(&nowC);

    if (gmtm != nullptr)
    {
        std::string timeString;
        timeString = std::to_string(1900 + gmtm->tm_year);
        timeString += "-";
        timeString += std::to_string(gmtm->tm_mon + 1);
        timeString += "-";
        timeString += std::to_string(gmtm->tm_mday);
        timeString += "T";
        timeString += std::to_string(gmtm->tm_hour);
        timeString += ":";
        timeString += std::to_string(gmtm->tm_min);
        timeString += ":";
        timeString += std::to_string(gmtm->tm_sec);
        timeString += ".";
        timeString += "0+";
        timeString += "00:00";

        return csp::common::String(timeString.c_str());
    }

    return "";

} // namespace

CSP_PUBLIC_TEST(CSPEngine, MaintenanceSystemTests, GetMaintenanceInfoTest)
{
    auto& systemsManager = SystemsManager::Get();
    auto* maintenanceSystem = systemsManager.GetMaintenanceSystem();

    auto [Result] = AWAIT(maintenanceSystem, GetMaintenanceInfo, "https://maintenance-windows.magnopus-dev.cloud/maintenance-windows.json");
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
}

CSP_PUBLIC_TEST(CSPEngine, MaintenanceSystemTests, IsInsideMaintenanceWindowInfoTest)
{
    auto& systemsManager = SystemsManager::Get();
    auto* maintenanceSystem = systemsManager.GetMaintenanceSystem();

    auto [Result] = AWAIT(maintenanceSystem, GetMaintenanceInfo, "https://maintenance-windows.magnopus-dev.cloud/maintenance-windows.json");

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const MaintenanceInfo& latestMaintenanceInfo = Result.GetLatestMaintenanceInfo();

    EXPECT_FALSE(latestMaintenanceInfo.IsInsideWindow());
}

CSP_PUBLIC_TEST(CSPEngine, MaintenanceSystemTests, GetLatestMaintenanceWindowInfoTest)
{
    auto& systemsManager = SystemsManager::Get();
    auto* maintenanceSystem = systemsManager.GetMaintenanceSystem();

    auto [Result] = AWAIT(maintenanceSystem, GetMaintenanceInfo, "https://maintenance-windows.magnopus-dev.cloud/maintenance-windows.json");
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const MaintenanceInfo& latestMaintenanceInfo = Result.GetLatestMaintenanceInfo();
    if (Result.HasAnyMaintenanceWindows())
    {
        // if any windows were retrieved, then we should expect these fields to all be filled
        EXPECT_NE(latestMaintenanceInfo.Description, "");
        EXPECT_NE(latestMaintenanceInfo.StartDateTimestamp, "");
        EXPECT_NE(latestMaintenanceInfo.EndDateTimestamp, "");
    }
    else
    {
        // if no windows were retrieved, we should expect to have gotten the default window back when asking for the latest one
        EXPECT_FALSE(latestMaintenanceInfo.IsInsideWindow());
        EXPECT_EQ(latestMaintenanceInfo.Description, Result.GetDefaultMaintenanceInfo().Description);
        EXPECT_EQ(latestMaintenanceInfo.StartDateTimestamp, Result.GetDefaultMaintenanceInfo().StartDateTimestamp);
        EXPECT_EQ(latestMaintenanceInfo.EndDateTimestamp, Result.GetDefaultMaintenanceInfo().EndDateTimestamp);
    }
}

CSP_PUBLIC_TEST(CSPEngine, MaintenanceSystemTests, SortMaintenanceInfosTest)
{
    csp::common::DateTime currentTime = csp::common::DateTime::UtcTimeNow();

    system_clock::time_point info1Timepoint = currentTime.GetTimePoint() + system_clock::duration(120min);

    MaintenanceInfo info1;
    info1.Description = "Info1";
    info1.EndDateTimestamp = CreateTimeString(info1Timepoint);

    system_clock::time_point info2Timepoint = currentTime.GetTimePoint() + system_clock::duration(60min);
    MaintenanceInfo info2;
    info2.Description = "Info2";
    info2.EndDateTimestamp = CreateTimeString(info2Timepoint);

    csp::common::Array<MaintenanceInfo> maintenanceInfos { info1, info2 };

    SortMaintenanceInfos(maintenanceInfos);

    EXPECT_EQ(maintenanceInfos[0].Description, "Info2");

    csp::common::Array<MaintenanceInfo> maintenanceInfos2 { info2, info1 };

    SortMaintenanceInfos(maintenanceInfos2);

    EXPECT_EQ(maintenanceInfos2[0].Description, "Info2");
}