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
#include "CSP/Common/SharedEnums.h"
#include "CSP/Common/String.h"

#include <cstdint>
#include <functional>

namespace csp::services
{
class ApiBase;
class ApiResponseBase;
} // namespace csp::services

namespace csp::systems
{

/// @brief Base class for a HTTP request result.
class CSP_API ResultBase
{
public:
    /// @brief Constructs an empty result.
    ResultBase();

    /// @brief Virtual destructor.
    virtual ~ResultBase() = default;

    // @brief Equality operator
    bool operator==(const ResultBase& other) const;
    // @brief Inequality operator
    bool operator!=(const ResultBase& other) const;

    /// @brief Called when progress has been updated.
    /// @param ApiResponse const ApiResponseBase* : Response received from the request
    CSP_NO_EXPORT virtual void OnProgress(const services::ApiResponseBase* apiResponse);

    /// @brief Called when a response has been received.
    /// @param ApiResponse const ApiResponseBase* : Response received from the request
    CSP_NO_EXPORT virtual void OnResponse(const services::ApiResponseBase* apiResponse);

    /// @brief Status of this response.
    /// @return EResultCode
    EResultCode GetResultCode() const;

    /// @brief Result of http request.
    /// @return uint16_t
    uint16_t GetHttpResultCode() const;

    /// @brief Body of the response.
    const csp::common::String& GetResponseBody() const;

    /// @brief Percentage of POST/PUT request completion.
    /// @return float
    float GetRequestProgress() const;

    /// @brief Percentage of GET/HEAD response completion.
    /// @return float
    float GetResponseProgress() const;

    /// @brief Get a code representing the failure reason, if relevant.
    /// @return ERequestFailureReason
    ERequestFailureReason GetFailureReason() const;

protected:
    ResultBase(csp::systems::EResultCode resCode, uint16_t httpResCode);
    ResultBase(csp::systems::EResultCode resCode, uint16_t httpResCode, csp::systems::ERequestFailureReason reason);

    void SetResult(csp::systems::EResultCode resCode, uint16_t httpResCode);

    EResultCode m_result = EResultCode::Init;
    uint16_t m_httpResponseCode = 0;

    float m_requestProgress = 0.0f;
    float m_responseProgress = 0.0f;

    csp::common::String m_responseBody;
    ERequestFailureReason m_failureReason;

private:
    ERequestFailureReason ParseErrorCode(const csp::common::String& value);
};

} // namespace csp::systems
