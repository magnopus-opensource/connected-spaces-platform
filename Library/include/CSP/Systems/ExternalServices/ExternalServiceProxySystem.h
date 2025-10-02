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
#include "CSP/Systems/SystemsResult.h"

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

/// *******************************************************************************************************************
/// @brief Data structure used when making invocations of the external service proxy.
class CSP_API ExternalServicesOperationParams
{
public:
    /// @brief The name of the external service of concern.
    csp::common::String ServiceName;

    /// @brief The operation name that should be performed by the proxy service.
    csp::common::String OperationName;

    /// @brief A flag indicating this invocation is actually a request for information about the operation identified by OperationName.
    bool SetHelp;

    /// @brief Map of parameters required in order for the proxy service to complete the operation.
    csp::common::Map<csp::common::String, csp::common::String> Parameters;
};

/// *******************************************************************************************************************
/// @brief Specialised data structure that can be used to retrieve an Agora user token via the proxy service.
class CSP_API AgoraUserTokenParams
{
public:
    /// @brief The unique identifer for the user requesting the token.
    csp::common::String AgoraUserId;

    /// @brief The unique name for the Agora channel being joined. It can be set to any string combination. For group calls all users must reference
    /// the same channelName.
    csp::common::String ChannelName;

    /// @brief The unique identfier for the space being joined. Only needs to be set if the channelName is not set to the space ID, so the appropriate
    /// permissions can be requested. It can be set to an empty string if not required.
    csp::common::String ReferenceId;

    /// @brief The amount of time the token is valid for in milliseconds.
    int Lifespan;

    /// @brief If the token is ready only.
    bool ReadOnly;

    /// @brief If the token is configured for sharing of audio.
    bool ShareAudio;

    /// @brief If the token is configured for sharing of video.
    bool ShareVideo;

    /// @brief If the token is configured for sharing of the user's screen.
    bool ShareScreen;
};

/// *******************************************************************************************************************
/// @ingroup External Service Proxy System
/// @brief Public facing system that allows client applications to interact with
/// backend services acting as a proxy for some other set of services.
///
/// In situations where a CSP client application needs secure access to some other set of third-party
/// services (like AI, VOIP, ECommerce etc), this system can be particularly helpful,
/// as it enables CSP services (like MCS) to own the responsibility of distributing secure tokens to those platforms.
class CSP_API ExternalServiceProxySystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */
    CSP_END_IGNORE

public:
    /// @brief Generic function which will make a post request to the services proxy endpoint to trigger some specified operation of specified
    /// service. The nature of the operation and what is returned is entirely dependent on the service and operation name provided.
    /// @param Params const TokenInfoParams& : Params to specify service, operation, set help and parameters.
    /// @param Callback StringResultCallback : Callback to call when a response is received.
    CSP_ASYNC_RESULT void InvokeOperation(const ExternalServicesOperationParams& Params, StringResultCallback Callback);

    /// @brief Specialised utility function which executes a post call to the external services proxy, specifically to retrieve Agora user token
    /// credentials. A good example for how client applications may wish to use PostServiceProxy.
    /// @param Params const AgoraUserTokenParams& : Params to configure the User token
    /// @param Callback StringResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void GetAgoraUserToken(const AgoraUserTokenParams& Params, StringResultCallback Callback);

private:
    ExternalServiceProxySystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT ExternalServiceProxySystem(csp::web::WebClient* InWebClient, common::LogSystem& LogSystem);
    ~ExternalServiceProxySystem();

    csp::services::ApiBase* ExternalServiceProxyApi;
};

} // namespace csp::systems
