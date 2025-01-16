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

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Variant.h"
#include "CSP/Systems/WebService.h"
#include "CSP/Web/HTTPResponseCodes.h"

#include <functional>

namespace csp::services
{

class ApiResponseBase;

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

/// @ingroup Settings System
/// @brief Data representation of a Settings collection which maps to a UserService::Settings.
class CSP_API SettingsCollection
{
public:
    SettingsCollection() = default;

    csp::common::String UserId;
    csp::common::String Context;
    csp::common::Map<csp::common::String, csp::common::String> Settings;
};

/// @ingroup Settings System
/// @brief Data class used to contain information when creating a Settings collection.
class CSP_API SettingsCollectionResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the settings collection result.
    /// @return SettingsCollection : const ref of settings collection class
    const SettingsCollection& GetSettingsCollection() const;

private:
    SettingsCollectionResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    SettingsCollection SettingsCollection;
};

/// @brief Used to specify the type of the user's avatar
enum class AvatarType
{
    None,
    Premade,
    ReadyPlayerMe,
    Custom,
};

/// @brief A result handler that is used to notify a user of an error while passing a String value.
class CSP_API AvatarInfoResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SettingsSystem;
    /** @endcond */

public:
    /// @brief A getter which returns the String passed via the result.
    [[nodiscard]] AvatarType GetAvatarType() const;
    [[nodiscard]] const csp::common::String& GetAvatarIdentifier() const;

    CSP_NO_EXPORT AvatarInfoResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode)
        , Type(AvatarType::None) {};

private:
    AvatarInfoResult()
        : Type(AvatarType::None) {};
    AvatarInfoResult(void*)
        : Type(AvatarType::None) {};

    void SetAvatarType(AvatarType InValue);
    void SetAvatarIdentifier(const csp::common::String& InValue);

    AvatarType Type;
    csp::common::String Identifier;
};

/// @brief Callback containing Settings collection.
/// @param Result SettingsCollectionResult : result class
typedef std::function<void(const SettingsCollectionResult& Result)> SettingsResultCallback;

/// @brief Callback containing Avatar info.
/// @param Result AvatarInfoResult : result class
typedef std::function<void(const AvatarInfoResult& Result)> AvatarInfoResultCallback;

} // namespace csp::systems
