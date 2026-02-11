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

#pragma once

#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/WebService.h"

namespace csp::services
{

class ApiResponseBase;

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

/// @ingroup Status System
/// @brief Stores the version and deprecation information for a service
class CSP_API VersionMetadata
{
public:
    /// @brief The version identifier of the service's API (e.g., "v1").
    csp::common::String Version;

    /// @brief The date and time at which the API version is considered deprecated, in ISO 8601 format.
    /// If empty, the version is currently active, or the deprecation date is unknown.
    csp::common::String DeprecationDatetime;

    bool operator==(const VersionMetadata& Other) const;
};

/// @ingroup Status System
/// @brief Stores information about a service, including available versions
class CSP_API ServiceStatus
{
public:
    /// @brief The reverse proxy endpoint or base URL through which the service is accessed.
    csp::common::String ReverseProxy;

    /// @brief The name identifier of the service (e.g., "User Service").
    csp::common::String Name;

    /// @brief A list of all available API versions for the service, along with their metadata.
    csp::common::Array<VersionMetadata> ApiVersions;

    /// @brief The currently active API version; this version is guaranteed to be stable and supported.
    csp::common::String CurrentApiVersion;

    bool operator==(const ServiceStatus& Other) const;
};

/// @ingroup Status System
/// @brief Store information about the current service deployment
class CSP_API ServicesDeploymentStatus
{
public:
    /// @brief The version identifier of the service container or deployment package.
    /// Typically reflects the software release version (e.g., "{Major}.{Minor}.{Patch}[-{Prerelease Tag}][+{Build Number}]").
    csp::common::String Version;

    /// @brief A list of services available in the current deployment, with their metadata and API versions.
    csp::common::Array<ServiceStatus> Services;

    bool operator==(const ServicesDeploymentStatus& Other) const;
};

/// @ingroup Status System
/// @brief Data class used to contain information when a Response is received from the Status Server
class CSP_API ServicesDeploymentStatusResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    friend class CSPFoundation;
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Will return services deployment status for the latest available deployment from the server
    /// @return ServicesDeploymentStatus : The latest services deployment status
    [[nodiscard]] const ServicesDeploymentStatus& GetLatestServicesDeploymentStatus() const;

private:
    ServicesDeploymentStatusResult() {};
    ServicesDeploymentStatusResult(void*) {};

    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;
    ServicesDeploymentStatus ServicesDeploymentStatusResponse;
};

/// @brief Callback containing a ServicesDeploymentStatus result used when creating or retrieving a ServicesDeploymentStatus.
/// @param Result ServicesDeploymentStatusResult : Data class containing information on task result/progress
typedef std::function<void(const ServicesDeploymentStatusResult& Result)> ServicesDeploymentStatusCallback;

} // namespace csp::systems
