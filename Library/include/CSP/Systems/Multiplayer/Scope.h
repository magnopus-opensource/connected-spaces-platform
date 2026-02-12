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
class ScopeDto;
}

namespace csp::systems
{

/// @ingroup Multiplayer System
/// @brief Enum representing the scopes pub/sub model type
/// Object: used in object scopes, each object is published to its own channel, and
/// client subscribes to the channels of only the objects they can see
/// Global: used in global scopes, all objects are published to a single channel,
/// client subscribes to the channel and can see everything in the channel/scope
enum class PubSubModelType
{
    Object,
    Global
};

/// @ingroup Multiplayer System
/// @brief Data representation for a scope in a space.
/// Scopes represent different channels in a space which objects can exist in.
/// This allows csp/mcs to only reason about objects in specific scopes.
class CSP_API Scope
{
public:
    /// @brief The unique identifier of the scope.
    /// This is set internally by MCS.
    csp::common::String Id;
    /// @brief The id of the object this scope relates to.
    /// This is currently always the space id.
    csp::common::String ReferenceId;
    /// @brief The type of object this scope relates to.
    /// This is currently always "GroupId", as it references the space.
    csp::common::String ReferenceType;
    /// @brief The name of the scope, this should be a human readable string to identify the scope.
    csp::common::String Name;
    /// @brief The pub/sub model of the scope.
    /// This allows us define a global scope for the entire space, or a scope with a position and size.
    /// See csp::systems::PubSubModelType for more details.
    PubSubModelType PubSubType = PubSubModelType::Global;
    /// @brief Determines the size of the scope using the radius from the object in meters.
    /// This is only used when PubSubType is set to "Object".
    double SolveRadius = 0.0;
    /// @brief Determines whether server side leader election is enabled on this scope.
    /// If this is true, MCS will automatically determine the leader for this scope.
    bool ManagedLeaderElection = false;

    bool operator==(const Scope& Other) const;
    bool operator!=(const Scope& Other) const;
};

void DtoToScope(const csp::services::generated::multiplayerservice::ScopeDto& Dto, csp::systems::Scope& ScopeLeader);

/// @ingroup Multiplayer System
/// @brief Contains details about an async operation which returns a scope.
/// If the ResultCode is successful, this will contain a valid scope.
class CSP_API ScopeResult : public ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Returns the scope if this result is successful.
    /// @return const Scope& : The scope retrieved by this result.
    const Scope& GetScope() const;

    CSP_NO_EXPORT ScopeResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode);

private:
    ScopeResult(void*) {};

    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    Scope Scope;
};

typedef std::function<void(const ScopeResult& Result)> ScopeResultCallback;

/// @ingroup Multiplayer System
/// @brief Contains details about an async operation which returns an array of scopes.
/// If the ResultCode is successful, this will contain a valid array of scopes.
class CSP_API ScopesResult : public ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Returns the an array of scopes if this result is successful.
    /// @return const csp::common::Array<Scope>& : The array of scopes retrieved by this result.
    const csp::common::Array<Scope>& GetScopes() const;

    CSP_NO_EXPORT ScopesResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode);

private:
    ScopesResult(void*) {};

    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<Scope> Scopes;
};

typedef std::function<void(const ScopesResult& Result)> ScopesResultCallback;

}
