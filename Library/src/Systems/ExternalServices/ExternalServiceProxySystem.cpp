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

inline const char* BoolToApiString(bool Val) { return Val ? "true" : "false"; }

/// @brief Generic function which will make a post request to the services proxy endpoint to trigger some specified operation of specified
/// service. The class is templated to allow specialized functions to customize the result object used to process the response sent back from
/// services.
template <class ResultType>
void InvokeOperationWithResult(
    csp::services::ApiBase* InExternalServiceProxyApi, const ExternalServicesOperationParams& Params, StringResultCallback Callback)
{
    auto TokenInfo = std::make_shared<chs_aggregation::ServiceRequest>();
    TokenInfo->SetServiceName(Params.ServiceName);
    TokenInfo->SetOperationName(Params.OperationName);
    TokenInfo->SetHelp(Params.SetHelp);
    TokenInfo->SetParameters(Convert(Params.Parameters));

    csp::services::ResponseHandlerPtr ResponseHandler
        = InExternalServiceProxyApi->CreateHandler<StringResultCallback, ResultType, void, chs_aggregation::ServiceResponse>(Callback, nullptr);
    static_cast<chs_aggregation::ExternalServiceProxyApi*>(InExternalServiceProxyApi)->service_proxyPost({ TokenInfo }, ResponseHandler);
}

ExternalServiceProxySystem::ExternalServiceProxySystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , ExternalServiceProxyApi(nullptr)
{
}

ExternalServiceProxySystem::ExternalServiceProxySystem(csp::web::WebClient* InWebClient, csp::common::LogSystem& LogSystem)
    : SystemBase(InWebClient, nullptr, &LogSystem)
    , ExternalServiceProxyApi { new chs_aggregation::ExternalServiceProxyApi(InWebClient) }
{
}

ExternalServiceProxySystem::~ExternalServiceProxySystem() { delete (ExternalServiceProxyApi); }

void ExternalServiceProxySystem::InvokeOperation(const ExternalServicesOperationParams& Params, StringResultCallback Callback)
{
    InvokeOperationWithResult<ExternalServiceInvocationResult>(ExternalServiceProxyApi, Params, Callback);
}

void ExternalServiceProxySystem::GetAgoraUserToken(const AgoraUserTokenParams& Params, StringResultCallback Callback)
{
    // As a specialisation function, we know the service name, operation name, and help params.
    ExternalServicesOperationParams AgoraUserTokenParams;
    AgoraUserTokenParams.ServiceName = "Agora";
    AgoraUserTokenParams.OperationName = "getUserToken";
    AgoraUserTokenParams.SetHelp = false;

    // And we pull the rest of the params from the specialised struct provided.
    AgoraUserTokenParams.Parameters["userId"] = Params.AgoraUserId;
    AgoraUserTokenParams.Parameters["channelName"] = Params.ChannelName;
    AgoraUserTokenParams.Parameters["referenceId"] = Params.ReferenceId;
    AgoraUserTokenParams.Parameters["lifespan"] = std::to_string(Params.Lifespan).c_str();
    AgoraUserTokenParams.Parameters["readOnly"] = BoolToApiString(Params.ReadOnly);
    AgoraUserTokenParams.Parameters["shareAudio"] = BoolToApiString(Params.ShareAudio);
    AgoraUserTokenParams.Parameters["shareVideo"] = BoolToApiString(Params.ShareVideo);
    AgoraUserTokenParams.Parameters["shareScreen"] = BoolToApiString(Params.ShareScreen);

    InvokeOperationWithResult<GetAgoraTokenResult>(ExternalServiceProxyApi, AgoraUserTokenParams, Callback);
}

} // namespace csp::systems
