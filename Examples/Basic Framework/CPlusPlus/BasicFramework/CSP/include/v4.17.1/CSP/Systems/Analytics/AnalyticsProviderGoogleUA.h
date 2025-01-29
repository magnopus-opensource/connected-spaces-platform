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

#include "AnalyticsProvider.h"

#include <chrono>

namespace csp::web
{
class WebClient;
}

namespace csp::systems
{

/// @ingroup Analytics System
/// @brief Analytics Provider implementation for Google Universal Analytics
class AnalyticsProviderGoogleUA : public IAnalyticsProvider
{
public:
    AnalyticsProviderGoogleUA(const csp::common::String& ClientId, const csp::common::String& PropertyTag);

    CSP_START_IGNORE
    void Log(AnalyticsEvent* Event) override;
    CSP_END_IGNORE

private:
    csp::common::String ClientId;
    csp::common::String PropertyTag;

    csp::web::WebClient* WebClient;

    std::chrono::steady_clock::time_point Start;
};

csp::common::String CreateUAEventString(
    const csp::common::String& ClientId, const csp::common::String& PropertyTag, csp::systems::AnalyticsEvent* Event);
} // namespace csp::systems
