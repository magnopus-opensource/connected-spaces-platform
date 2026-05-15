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
    , m_applicationSettingsApi(nullptr)
{
}

ApplicationSettingsSystem::ApplicationSettingsSystem(csp::web::WebClient* inWebClient, csp::common::LogSystem& logSystem)
    : SystemBase(inWebClient, nullptr, &logSystem)
{
    m_applicationSettingsApi = new chs::ApplicationSettingsApi(inWebClient);
}

ApplicationSettingsSystem::~ApplicationSettingsSystem() { delete (m_applicationSettingsApi); }

void ApplicationSettingsSystem::GetSettingsByContext(
    const String& applicationName, const String& context, const Optional<Array<String>>& keys, ApplicationSettingsResultCallback callback)
{
    GetSettingsByContext(applicationName, context, keys)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<ApplicationSettingsResult>(
            "ApplicationSettingsSystem::GetSettingsByContext successfully retrieved application settings", "Failed to get application settings", {},
            {}, {}))
        .then([callback](const ApplicationSettingsResult& result) { callback(result); })
        .then(common::continuations::InvokeIfExceptionInChain(*m_logSystem,
            [callback]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
            { callback(csp::common::continuations::GetResultExceptionOrInvalid<ApplicationSettingsResult>(exception)); }));
}

void ApplicationSettingsSystem::GetSettingsByContextAnonymous(const csp::common::String& tenant, const csp::common::String& applicationName,
    const csp::common::String& context, const csp::common::Optional<csp::common::Array<csp::common::String>>& keys,
    ApplicationSettingsResultCallback callback)
{
    GetSettingsByContextAnonymous(tenant, applicationName, context, keys)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<ApplicationSettingsResult>(
            "ApplicationSettingsSystem::GetSettingsByContextAnonymous successfully retrieved application settings",
            "Failed to get application settings", {}, {}, {}))
        .then([callback](const ApplicationSettingsResult& result) { callback(result); })
        .then(common::continuations::InvokeIfExceptionInChain(*m_logSystem,
            [callback]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
            { callback(csp::common::continuations::GetResultExceptionOrInvalid<ApplicationSettingsResult>(exception)); }));
}

void ApplicationSettingsSystem::CreateSettingsByContext(const ApplicationSettings& applicationSettings, ApplicationSettingsResultCallback callback)
{
    CreateSettingsByContext(applicationSettings)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<ApplicationSettingsResult>(
            "ApplicationSettingsSystem::CreateSettingsByContext successfully created application settings", "Failed to create application settings",
            {}, {}, {}))
        .then([callback](const ApplicationSettingsResult& result) { callback(result); })
        .then(common::continuations::InvokeIfExceptionInChain(*m_logSystem,
            [callback]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
            { callback(csp::common::continuations::GetResultExceptionOrInvalid<ApplicationSettingsResult>(exception)); }));
}

async::task<ApplicationSettingsResult> ApplicationSettingsSystem::CreateSettingsByContext(const ApplicationSettings& applicationSettings)
{
    auto onCompleteEvent = std::make_shared<async::event_task<ApplicationSettingsResult>>();
    async::task<ApplicationSettingsResult> onCompleteTask = onCompleteEvent->get_task();

    if (applicationSettings.ApplicationName.IsEmpty())
    {
        CSP_LOG_ERROR_MSG(
            "Your request for application settings was made without a valid application name. A valid application name must be provided.");
        onCompleteEvent->set_exception(std::make_exception_ptr(std::exception()));
        return onCompleteTask;
    }

    if (applicationSettings.Context.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("Your request for application settings was made without a valid context. A valid application context must be provided.");
        onCompleteEvent->set_exception(std::make_exception_ptr(std::exception()));
        return onCompleteTask;
    }

    services::ResponseHandlerPtr settingsResponseHandler
        = m_applicationSettingsApi->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            [](const ApplicationSettingsResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(*onCompleteEvent.get()));

    auto request = std::make_shared<chs::ApplicationSettingsDto>();
    request->SetAllowAnonymous(applicationSettings.AllowAnonymous);
    request->SetSettings(Convert(applicationSettings.Settings));

    static_cast<chs::ApplicationSettingsApi*>(m_applicationSettingsApi)
        ->applicationsApplicationNameSettingsContextPut(
            { applicationSettings.ApplicationName, applicationSettings.Context, request }, settingsResponseHandler);

    return onCompleteTask;
}

async::task<ApplicationSettingsResult> ApplicationSettingsSystem::GetSettingsByContext(const csp::common::String& applicationName,
    const csp::common::String& context, const csp::common::Optional<csp::common::Array<csp::common::String>>& keys)
{
    auto onCompleteEvent = std::make_shared<async::event_task<ApplicationSettingsResult>>();
    async::task<ApplicationSettingsResult> onCompleteTask = onCompleteEvent->get_task();

    if (applicationName.IsEmpty())
    {
        CSP_LOG_ERROR_MSG(
            "Your request for application settings was made without a valid application name. A valid application name must be provided.");
        onCompleteEvent->set_exception(std::make_exception_ptr(std::exception()));
        return onCompleteTask;
    }

    if (context.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("Your request for application settings was made without a valid context. A valid application context must be provided.");
        onCompleteEvent->set_exception(std::make_exception_ptr(std::exception()));
        return onCompleteTask;
    }

    services::ResponseHandlerPtr settingsResponseHandler
        = m_applicationSettingsApi->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            [](const ApplicationSettingsResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(*onCompleteEvent.get()));

    static_cast<chs::ApplicationSettingsApi*>(m_applicationSettingsApi)
        ->applicationsApplicationNameSettingsContextGet({ applicationName, context, Convert(keys) }, settingsResponseHandler);

    return onCompleteTask;
}

async::task<ApplicationSettingsResult> ApplicationSettingsSystem::GetSettingsByContextAnonymous(const csp::common::String& tenant,
    const csp::common::String& applicationName, const csp::common::String& context,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& keys)
{
    auto onCompleteEvent = std::make_shared<async::event_task<ApplicationSettingsResult>>();
    async::task<ApplicationSettingsResult> onCompleteTask = onCompleteEvent->get_task();

    if (applicationName.IsEmpty())
    {
        CSP_LOG_ERROR_MSG(
            "Your request for application settings was made without a valid application name. A valid application name must be provided.");
        onCompleteEvent->set_exception(std::make_exception_ptr(std::exception()));
        return onCompleteTask;
    }

    if (context.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("Your request for application settings was made without a valid context. A valid application context must be provided.");
        onCompleteEvent->set_exception(std::make_exception_ptr(std::exception()));
        return onCompleteTask;
    }

    services::ResponseHandlerPtr settingsResponseHandler
        = m_applicationSettingsApi->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            [](const ApplicationSettingsResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(*onCompleteEvent.get()));

    static_cast<chs::ApplicationSettingsApi*>(m_applicationSettingsApi)
        ->tenantsTenantApplicationsApplicationNameSettingsContextGet({ tenant, applicationName, context, Convert(keys) }, settingsResponseHandler);

    return onCompleteTask;
}

} // namespace csp::systems
