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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

csp::common::String CreateTimeString(system_clock::time_point tp)
{
    std::time_t now_c = std::chrono::system_clock::to_time_t(tp);
    std::tm* Gmtm = std::gmtime(&now_c);

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

        return csp::common::String(TimeString.c_str());
    }

    return "";

} // namespace

#if RUN_ALL_UNIT_TESTS || RUN_MAINTENANCESYSTEM_TESTS || RUN_MAINTENANCESYSTEM_GETMAINTENANCEINFO_TEST

CSP_PUBLIC_TEST(CSPEngine, MaintenanceSystemTests, GetMaintenanceInfoTest)
{
    auto& SystemsManager = SystemsManager::Get();
    auto* MaintenanceSystem = SystemsManager.GetMaintenanceSystem();

    auto [Result] = AWAIT(MaintenanceSystem, GetMaintenanceInfo, "https://maintenance-windows.magnopus-dev.cloud/maintenance-windows.json");
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MAINTENANCESYSTEM_TESTS || RUN_MAINTENANCESYSTEM_ISINSIDEMAINTENANCEWINDOW_TEST

CSP_PUBLIC_TEST(CSPEngine, MaintenanceSystemTests, IsInsideMaintenanceWindowInfoTest)
{
    auto& SystemsManager = SystemsManager::Get();
    auto* MaintenanceSystem = SystemsManager.GetMaintenanceSystem();

    auto [Result] = AWAIT(MaintenanceSystem, GetMaintenanceInfo, "https://maintenance-windows.magnopus-dev.cloud/maintenance-windows.json");

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const MaintenanceInfo& LatestMaintenanceInfo = Result.GetLatestMaintenanceInfo();

    EXPECT_FALSE(LatestMaintenanceInfo.IsInsideWindow());
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MAINTENANCESYSTEM_TESTS || RUN_MAINTENANCESYSTEM_GET_LATEST_MAINTENANCEWINDOW_TEST

CSP_PUBLIC_TEST(CSPEngine, MaintenanceSystemTests, GetLatestMaintenanceWindowInfoTest)
{
    auto& SystemsManager = SystemsManager::Get();
    auto* MaintenanceSystem = SystemsManager.GetMaintenanceSystem();

    auto [Result] = AWAIT(MaintenanceSystem, GetMaintenanceInfo, "https://maintenance-windows.magnopus-dev.cloud/maintenance-windows.json");
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const MaintenanceInfo& LatestMaintenanceInfo = Result.GetLatestMaintenanceInfo();
    if (Result.HasAnyMaintenanceWindows())
    {
        // if any windows were retrieved, then we should expect these fields to all be filled
        EXPECT_NE(LatestMaintenanceInfo.Description, "");
        EXPECT_NE(LatestMaintenanceInfo.StartDateTimestamp, "");
        EXPECT_NE(LatestMaintenanceInfo.EndDateTimestamp, "");
    }
    else
    {
        // if no windows were retrieved, we should expect to have gotten the default window back when asking for the latest one
        EXPECT_FALSE(LatestMaintenanceInfo.IsInsideWindow());
        EXPECT_EQ(LatestMaintenanceInfo.Description, Result.GetDefaultMaintenanceInfo().Description);
        EXPECT_EQ(LatestMaintenanceInfo.StartDateTimestamp, Result.GetDefaultMaintenanceInfo().StartDateTimestamp);
        EXPECT_EQ(LatestMaintenanceInfo.EndDateTimestamp, Result.GetDefaultMaintenanceInfo().EndDateTimestamp);
    }
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MAINTENANCESYSTEM_TESTS || RUN_MAINTENANCESYSTEM_SORTMAINTENANCEINFOS_TEST

CSP_PUBLIC_TEST(CSPEngine, MaintenanceSystemTests, SortMaintenanceInfosTest)
{
    csp::common::DateTime CurrentTime = csp::common::DateTime::UtcTimeNow();

    system_clock::time_point Info1Timepoint = CurrentTime.GetTimePoint() + system_clock::duration(120min);

    MaintenanceInfo Info1;
    Info1.Description = "Info1";
    Info1.EndDateTimestamp = CreateTimeString(Info1Timepoint);

    system_clock::time_point Info2Timepoint = CurrentTime.GetTimePoint() + system_clock::duration(60min);
    MaintenanceInfo Info2;
    Info2.Description = "Info2";
    Info2.EndDateTimestamp = CreateTimeString(Info2Timepoint);

    csp::common::Array<MaintenanceInfo> MaintenanceInfos { Info1, Info2 };

    SortMaintenanceInfos(MaintenanceInfos);

    EXPECT_EQ(MaintenanceInfos[0].Description, "Info2");

    csp::common::Array<MaintenanceInfo> MaintenanceInfos2 { Info2, Info1 };

    SortMaintenanceInfos(MaintenanceInfos2);

    EXPECT_EQ(MaintenanceInfos2[0].Description, "Info2");
}

#endif

} // namespace