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
/*
 * GraphQLApi.h
 *
 *
 */

#pragma once
#include "CSP/Systems/WebService.h"

namespace csp::services
{

class ApiResponseBase;

} // namespace csp::services

namespace csp::systems
{

/// @ingroup GraphQL System
/// @brief Data class used to contain information when a Response is received from GraphQL

class CSP_API GraphQLResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    friend class GraphQLSystem;
    CSP_END_IGNORE
    /** @endcond */
public:
    GraphQLResult() {};
    GraphQLResult(void*) {};

    /// @brief Retrieves response data from the GraphQL Server
    /// @return String : Data String as described above
    [[nodiscard]] const csp::common::String& GetResponse();

private:
    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;
    csp::common::String GraphQLResponse;
};
// callback signatures
typedef std::function<void(GraphQLResult& Result)> GraphQLReceivedCallback;
} // namespace csp::systems
