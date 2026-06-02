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
#include "CSP/Common/List.h"
#include "CSP/Common/Settings.h"
#include "CSP/Common/SharedEnums.h"
#include "CSP/Common/String.h"

#include <memory>
#include <mutex>

namespace csp::common
{

class LoginStateData;

/// @brief Data structure representing the user login state, including detection of access token expiry
/// @inv All access to the underlying LoginState data happen under a mutex lock.
class CSP_API LoginState
{
public:
    LoginState();
    ~LoginState();
    
    LoginState(const LoginState& OtherState);
    LoginState& operator=(const LoginState& OtherState);

    /// @brief Get a thread-safe copy of the current login state data.
    /// This is a snapshot of the data at the time of the call, and will not update if the underlying state is mutated.
    /// @return A snapshot of the login state data.
    CSP_NO_EXPORT LoginStateData GetSnapshotThreadSafe() const;

    /// @brief Set the login state data. This will replace all existing data in the login state with the new data provided.
    CSP_NO_EXPORT void SetLoginStateData(csp::common::LoginStateData NewData);

    /// @brief Attempt to set the login state to LoginRequested if the current state is LoggedOut.
    /// This will prevent multiple concurrent login attempts through use of a mutex guard.
    /// @return If the state was successfully changed to LoginRequested.
    CSP_NO_EXPORT bool TrySetLoginRequested();

    /// @brief Attempt to set the login state to LogoutRequested if the current state is LoggedIn.
    /// This will prevent multiple concurrent logout attempts through use of a mutex guard.
    /// @return If the state was successfully changed to LogoutRequested.
    CSP_NO_EXPORT bool TrySetLogoutRequested();

    /// @brief Reset the login state after a failed login/logout attempt.
    CSP_NO_EXPORT void ResetResponseLoginState(csp::common::ELoginState LoginState);

    /// @brief Update the access token expiry time.
    CSP_NO_EXPORT void UpdateAccessTokenExpiry(csp::common::String AccessTokenExpiryLength);

    /// @brief Update the refresh token expiry time.
    CSP_NO_EXPORT void UpdateRefreshTokenExpiry(csp::common::String RefreshTokenExpiryLength);

    /// @brief Check if the access token for the login is expired.
    /// @return Is the token expired.
    [[nodiscard]] bool RefreshNeeded() const;

    /// @brief Get the user ID of the currently logged-in user.
    /// @return The user ID, or an empty string if not logged in.
    csp::common::String GetUserId() const;

    /// @brief Get the device ID of the currently logged-in user.
    /// @return The device ID, or an empty string if not logged in.
    csp::common::String GetDeviceId() const;

    /// @brief Get the access token the currently logged-in user.
    /// @return The access token, or an empty string if not logged in.
    csp::common::String GetAccessToken() const;

    /// @brief Get the refresh token the currently logged-in user.
    /// @return The refresh token, or an empty string if not logged in.
    csp::common::String GetRefreshToken() const;

    /// @brief Get the current login state value.
    /// @return The current login state value.
    csp::common::ELoginState GetLoginStateValue() const;

    /// @brief Get the default tenant-wide application settings returned at login.
    /// @return The default application settings.
    csp::common::List<csp::common::ApplicationSettings> GetDefaultApplicationSettings() const;

    /// @brief Get the default user settings returned at login.
    /// @return The default user settings.
    csp::common::List<csp::common::SettingsCollection> GetDefaultSettings() const;

private:
    void CopyStateFrom(const LoginState& OtherState);
    
    std::unique_ptr<LoginStateData> Data;
    CSP_START_IGNORE
    mutable std::mutex LoginStateMutex;
    CSP_END_IGNORE
};

} // namespace csp::common
