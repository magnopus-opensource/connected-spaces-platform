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

/// @brief Stores the version and deprecation information for a services
class CSP_API ServiceVersionInfo
{
public:
    csp::common::String Version;
    csp::common::String DeprecationDatetime;
};

/// @brief Stores information about a service include available versions
class CSP_API ServiceInfo
{
public:
    csp::common::String ReverseProxy;
    csp::common::String Name;
    csp::common::Array<ServiceVersionInfo> ApiVersions;
    csp::common::String CurrentApiVersion;
};

/// @brief Store information about the current service deployment
class CSP_API StatusInfo
{
public:
    csp::common::String ContainerVersion;
    csp::common::Array<ServiceInfo> Services;
};

/// @ingroup CSPFoundation
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
    /// @return StatusInfo : the closest maintenance window information
    [[nodiscard]] const StatusInfo& GetLatestStatusInfo() const;

private:
    StatusInfoResult() {};
    StatusInfoResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;
    StatusInfo StatusInfoResponse;
};

typedef std::function<void(const StatusInfoResult& Result)> StatusInfoCallback;

} // namespace csp::systems
