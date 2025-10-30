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
#include "CSP/Systems/SystemsResult.h"
#include "CSP/Systems/WebService.h"

#include <functional>

namespace csp::services
{

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::services::generated::multiplayerservice
{
class ScopeLeaderDto;
}

namespace csp::systems
{

/// @ingroup Multiplayer System
/// @brief Data representation for a scope leader.
/// A scope leader represents a user which owns a specific scope in a space.
/// The scope leader will run scripts and other operations for the scope.
class CSP_API ScopeLeader
{
public:
    /// @brief The scope id the client is leader of.
    csp::common::String ScopeId;
    /// @brief The client id which is the leader.
    csp::common::String ScopeLeaderUserId;
    /// @brief Whether there is a server side election currently in progress when this object is received.
    bool ElectionInProgress = false;
};

void DtoToScopeLeader(const csp::services::generated::multiplayerservice::ScopeLeaderDto& Dto, csp::systems::ScopeLeader& ScopeLeader);

/// @ingroup Multiplayer System
/// @brief Contains details about an async operation which returns a scope leader.
/// If the ResultCode is successful, this will contain a valid scope leader.
class CSP_API ScopeLeaderResult : public ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Returns the scope leader if this result is successful.
    /// @return const ScopeLeader& : The scope leader retrieved by this result.
    const ScopeLeader& GetScopeLeader() const;

private:
    ScopeLeaderResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    ScopeLeader Leader;
};

typedef std::function<void(const ScopeLeaderResult& Result)> ScopeLeaderResultCallback;
}
