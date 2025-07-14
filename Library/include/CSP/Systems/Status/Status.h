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
class CSP_API ServiceVersionInfo
{
public:
    /// @brief The version identifier of the service's API (e.g., "v1").
    csp::common::String Version;

    /// @brief The date and time at which the API version is considered deprecated, in ISO 8601 format.
    /// If empty, the version is currently active or the deprecation date is unknown.
    csp::common::String DeprecationDatetime;
};

/// @ingroup Status System
/// @brief Stores information about a service including available versions
class CSP_API ServiceInfo
{
public:
    /// @brief The reverse proxy endpoint or base URL through which the service is accessed.
    csp::common::String ReverseProxy;

    /// @brief The name identifier of the service (e.g., "User Service").
    csp::common::String Name;

    /// @brief A list of all available API versions for the service, along with their metadata.
    csp::common::Array<ServiceVersionInfo> ApiVersions;

    /// @brief The currently active API version, this version is guaranteed to be stable and supported.
    csp::common::String CurrentApiVersion;
};

/// @ingroup Status System
/// @brief Store information about the current service deployment
class CSP_API StatusInfo
{
public:
    /// @brief The version identifier of the service container or deployment package.
    /// Typically reflects the software release version (e.g., "{Major}.{Minor}.{Patch}[-{Prerelease Tag}][+{Build Number}]").
    csp::common::String ContainerVersion;

    /// @brief A list of services available in the current deployment, with their metadata and API versions.
    csp::common::Array<ServiceInfo> Services;
};

/// @ingroup Status System
/// @brief Data class used to contain information when a Response is received from Status Server
class CSP_API StatusInfoResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    friend class CSPFoundation;
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Will return info for the latest available status information from the server
    /// @return StatusInfo : The latest status information
    [[nodiscard]] const StatusInfo& GetLatestStatusInfo() const;

private:
    StatusInfoResult() {};
    StatusInfoResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;
    StatusInfo StatusInfoResponse;
};

/// @brief Callback containing an StatusInfo result used when creating or retrieving an StatusInfo.
/// @param Result StatusInfoResult : Data class containing information on task result/progress
typedef std::function<void(const StatusInfoResult& Result)> StatusInfoCallback;

} // namespace csp::systems
