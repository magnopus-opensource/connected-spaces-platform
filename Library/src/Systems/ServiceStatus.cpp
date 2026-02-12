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

bool VersionMetadata::operator==(const VersionMetadata& Other) const
{
    return Version == Other.Version && DeprecationDatetime == Other.DeprecationDatetime;
}

bool ServiceStatus::operator==(const ServiceStatus& Other) const
{
    return ReverseProxy == Other.ReverseProxy && Name == Other.Name && ApiVersions == Other.ApiVersions
        && CurrentApiVersion == Other.CurrentApiVersion;
}

bool ServicesDeploymentStatus::operator==(const ServicesDeploymentStatus& Other) const
{
    return Version == Other.Version && Services == Other.Services;
}

bool VersionMetadata::operator!=(const VersionMetadata& Other) const { return !(*this == Other); }
bool ServiceStatus::operator!=(const ServiceStatus& Other) const { return !(*this == Other); }
bool ServicesDeploymentStatus::operator!=(const ServicesDeploymentStatus& Other) const { return !(*this == Other); }

const ServicesDeploymentStatus& ServicesDeploymentStatusResult::GetLatestServicesDeploymentStatus() const { return ServicesDeploymentStatusResponse; }

void ServicesDeploymentStatusResult::OnResponse(const csp::services::ApiResponseBase* /*ApiResponse*/) { }

} // namespace csp::systems
