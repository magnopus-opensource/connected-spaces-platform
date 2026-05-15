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
#include "CSP/Common/fmt_Formatters.h"
#include "CSP/Systems/ServiceStatus.h"
#include "CSP/Systems/SystemsManager.h"
#include "RAIIMockLogger.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <fmt/format.h>
#include <gmock/gmock.h>

using namespace csp::common;

namespace
{

csp::systems::VersionMetadata CreateVersionMetadata(const String& version, const String& deprecationDatetime)
{
    auto versionMetadata = csp::systems::VersionMetadata();

    versionMetadata.Version = version;
    versionMetadata.DeprecationDatetime = deprecationDatetime;

    return versionMetadata;
}

csp::systems::ServiceStatus CreateServiceStatus(
    const String& reverseProxy, const String& name, const Array<csp::systems::VersionMetadata> apiVersions, const String& currentApiVersion)
{
    auto serviceStatus = csp::systems::ServiceStatus();

    serviceStatus.ReverseProxy = reverseProxy;
    serviceStatus.Name = name;
    serviceStatus.ApiVersions = apiVersions;
    serviceStatus.CurrentApiVersion = currentApiVersion;

    return serviceStatus;
}

csp::systems::ServicesDeploymentStatus CreateServicesDeploymentStatus(const String& version, const Array<csp::systems::ServiceStatus> services)
{
    auto servicesDeploymentStatus = csp::systems::ServicesDeploymentStatus();

    servicesDeploymentStatus.Version = version;
    servicesDeploymentStatus.Services = services;

    return servicesDeploymentStatus;
}
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveServiceDefinitionWithValidDataTest)
{
    auto const endpoints = csp::CSPFoundation::GetEndpoints();

    // Create a dummy response for ServicesDeploymentStatus containing the user service
    auto versionMetadata = CreateVersionMetadata(fmt::format("v{0}", endpoints.UserService.GetVersion()).c_str(), "");
    auto userServiceServiceStatus
        = CreateServiceStatus("mag-user", "User Service", { versionMetadata }, fmt::format("v{0}", endpoints.UserService.GetVersion()).c_str());
    auto servicesDeploymentStatus = CreateServicesDeploymentStatus("2.0.1-{GUID}", { userServiceServiceStatus });

    auto const result = endpoints.UserService.CheckPrerequisites(servicesDeploymentStatus);
    EXPECT_TRUE(result);
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveServiceDefinitionWithInvalidDataTest)
{
    RAIIMockLogger mockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Error);

    auto const endpoints = csp::CSPFoundation::GetEndpoints();

    // Validate that the retired code path has been triggered and populated through the log system
    const String errorMsg = fmt::format("Unable to resolve {} in Status Info", endpoints.UserService.GetURI()).c_str();
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, errorMsg)).Times(1);

    // Create a dummy response for ServicesDeploymentStatus containing the invalid information
    auto servicesDeploymentStatus = CreateServicesDeploymentStatus("", {});

    auto const result = endpoints.UserService.CheckPrerequisites(servicesDeploymentStatus);
    EXPECT_FALSE(result);
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveServiceDefinitionWithDeprecatedDataTest)
{
    RAIIMockLogger mockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Fatal);

    auto const endpoints = csp::CSPFoundation::GetEndpoints();

    // Create a dummy response for ServicesDeploymentStatus containing the user service
    auto versionMetadata = CreateVersionMetadata("v2", "");
    auto userServiceServiceStatus = CreateServiceStatus("mag-user", "User Service", { versionMetadata }, "v2");
    auto servicesDeploymentStatus = CreateServicesDeploymentStatus("2.0.1-{GUID}", { userServiceServiceStatus });

    // Validate that the retired code path has been triggered and populated through the log system
    const String errorMsg = "User Service v1 has been retired, the latest version is v2. For more information please visit: "
                            "https://connected-spaces-platform.net/index.html";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Fatal, errorMsg)).Times(1);

    auto const result = endpoints.UserService.CheckPrerequisites(servicesDeploymentStatus);
    EXPECT_FALSE(result);
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveServiceDefinitionWithRetiredDataTest)
{
    RAIIMockLogger mockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Warning);

    auto const endpoints = csp::CSPFoundation::GetEndpoints();

    // Create a dummy response for ServicesDeploymentStatus containing the user service
    auto versionMetadata = CreateVersionMetadata(fmt::format("v{0}", endpoints.UserService.GetVersion()).c_str(), "YYYY-MM-DDThh:mm:ss.sTZD");
    auto userServiceServiceStatus
        = CreateServiceStatus("mag-user", "User Service", { versionMetadata }, fmt::format("v{0}", endpoints.UserService.GetVersion()).c_str());
    auto servicesDeploymentStatus = CreateServicesDeploymentStatus("2.0.1-{GUID}", { userServiceServiceStatus });

    // Validate that the deprecated code path has been triggered and populated through the log system
    const String errorMsg = "User Service v1 will be deprecated as of YYYY-MM-DDThh:mm:ss.sTZD, the latest version is v1. For more information "
                            "please visit: https://connected-spaces-platform.net/index.html";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, errorMsg)).Times(1);

    auto const result = endpoints.UserService.CheckPrerequisites(servicesDeploymentStatus);
    EXPECT_TRUE(result);
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveServiceDefinitionWithNewVersionAvailableDataTest)
{
    RAIIMockLogger mockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

    auto const endpoints = csp::CSPFoundation::GetEndpoints();

    // Create a dummy response for ServicesDeploymentStatus containing the user service
    auto versionMetadata = CreateVersionMetadata(fmt::format("v{0}", endpoints.UserService.GetVersion()).c_str(), "");
    auto userServiceServiceStatus = CreateServiceStatus("mag-user", "User Service", { versionMetadata }, "v{Infinity}");
    auto servicesDeploymentStatus = CreateServicesDeploymentStatus("2.0.1-{GUID}", { userServiceServiceStatus });

    // Validate that the latest available version code path has been triggered and populated through the log system
    const String errorMsg = "User Service v1 is not the latest available, the latest version is v{Infinity}. For more information please visit: "
                            "https://connected-spaces-platform.net/index.html";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, errorMsg)).Times(1);

    auto const result = endpoints.UserService.CheckPrerequisites(servicesDeploymentStatus);
    EXPECT_TRUE(result);
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveMultiplayerHubMethodWithValidDataTest)
{
    auto const multiplayerHubMethodMap = csp::multiplayer::MultiplayerHubMethodMap();
    auto methods = Array<String>(multiplayerHubMethodMap.size());

    // Construct a csp::common::Array of all available multiplayer hub methods
    for (auto const method : multiplayerHubMethodMap)
        methods[static_cast<int>(method.first)] = String(method.second.c_str());

    auto const result = multiplayerHubMethodMap.CheckPrerequisites(methods);
    EXPECT_TRUE(result);
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveMultiplayerHubMethodWithInvalidDataTest)
{
    RAIIMockLogger mockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Fatal);

    // Validate that the failure to find the multiplayer hub methods code path has been triggered and populated through the log system
    const String errorMsg = "Failed to resolve the Multiplayer Hub Method: DeleteObjects";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Fatal, errorMsg)).Times(1);

    auto const multiplayerHubMethodMap = csp::multiplayer::MultiplayerHubMethodMap();
    auto const methods = Array<String> {};
    auto const result = multiplayerHubMethodMap.CheckPrerequisites(methods);
    EXPECT_FALSE(result);
}

CSP_PUBLIC_TEST(CSPEngine, MCSExternalDependencyTests, ResolveMultiplayerHubMethodWithIncompleteDataTest)
{
    RAIIMockLogger mockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Fatal);

    // Validate that the failure to find the multiplayer hub methods code path has been triggered and populated through the log system
    const String errorMsg = "Failed to resolve the Multiplayer Hub Method: GenerateObjectIds";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Fatal, errorMsg)).Times(1);

    auto const multiplayerHubMethodMap = csp::multiplayer::MultiplayerHubMethodMap();
    auto const methods = Array<String> { "DeleteObjects", "SendEventMessage", "StopListening" };
    auto const result = multiplayerHubMethodMap.CheckPrerequisites(methods);
    EXPECT_FALSE(result);
}
