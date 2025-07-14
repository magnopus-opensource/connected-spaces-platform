/*
 * Copyright 2025 Magnopus LLC

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
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Systems/Status/Status.h"
#include "CSP/Systems/SystemsManager.h"
#include "RAIIMockLogger.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <fmt/format.h>
#include <gmock/gmock.h>

using namespace csp::common;

namespace
{

csp::systems::ServiceVersionInfo CreateServiceVersionInfo(const String& Version, const String& DeprecationDatetime)
{
    auto ServiceVersionInfo = csp::systems::ServiceVersionInfo();

    ServiceVersionInfo.Version = Version;
    ServiceVersionInfo.DeprecationDatetime = DeprecationDatetime;

    return ServiceVersionInfo;
}

csp::systems::ServiceInfo CreateServiceInfo(
    const String& ReverseProxy, const String& Name, const Array<csp::systems::ServiceVersionInfo> ApiVersions, const String& CurrentApiVersion)
{
    auto ServiceInfo = csp::systems::ServiceInfo();

    ServiceInfo.ReverseProxy = ReverseProxy;
    ServiceInfo.Name = Name;
    ServiceInfo.ApiVersions = ApiVersions;
    ServiceInfo.CurrentApiVersion = CurrentApiVersion;

    return ServiceInfo;
}

csp::systems::StatusInfo CreateStatusInfo(const String& ContainerVersion, const Array<csp::systems::ServiceInfo> Services)
{
    auto StatusInfo = csp::systems::StatusInfo();

    StatusInfo.ContainerVersion = ContainerVersion;
    StatusInfo.Services = Services;

    return StatusInfo;
}
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveServiceDefinitionWithValidDataTest)
{
    auto const Endpoints = csp::CSPFoundation::GetEndpoints();

    // Create a dummy response for StatusInfo containing the user service
    auto ServiceVersionInfo = CreateServiceVersionInfo(fmt::format("v{0}", Endpoints.UserService.GetVersion()).c_str(), "");
    auto UserServiceServiceInfo
        = CreateServiceInfo("mag-user", "User Service", { ServiceVersionInfo }, fmt::format("v{0}", Endpoints.UserService.GetVersion()).c_str());
    auto StatusInfo = CreateStatusInfo("2.0.1-{GUID}", { UserServiceServiceInfo });

    auto const result = Endpoints.UserService.CheckPrerequisites(StatusInfo);
    EXPECT_TRUE(result);
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveServiceDefinitionWithInvalidDataTest)
{
    RAIIMockLogger MockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Error);

    auto const Endpoints = csp::CSPFoundation::GetEndpoints();

    // Validate that the retired code path has been triggered and populated through the log system
    const String ErrorMsg = "Unable to resolve mag-user Reverse Proxy in Status Info";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(ErrorMsg)).Times(1);

    // Create a dummy response for StatusInfo containing the invalid information
    auto StatusInfo = CreateStatusInfo("", {});

    auto const result = Endpoints.UserService.CheckPrerequisites(StatusInfo);
    EXPECT_FALSE(result);
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveServiceDefinitionWithDeprecatedDataTest)
{
    RAIIMockLogger MockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Fatal);

    auto const Endpoints = csp::CSPFoundation::GetEndpoints();

    // Create a dummy response for StatusInfo containing the user service
    auto ServiceVersionInfo = CreateServiceVersionInfo("v2", "");
    auto UserServiceServiceInfo = CreateServiceInfo("mag-user", "User Service", { ServiceVersionInfo }, "v2");
    auto StatusInfo = CreateStatusInfo("2.0.1-{GUID}", { UserServiceServiceInfo });

    // Validate that the retired code path has been triggered and populated through the log system
    const String ErrorMsg = "User Service v1 has been retired, the latest version is v2. For more information please visit: "
                            "https://connected-spaces-platform.net/index.html";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(ErrorMsg)).Times(1);

    auto const result = Endpoints.UserService.CheckPrerequisites(StatusInfo);
    EXPECT_FALSE(result);
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveServiceDefinitionWithRetiredDataTest)
{
    RAIIMockLogger MockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Warning);

    auto const Endpoints = csp::CSPFoundation::GetEndpoints();

    // Create a dummy response for StatusInfo containing the user service
    auto ServiceVersionInfo = CreateServiceVersionInfo(fmt::format("v{0}", Endpoints.UserService.GetVersion()).c_str(), "YYYY-MM-DDThh:mm:ss.sTZD");
    auto UserServiceServiceInfo
        = CreateServiceInfo("mag-user", "User Service", { ServiceVersionInfo }, fmt::format("v{0}", Endpoints.UserService.GetVersion()).c_str());
    auto StatusInfo = CreateStatusInfo("2.0.1-{GUID}", { UserServiceServiceInfo });

    // Validate that the deprecated code path has been triggered and populated through the log system
    const String ErrorMsg = "User Service v1 will be deprecated as of YYYY-MM-DDThh:mm:ss.sTZD, the latest version is v1. For more information "
                            "please visit: https://connected-spaces-platform.net/index.html";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(ErrorMsg)).Times(1);

    auto const result = Endpoints.UserService.CheckPrerequisites(StatusInfo);
    EXPECT_TRUE(result);
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveServiceDefinitionWithNewVersionAvailableDataTest)
{
    RAIIMockLogger MockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

    auto const Endpoints = csp::CSPFoundation::GetEndpoints();

    // Create a dummy response for StatusInfo containing the user service
    auto ServiceVersionInfo = CreateServiceVersionInfo(fmt::format("v{0}", Endpoints.UserService.GetVersion()).c_str(), "");
    auto UserServiceServiceInfo = CreateServiceInfo("mag-user", "User Service", { ServiceVersionInfo }, "v{Infinity}");
    auto StatusInfo = CreateStatusInfo("2.0.1-{GUID}", { UserServiceServiceInfo });

    // Validate that the latest available version code path has been triggered and populated through the log system
    const String ErrorMsg = "User Service v1 is not the latest available, the latest version is v{Infinity}. For more information please visit: "
                            "https://connected-spaces-platform.net/index.html";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(ErrorMsg)).Times(1);

    auto const result = Endpoints.UserService.CheckPrerequisites(StatusInfo);
    EXPECT_TRUE(result);
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveMultiplayerHubMethodWithValidDataTest)
{
    auto const MultiplayerHubMethodMap = csp::multiplayer::MultiplayerHubMethodMap();
    auto Methods = Array<String>(MultiplayerHubMethodMap.size());

    // Construct a csp::common::Array of all available multiplayer hub methods
    for (auto const Method : MultiplayerHubMethodMap)
        Methods[static_cast<int>(Method.first)] = String(Method.second.c_str());

    auto const result = csp::multiplayer::MultiplayerConnection::ResolveMultiplayerHubMethods(Methods);
    EXPECT_TRUE(result);
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveMultiplayerHubMethodWithInvalidDataTest)
{
    RAIIMockLogger MockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Fatal);

    // Validate that the failure to find multiplayer hub methods  code path has been triggered and populated through the log system
    const String ErrorMsg = "Failed to resolve the Multiplayer Hub Method: DeleteObjects";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(ErrorMsg)).Times(1);

    auto const Methods = Array<String> {};
    auto const result = csp::multiplayer::MultiplayerConnection::ResolveMultiplayerHubMethods(Methods);
    EXPECT_FALSE(result);
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveMultiplayerHubMethodWithIncompleteDataTest)
{
    RAIIMockLogger MockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Fatal);

    // Validate that the failure to find multiplayer hub methods  code path has been triggered and populated through the log system
    const String ErrorMsg = "Failed to resolve the Multiplayer Hub Method: GenerateObjectIds";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(ErrorMsg)).Times(1);

    auto const Methods = Array<String> { "DeleteObjects", "SendEventMessage", "StopListening" };
    auto const result = csp::multiplayer::MultiplayerConnection::ResolveMultiplayerHubMethods(Methods);
    EXPECT_FALSE(result);
}
