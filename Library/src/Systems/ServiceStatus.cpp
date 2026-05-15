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

#include "CSP/Systems/ServiceStatus.h"

#include "Debug/Logging.h"
#include "Services/ApiBase/ApiBase.h"
#include "Systems/ResultHelpers.h"

namespace csp::systems
{

bool VersionMetadata::operator==(const VersionMetadata& other) const
{
    return Version == other.Version && DeprecationDatetime == other.DeprecationDatetime;
}

bool ServiceStatus::operator==(const ServiceStatus& other) const
{
    return ReverseProxy == other.ReverseProxy && Name == other.Name && ApiVersions == other.ApiVersions
        && CurrentApiVersion == other.CurrentApiVersion;
}

bool ServicesDeploymentStatus::operator==(const ServicesDeploymentStatus& other) const
{
    return Version == other.Version && Services == other.Services;
}

bool VersionMetadata::operator!=(const VersionMetadata& other) const { return !(*this == other); }
bool ServiceStatus::operator!=(const ServiceStatus& other) const { return !(*this == other); }
bool ServicesDeploymentStatus::operator!=(const ServicesDeploymentStatus& other) const { return !(*this == other); }

const ServicesDeploymentStatus& ServicesDeploymentStatusResult::GetLatestServicesDeploymentStatus() const { return m_servicesDeploymentStatusResponse; }

void ServicesDeploymentStatusResult::OnResponse(const csp::services::ApiResponseBase* /*ApiResponse*/) { }

} // namespace csp::systems
