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
#include "CSP/Systems/Multiplayer/Scope.h"
#include "CSP/Systems/Multiplayer/ScopeLeader.h"
#include "CSP/Systems/SystemBase.h"

#include <memory>

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

/// @ingroup Multiplayer System
/// @brief Public facing system that allows interfacing with Magnopus Connected Services' multiplayer api.
/// Offers methods for managing realtime state via REST calls.
class CSP_API CSP_NO_DISPOSE MultiplayerSystem : public SystemBase
{
public:
    CSP_NO_EXPORT MultiplayerSystem(csp::web::WebClient* WebClient, csp::common::LogSystem& LogSystem);
    MultiplayerSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    ~MultiplayerSystem();

    /// @brief Gets all scopes associated with the given space id.
    /// @param SpaceId const csp::common::String& : The id of the space we want to get scopes for.
    /// @param Callback csp::systems::ScopesResultCallback : Callback when asynchronous task finishes.
    /// @pre Must already have entered the space of the SpaceId parameter.
    CSP_ASYNC_RESULT void GetScopesBySpace(const csp::common::String& SpaceId, ScopesResultCallback Callback);

    /// @brief Updates Data on a scope
    /// @param ScopeId const csp::common::String& : The id of the scope we want to update.
    /// @param Scope const csp::systems::Scope& : Scope containing new values for the given id.
    /// @param Callback csp::systems::ScopesResultCallback : Callback when asynchronous task finishes.
    /// @pre Must already have entered the space of the SpaceId parameter.
    CSP_ASYNC_RESULT void UpdateScopeById(const csp::common::String& ScopeId, const csp::systems::Scope& Scope, ScopeResultCallback Callback);

    /// @brief Gets details about a scope leader.
    /// @param ScopeId const csp::common::String& : The id of the scope we want to get scope leader details about.
    /// @param Callback csp::systems::ScopesResultCallback : Callback when asynchronous task finishes.
    /// @pre "ManagedLeaderElection" should be set to true on the scope, otherwise this function will fail with a EResultCode::Failed response.
    /// @pre Must already have entered the space of the SpaceId parameter.
    CSP_ASYNC_RESULT void GetScopeLeader(const csp::common::String& ScopeId, ScopeLeaderResultCallback Callback);

    /// @brief Starts leader election for the given scope.
    /// This should not need to be called outside of testing, as leader election is done automatically.
    /// @param ScopeId const csp::common::String& : The id of the scope we want to start leader election for.
    /// @param UserIdsToExclude const csp::common::Optional<csp::common::Array<csp::common::String>>& : A list of user ids we don't want to consider
    /// for election.
    /// @param Callback csp::systems::ScopesResultCallback : Callback when asynchronous task finishes.
    /// @pre "ManagedLeaderElection" should be set to true on the scope, otherwise this function will fail with a EResultCode::Failed response.
    /// @pre Must already have entered the space of the SpaceId parameter.
    CSP_ASYNC_RESULT void PerformLeaderElectionInScope(const csp::common::String& ScopeId,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& UserIdsToExclude, NullResultCallback Callback);

private:
    csp::services::ApiBase* ScopeLeaderApi;
    csp::services::ApiBase* ScopesApi;
};

}
