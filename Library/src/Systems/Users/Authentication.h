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

#pragma once

#include "CSP/Systems/Users/Authentication.h"

namespace csp::systems
{

/// @brief Result structure for a logout state request.
class CSP_API LogoutResult : public NullResult
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    friend class UserSystem;
    CSP_END_IGNORE
    /** @endcond */

private:
    LogoutResult();
    LogoutResult(LoginState* InStatePtr);

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    LoginState* State;
};

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

/// @brief Result url for a tier checkout session request
class CSP_API CheckoutSessionUrlResult : public StringResult
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    friend class UserSystem;
    CSP_END_IGNORE
    /** @endcond */

public:
    CheckoutSessionUrlResult() = default;
    CheckoutSessionUrlResult(void*) {};

private:
    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;
};

/// @brief Result url for a user customer portal request
class CSP_API CustomerPortalUrlResult : public StringResult
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    friend class UserSystem;
    CSP_END_IGNORE
    /** @endcond */

public:
    CustomerPortalUrlResult() = default;
    CustomerPortalUrlResult(void*) {};

private:
    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;
};

} // namespace csp::systems
