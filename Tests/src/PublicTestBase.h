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

#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Common/SharedEnums.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/Settings/SettingsCollection.h"
#include "CSP/Systems/Spaces/Space.h"
#include "Mocks/SignalRConnectionMock.h"
#include <gtest/gtest.h>

namespace
{
constexpr char* EndpointEnvironmentName = "MAGNOPUS_SERVICES_ENDPOINT";
constexpr char* AdminAccountEmailName = "MAGNOPUS_SERVICES_ADMIN_EMAIL";
constexpr char* AdminAccountPasswordName = "MAGNOPUS_SERVICES_ADMIN_PASSWORD";

// If you're trying to run with localMCS, set `MAGNOPUS_SERVICES_ENDPOINT=http://localhost:8081` after having launched the maglocal docker instance.
const char* GetEnvironmentVariableOrDefault(const char* EnvironmentKey, const char* DefaultValue)
{
    const auto EnvironmentVariable = std::getenv(EnvironmentKey);
    return (EnvironmentVariable) ? EnvironmentVariable : DefaultValue;
}
} // namespace

inline const char* EndpointBaseURI() { return GetEnvironmentVariableOrDefault(EndpointEnvironmentName, "https://ogs-internal.magnopus-dev.cloud"); }

inline const char* AdminAccountEmail() { return GetEnvironmentVariableOrDefault(AdminAccountEmailName, ""); }

inline const char* AdminAccountPassword() { return GetEnvironmentVariableOrDefault(AdminAccountPasswordName, ""); }

class PublicTestBase : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;
};

class PublicTestBaseWithMocks : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    // We don't have to/can't clean this up here, we inject it and CSP takes ownership.
    // Confusing from an external user perspective I know, and somewhat fragile because we're relying on SystemsManager::destroy to trigger the RAII
    // behaviour, may change with a new initialisation api.
    SignalRConnectionMock* SignalRMock;
};

// For parameterized (data driven) tests
template <typename T> class PublicTestBaseWithParam : public ::testing::TestWithParam<T>
{
protected:
    void SetUp() override;
    void TearDown() override;
};

// If you want to use a new parameterized test structure, you need to explicitly instantiate here (and in the .cpp)
extern template class PublicTestBaseWithParam<std::tuple<csp::systems::SpaceAttributes, csp::systems::EResultCode, std::string>>;
extern template class PublicTestBaseWithParam<std::tuple<csp::common::RealtimeEngineType, bool, csp::systems::EResultCode, std::string>>;
extern template class PublicTestBaseWithParam<csp::common::RealtimeEngineType>;
extern template class PublicTestBaseWithParam<std::tuple<csp::common::RealtimeEngineType, bool>>;
extern template class PublicTestBaseWithParam<std::tuple<csp::systems::AvatarType, csp::common::String, bool>>;
extern template class PublicTestBaseWithParam<std::tuple<csp::systems::EResultCode, csp::web::EResponseCodes, csp::common::String, bool>>;
extern template class PublicTestBaseWithParam<std::tuple<csp::common::String, csp::systems::EAssetCollectionType, csp::common::String>>;