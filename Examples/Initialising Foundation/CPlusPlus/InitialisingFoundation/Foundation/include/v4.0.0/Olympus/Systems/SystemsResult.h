#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/Map.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Services/WebService.h"

namespace oly_multiplayer
{
class ConversationSystem;
}

namespace oly_services
{

// TODO: Add support for template friends in wrapper generator and remove usage of these ignore macros
OLY_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
OLY_END_IGNORE

} // namespace oly_services

namespace oly_systems
{

class OLY_API NullResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceSystem;
    friend class SettingsSystem;
    friend class UserSystem;
    friend class oly_multiplayer::ConversationSystem;
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */
public:
    /// @brief Creates an invalid NullResult instance that can be used to notify the user of an error.
    /// @return NullResult : invalid NullResult instance
    OLY_NO_EXPORT static NullResult Invalid();

protected:
    NullResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};
    OLY_NO_EXPORT NullResult(const oly_services::ResultBase& InResult)
        : oly_services::ResultBase(InResult.GetResultCode(), InResult.GetHttpResultCode()) {};
    NullResult() = default;
    NullResult(void*) {};
};

class OLY_API BooleanResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SettingsSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    [[nodiscard]] bool GetValue() const;

private:
    BooleanResult() = default;
    BooleanResult(void*) {};
    BooleanResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};

    void SetValue(bool InValue);

    bool Value = false;
};

class OLY_API StringResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SettingsSystem;
    friend class UserSystem;
    friend class oly_multiplayer::ConversationSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    [[nodiscard]] const oly_common::String& GetValue() const;

    /// @brief Creates an invalid StringResult instance that can be used to notify the user of an error.
    /// @return StringResult : invalid StringResult instance
    OLY_NO_EXPORT static StringResult Invalid();

private:
    StringResult() = default;
    StringResult(void*) {};
    StringResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};

    void SetValue(const oly_common::String& InValue);

    oly_common::String Value;
};

class OLY_API StringArrayResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SettingsSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    [[nodiscard]] const oly_common::Array<oly_common::String>& GetValue() const;

private:
    StringArrayResult() = default;
    StringArrayResult(void*) {};
    StringArrayResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};

    void SetValue(const oly_common::Array<oly_common::String>& InValue);

    oly_common::Array<oly_common::String> Value;
};

class OLY_API UInt64Result : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    [[nodiscard]] uint64_t GetValue() const;

private:
    UInt64Result() = default;
    UInt64Result(void*) {};
    UInt64Result(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};

    void SetValue(uint64_t InValue);

    uint64_t Value = 0UL;
};

class OLY_API HTTPHeadersResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    OLY_NO_EXPORT void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    [[nodiscard]] const oly_common::Map<oly_common::String, oly_common::String>& GetValue() const;

private:
    HTTPHeadersResult() = default;
    HTTPHeadersResult(void*) {};
    HTTPHeadersResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};

    oly_common::Map<oly_common::String, oly_common::String> Value;
};

typedef std::function<void(const NullResult& Result)> NullResultCallback;
typedef std::function<void(const BooleanResult& Result)> BooleanResultCallback;
typedef std::function<void(const StringResult& Result)> StringResultCallback;
typedef std::function<void(const StringArrayResult& Result)> StringArrayResultCallback;
typedef std::function<void(const UInt64Result& Result)> UInt64ResultCallback;
typedef std::function<void(const HTTPHeadersResult& Result)> HTTPHeadersResultCallback;

} // namespace oly_systems
