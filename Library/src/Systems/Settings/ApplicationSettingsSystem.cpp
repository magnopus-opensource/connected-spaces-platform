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

#include "CSP/Systems/ContinuationUtils.h"

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

void ApplicationSettingsSystem::GetSettingsByContext(
    const String& ApplicationName, const String& Context, const Optional<Array<String>>& Keys, ApplicationSettingsResultCallback Callback)
{
    GetSettingsByContext(ApplicationName, Context, Keys)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<ApplicationSettingsResult>(Callback,
            "ApplicationSettingsSystem::GetSettingsByContext successfully retrieved application settings", "Failed to get application settings", {},
            {}, {}))
        .then([Callback](const ApplicationSettingsResult& Result) { Callback(Result); })
        .then(common::continuations::InvokeIfExceptionInChain(
            [Callback](const std::exception& /*exception*/) { Callback(MakeInvalid<ApplicationSettingsResult>()); }, *LogSystem));
}

void ApplicationSettingsSystem::GetSettingsByContextAnonymous(const csp::common::String& Tenant, const csp::common::String& ApplicationName,
    const csp::common::String& Context, const csp::common::Optional<csp::common::Array<csp::common::String>>& Keys,
    ApplicationSettingsResultCallback Callback)
{
    GetSettingsByContextAnonymous(Tenant, ApplicationName, Context, Keys)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<ApplicationSettingsResult>(Callback,
            "ApplicationSettingsSystem::GetSettingsByContextAnonymous successfully retrieved application settings",
            "Failed to get application settings", {}, {}, {}))
        .then([Callback](const ApplicationSettingsResult& Result) { Callback(Result); })
        .then(common::continuations::InvokeIfExceptionInChain(
            [Callback](const std::exception& /*exception*/) { Callback(MakeInvalid<ApplicationSettingsResult>()); }, *LogSystem));
}

void ApplicationSettingsSystem::CreateSettingsByContext(const ApplicationSettings& ApplicationSettings, ApplicationSettingsResultCallback Callback)
{
    CreateSettingsByContext(ApplicationSettings)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<ApplicationSettingsResult>(Callback,
            "ApplicationSettingsSystem::CreateSettingsByContext successfully created application settings", "Failed to create application settings",
            {}, {}, {}))
        .then([Callback](const ApplicationSettingsResult& Result) { Callback(Result); })
        .then(common::continuations::InvokeIfExceptionInChain(
            [Callback](const std::exception& /*exception*/) { Callback(MakeInvalid<ApplicationSettingsResult>()); }, *LogSystem));
}

async::task<ApplicationSettingsResult> ApplicationSettingsSystem::CreateSettingsByContext(const ApplicationSettings& ApplicationSettings)
{
    auto OnCompleteEvent = std::make_shared<async::event_task<ApplicationSettingsResult>>();
    async::task<ApplicationSettingsResult> OnCompleteTask = OnCompleteEvent->get_task();

    if (ApplicationSettings.ApplicationName.IsEmpty())
    {
        CSP_LOG_ERROR_MSG(
            "Your request for application settings was made without a valid application name. A valid application name must be provided.");
        OnCompleteEvent->set_exception(std::make_exception_ptr(std::exception()));
        return OnCompleteTask;
    }

    if (ApplicationSettings.Context.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("Your request for application settings was made without a valid context. A valid application context must be provided.");
        OnCompleteEvent->set_exception(std::make_exception_ptr(std::exception()));
        return OnCompleteTask;
    }

    services::ResponseHandlerPtr SettingsResponseHandler
        = ApplicationSettingsAPI->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            [](const ApplicationSettingsResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(*OnCompleteEvent.get()));

    auto Request = std::make_shared<chs::ApplicationSettingsDto>();
    Request->SetAllowAnonymous(ApplicationSettings.AllowAnonymous);
    Request->SetSettings(Convert(ApplicationSettings.Settings));

    static_cast<chs::ApplicationSettingsApi*>(ApplicationSettingsAPI)
        ->applicationsApplicationNameSettingsContextPut(
            ApplicationSettings.ApplicationName, ApplicationSettings.Context, Request, SettingsResponseHandler);

    return OnCompleteTask;
}

async::task<ApplicationSettingsResult> ApplicationSettingsSystem::GetSettingsByContext(const csp::common::String& ApplicationName,
    const csp::common::String& Context, const csp::common::Optional<csp::common::Array<csp::common::String>>& Keys)
{
    auto OnCompleteEvent = std::make_shared<async::event_task<ApplicationSettingsResult>>();
    async::task<ApplicationSettingsResult> OnCompleteTask = OnCompleteEvent->get_task();

    if (ApplicationName.IsEmpty())
    {
        CSP_LOG_ERROR_MSG(
            "Your request for application settings was made without a valid application name. A valid application name must be provided.");
        OnCompleteEvent->set_exception(std::make_exception_ptr(std::exception()));
        return OnCompleteTask;
    }

    if (Context.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("Your request for application settings was made without a valid context. A valid application context must be provided.");
        OnCompleteEvent->set_exception(std::make_exception_ptr(std::exception()));
        return OnCompleteTask;
    }

    services::ResponseHandlerPtr SettingsResponseHandler
        = ApplicationSettingsAPI->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            [](const ApplicationSettingsResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(*OnCompleteEvent.get()));

    static_cast<chs::ApplicationSettingsApi*>(ApplicationSettingsAPI)
        ->applicationsApplicationNameSettingsContextGet(ApplicationName, Context, Convert(Keys), SettingsResponseHandler);

    return OnCompleteTask;
}

async::task<ApplicationSettingsResult> ApplicationSettingsSystem::GetSettingsByContextAnonymous(const csp::common::String& Tenant,
    const csp::common::String& ApplicationName, const csp::common::String& Context,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& Keys)
{
    auto OnCompleteEvent = std::make_shared<async::event_task<ApplicationSettingsResult>>();
    async::task<ApplicationSettingsResult> OnCompleteTask = OnCompleteEvent->get_task();

    if (ApplicationName.IsEmpty())
    {
        CSP_LOG_ERROR_MSG(
            "Your request for application settings was made without a valid application name. A valid application name must be provided.");
        OnCompleteEvent->set_exception(std::make_exception_ptr(std::exception()));
        return OnCompleteTask;
    }

    if (Context.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("Your request for application settings was made without a valid context. A valid application context must be provided.");
        OnCompleteEvent->set_exception(std::make_exception_ptr(std::exception()));
        return OnCompleteTask;
    }

    services::ResponseHandlerPtr SettingsResponseHandler
        = ApplicationSettingsAPI->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            [](const ApplicationSettingsResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(*OnCompleteEvent.get()));

    static_cast<chs::ApplicationSettingsApi*>(ApplicationSettingsAPI)
        ->tenantsTenantApplicationsApplicationNameSettingsContextGet(Tenant, ApplicationName, Context, Convert(Keys), SettingsResponseHandler);

    return OnCompleteTask;
}

} // namespace csp::systems
