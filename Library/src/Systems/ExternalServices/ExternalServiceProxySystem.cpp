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

#include "CSP/Systems/ExternalServices/ExternalServiceProxySystem.h"

#include "CSP/CSPFoundation.h"
#include "CallHelpers.h"
#include "Common/Convert.h"
#include "Events/EventListener.h"
#include "Events/EventSystem.h"
#include "Services/AggregationService/Api.h"
#include "Services/AggregationService/Dto.h"
#include "Systems/ResultHelpers.h"

using namespace csp;
using namespace csp::common;

namespace chs_aggregation = csp::services::generated::aggregationservice;

namespace csp::systems
{

inline const char* BoolToApiString(bool val) { return val ? "true" : "false"; }

/// @brief Generic function which will make a post request to the services proxy endpoint to trigger some specified operation of specified
/// service. The class is templated to allow specialized functions to customize the result object used to process the response sent back from
/// services.
template <class ResultType>
void InvokeOperationWithResult(
    csp::services::ApiBase* inExternalServiceProxyApi, const ExternalServicesOperationParams& params, StringResultCallback callback)
{
    auto tokenInfo = std::make_shared<chs_aggregation::ServiceRequest>();
    tokenInfo->SetServiceName(params.ServiceName);
    tokenInfo->SetOperationName(params.OperationName);
    tokenInfo->SetHelp(params.SetHelp);
    tokenInfo->SetParameters(Convert(params.Parameters));

    csp::services::ResponseHandlerPtr responseHandler
        = inExternalServiceProxyApi->CreateHandler<StringResultCallback, ResultType, void, chs_aggregation::ServiceResponse>(callback, nullptr);
    static_cast<chs_aggregation::ExternalServiceProxyApi*>(inExternalServiceProxyApi)->service_proxyPost({ tokenInfo }, responseHandler);
}

ExternalServiceProxySystem::ExternalServiceProxySystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , m_externalServiceProxyApi(nullptr)
{
}

ExternalServiceProxySystem::ExternalServiceProxySystem(csp::web::WebClient* inWebClient, csp::common::LogSystem& logSystem)
    : SystemBase(inWebClient, nullptr, &logSystem)
    , m_externalServiceProxyApi { new chs_aggregation::ExternalServiceProxyApi(inWebClient) }
{
}

ExternalServiceProxySystem::~ExternalServiceProxySystem() { delete (m_externalServiceProxyApi); }

void ExternalServiceProxySystem::InvokeOperation(const ExternalServicesOperationParams& params, StringResultCallback callback)
{
    InvokeOperationWithResult<ExternalServiceInvocationResult>(m_externalServiceProxyApi, params, callback);
}

void ExternalServiceProxySystem::GetAgoraUserToken(const AgoraUserTokenParams& params, StringResultCallback callback)
{
    // As a specialisation function, we know the service name, operation name, and help params.
    ExternalServicesOperationParams agoraUserTokenParams;
    agoraUserTokenParams.ServiceName = "Agora";
    agoraUserTokenParams.OperationName = "getUserToken";
    agoraUserTokenParams.SetHelp = false;

    // And we pull the rest of the params from the specialised struct provided.
    agoraUserTokenParams.Parameters["userId"] = params.AgoraUserId;
    agoraUserTokenParams.Parameters["channelName"] = params.ChannelName;
    agoraUserTokenParams.Parameters["referenceId"] = params.ReferenceId;
    agoraUserTokenParams.Parameters["lifespan"] = std::to_string(params.Lifespan).c_str();
    agoraUserTokenParams.Parameters["readOnly"] = BoolToApiString(params.ReadOnly);
    agoraUserTokenParams.Parameters["shareAudio"] = BoolToApiString(params.ShareAudio);
    agoraUserTokenParams.Parameters["shareVideo"] = BoolToApiString(params.ShareVideo);
    agoraUserTokenParams.Parameters["shareScreen"] = BoolToApiString(params.ShareScreen);

    InvokeOperationWithResult<GetAgoraTokenResult>(m_externalServiceProxyApi, agoraUserTokenParams, callback);
}

} // namespace csp::systems
