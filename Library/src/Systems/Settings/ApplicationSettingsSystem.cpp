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

#include "CSP/Systems/Settings/ApplicationSettingsSystem.h"

#include "CallHelpers.h"
#include "Common/Convert.h"
#include "Services/UserService/Api.h"
#include "Services/UserService/Dto.h"
#include "Systems/ResultHelpers.h"

using namespace csp::common;

namespace chs = csp::services::generated::userservice;

namespace csp::systems
{

ApplicationSettingsSystem::ApplicationSettingsSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , ApplicationSettingsAPI(nullptr)
{
}

ApplicationSettingsSystem::ApplicationSettingsSystem(csp::web::WebClient* InWebClient, csp::common::LogSystem& LogSystem)
    : SystemBase(InWebClient, nullptr, &LogSystem)
{
    ApplicationSettingsAPI = new chs::ApplicationSettingsApi(InWebClient);
}

ApplicationSettingsSystem::~ApplicationSettingsSystem() { delete (ApplicationSettingsAPI); }

} // namespace csp::systems
