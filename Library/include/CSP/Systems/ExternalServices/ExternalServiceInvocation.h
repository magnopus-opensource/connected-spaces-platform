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

/// @brief Data class used to contain information relating to invocations via the external service proxy system.
class CSP_API ExternalServiceInvocationResult : public StringResult
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    friend class UserSystem;
    CSP_END_IGNORE
    /** @endcond */

protected:
    ExternalServiceInvocationResult() = default;
    ExternalServiceInvocationResult(void*) {};

    virtual void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;
};

/// @brief A specialisation of the result data class that handles respnses from the external service proxy system.
class CSP_API GetAgoraTokenResult : public ExternalServiceInvocationResult
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    friend class UserSystem;
    CSP_END_IGNORE
    /** @endcond */

protected:
    GetAgoraTokenResult() = default;
    GetAgoraTokenResult(void*) {};

    virtual void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;
};

} // namespace csp::systems
