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

#include "CSP/Common/LoginState.h"

/**
 * @class IAuthContext
 * @brief Represents an object which holds information about the authentication session state,
 * and refreshing the refresh token to keep the session in a valid state.
 *
 * This was created to seperate systems logic from multiplayer
 * by providing a public interface that doesn't rely on any systems types
 * due to the WebClient needing the ability to refresh token, and the web client is used in both
 * systems and multiplayer.
 */

namespace csp::common
{

class IAuthContext
{
public:
    /// @brief Gets information about the state of authentication and information about the current session.
    /// @return csp::common::LoginState&
    virtual const csp::common::LoginState& GetLoginState() const = 0;

    /// @brief Refreshes the current refresh token for the authentication session.
    /// @Paramm Callback std::function<void(bool)> : Function that is called when the token refreshes. True = success.
    CSP_NO_EXPORT virtual void RefreshToken(std::function<void(bool)> Callback) = 0;

    virtual ~IAuthContext() = default;
};
}
