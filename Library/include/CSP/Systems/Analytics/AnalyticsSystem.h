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

#include "CSP/CSPCommon.h"
#include "CSP/Common/CancellationToken.h"
#include "CSP/Common/Optional.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/SystemBase.h"

namespace csp
{
class ClientUserAgent;
} // namespace csp

namespace csp::services
{
class ApiBase;
} // namespace csp::services

namespace csp::web
{
class WebClient;
} // namespace csp::web

namespace csp::systems
{

/// @ingroup Analytics System
/// @brief Public facing system that allows AnalyticsRecords to be sent to MCS.
class CSP_API AnalyticsSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */
    CSP_END_IGNORE

public:
    /**
     * @brief Sends an analytics event to the Analytics service.
     * @details Please note: The BatchAnalyticsEvent method should be used by default as it will batch events before sending them.
     * This method will immediately send the analytics event and should therefore only be used when this behaviour is required.
     * Constructs an Analytics Record with the following format:
     *      {ProductIdentifer}.{ProductContext}.{Tenant}.{ProductContextSection}.{Category}.{InteractionType}.{SubCategory}
     *      csp.web-client-x.CLIENT_TENANT_X.UI.menus.click
     * @param ProductContextSection const csp::common::String& : Section of the client or runtime-context - 'UI' in the example provided above.
     * @param Category const csp::common::String& : Categorization field - 'menus' in the example provided above.
     * @param InteractionType const csp::common::String& : The interaction that occurred - 'click' in the example provided above.
     * @param SubCategory const csp::common::Optional<csp::common::String>& : Optional sub-category field - not used in the example provided above.
     * @param Metadata const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& : Optional analytics event metadata -
     * not used in the example provided above.
     */
    void SendAnalyticsEvent(const csp::common::String& ProductContextSection, const csp::common::String& Category,
        const csp::common::String& InteractionType, const csp::common::Optional<csp::common::String>& SubCategory,
        const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& Metadata, NullResultCallback Callback);

private:
    AnalyticsSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT AnalyticsSystem(csp::web::WebClient* InWebClient, const csp::ClientUserAgent* AgentInfo, common::LogSystem& LogSystem);
    ~AnalyticsSystem();

    csp::services::ApiBase* AnalyticsApi;

    const csp::ClientUserAgent* UserAgentInfo;
};

} // namespace csp::systems
