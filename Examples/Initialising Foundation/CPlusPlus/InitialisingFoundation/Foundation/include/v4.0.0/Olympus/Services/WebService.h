#pragma once

#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"

#include <cstdint>
#include <functional>

namespace oly_services
{
class ApiBase;
class ApiResponseBase;
} // namespace oly_services

namespace oly_services
{

/**
 *   @namespace oly_services
 *
 *   This namespace contains wrappers for any Services provided by Magnopus CHS
 *
 */

/// @brief Abstract base class for all CHS web services.
class OLY_API WebService
{
public:
    WebService() { }
    virtual ~WebService() = default;
};

enum class EResultCode : uint8_t
{
    Init,
    InProgress,
    Success,
    Failed
};

class OLY_API ResultBase
{
public:
    ResultBase();
    virtual ~ResultBase() = default;

    OLY_NO_EXPORT virtual void OnProgress(const ApiResponseBase* ApiResponse);
    OLY_NO_EXPORT virtual void OnResponse(const ApiResponseBase* ApiResponse);

    const EResultCode GetResultCode() const;
    const uint16_t GetHttpResultCode() const;
    const oly_common::String& GetResponseBody() const;

    float GetRequestProgress() const;
    float GetResponseProgress() const;

protected:
    ResultBase(oly_services::EResultCode ResCode, uint16_t HttpResCode);
    void SetResult(oly_services::EResultCode ResCode, uint16_t HttpResCode);

private:
    EResultCode Result = EResultCode::Init;
    uint16_t HttpResponseCode = 0;

    float RequestProgress = 0.0f;
    float ResponseProgress = 0.0f;

    oly_common::String ResponseBody;
};

} // namespace oly_services
