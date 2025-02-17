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
#include "CSP/Systems/WebService.h"
#include "CSP/Web/HTTPResponseCodes.h"

namespace csp::multiplayer
{

class ConversationSystem;

}

namespace csp::services
{

// TODO: Add support for template friends in wrapper generator and remove usage of these ignore macros
CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

/// @brief A result handler that is used to notify a user of an error.
class CSP_API NullResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceSystem;
    friend class SettingsSystem;
    friend class UserSystem;
    friend class csp::multiplayer::ConversationSystem;
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    CSP_NO_EXPORT NullResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};
    CSP_NO_EXPORT NullResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode, csp::systems::ERequestFailureReason Reason)
        : csp::systems::ResultBase(ResCode, HttpResCode, Reason) {};
    CSP_NO_EXPORT NullResult(csp::systems::EResultCode ResCode, csp::web::EResponseCodes HttpResCode)
        : csp::systems::ResultBase(ResCode, static_cast<std::underlying_type<csp::web::EResponseCodes>::type>(HttpResCode)) {};
    CSP_NO_EXPORT NullResult(csp::systems::EResultCode ResCode, csp::web::EResponseCodes HttpResCode, csp::systems::ERequestFailureReason Reason)
        : csp::systems::ResultBase(ResCode, static_cast<std::underlying_type<csp::web::EResponseCodes>::type>(HttpResCode), Reason) {};

protected:
    CSP_NO_EXPORT NullResult(const csp::systems::ResultBase& InResult)
        : csp::systems::ResultBase(InResult.GetResultCode(), InResult.GetHttpResultCode()) {};
    NullResult() = default;
    NullResult(void*) {};
};

/// @brief A result handler that is used to notify a user of an error while passing a boolean value.
class CSP_API BooleanResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SettingsSystem;
    friend class MaintenanceSystem;
    friend class SpaceSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief A getter which returns the bool passed via the result.
    [[nodiscard]] bool GetValue() const;

private:
    BooleanResult() = default;
    BooleanResult(void*) {};
    BooleanResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

    void SetValue(bool InValue);

    bool Value = false;
};

/// @brief A result handler that is used to notify a user of an error while passing a String value.
class CSP_API StringResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SettingsSystem;
    friend class UserSystem;
    friend class csp::multiplayer::ConversationSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief A getter which returns the String passed via the result.
    [[nodiscard]] const csp::common::String& GetValue() const;

    CSP_NO_EXPORT StringResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

protected:
    StringResult() = default;
    StringResult(void*) {};

    void SetValue(const csp::common::String& InValue);

private:
    csp::common::String Value;
};

/// @brief A result handler that is used to notify a user of an error while passing a StringArray value.
class CSP_API StringArrayResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SettingsSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief A getter which returns the StringArray passed via the result.
    [[nodiscard]] const csp::common::Array<csp::common::String>& GetValue() const;

private:
    StringArrayResult() = default;
    StringArrayResult(void*) {};
    StringArrayResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

    void SetValue(const csp::common::Array<csp::common::String>& InValue);

    csp::common::Array<csp::common::String> Value;
};

/// @brief A result handler that is used to notify a user of an error while passing a uint64_t value.
class CSP_API UInt64Result : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief A getter which returns the uint64_t passed via the result.
    [[nodiscard]] uint64_t GetValue() const;

private:
    UInt64Result() = default;
    UInt64Result(void*) {};
    UInt64Result(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

    void SetValue(uint64_t InValue);

    uint64_t Value = 0UL;
};

/// @brief A result handler that is used to notify a user of an error while providing an event for a callback response, in addition to
/// passing a Map of Strings representing the HTTP Responses.
class CSP_API HTTPHeadersResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Event function used to act upon a HTTP response.
    /// @param ApiResponse : An ApiResponseBase instance used for translating response calls.
    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    /// @brief A getter which returns the map of strings representing HTTP responses passed via the result.
    [[nodiscard]] const csp::common::Map<csp::common::String, csp::common::String>& GetValue() const;

private:
    HTTPHeadersResult() = default;
    HTTPHeadersResult(void*) {};
    HTTPHeadersResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

    csp::common::Map<csp::common::String, csp::common::String> Value;
};

typedef std::function<void(const NullResult& Result)> NullResultCallback;
typedef std::function<void(const BooleanResult& Result)> BooleanResultCallback;
typedef std::function<void(const StringResult& Result)> StringResultCallback;
typedef std::function<void(const StringArrayResult& Result)> StringArrayResultCallback;
typedef std::function<void(const UInt64Result& Result)> UInt64ResultCallback;
typedef std::function<void(const HTTPHeadersResult& Result)> HTTPHeadersResultCallback;

} // namespace csp::systems
