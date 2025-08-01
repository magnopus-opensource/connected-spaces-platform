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
 * @brief This interface decouples systems logic from multiplayer by offering a public interface for authentication functionality, that doesn't rely on any systems functionality. 
 * This is necessary because the WebClient is utilized in both systems and multiplayer contexts.
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
