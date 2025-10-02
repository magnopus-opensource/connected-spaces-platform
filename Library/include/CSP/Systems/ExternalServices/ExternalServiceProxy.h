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

#include "CSP/Systems/SystemsResult.h"
#include "CSP/Systems/WebService.h"

namespace csp::systems
{

/// @brief @brief Data class used to contain information requesting a user token
class CSP_API AgoraUserTokenResult : public StringResult
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    friend class UserSystem;
    CSP_END_IGNORE
    /** @endcond */

private:
    AgoraUserTokenResult() = default;
    AgoraUserTokenResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;
};

/// @brief @brief Data class used to contain information posting a service proxy
class CSP_API PostServiceProxyResult : public StringResult
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    friend class UserSystem;
    CSP_END_IGNORE
    /** @endcond */

private:
    PostServiceProxyResult() = default;
    PostServiceProxyResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;
};

} // namespace csp::systems
