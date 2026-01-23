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

#include "CSP/Common/String.h"
#include "CSP/Common/Array.h"
#include "CSP/Systems/WebService.h"

#include <rapidjson/document.h>
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace csp::services
{

class ApiResponseBase;

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

/// @brief Represents a ToolCall Info
class CSP_API ToolCallInfo
{
public:
    csp::common::String FunctionName;
    csp::common::String Arguments;
};

/// @brief Represents a collection of individual Tool Calls
class CSP_API RequestedToolCalls
{
public:
    csp::common::String ResponseId;
    csp::common::Array<ToolCallInfo> ToolCalls;
};

/// @ingroup CSPFoundation
/// @brief Data class used to contain information when a Response is received from Gemini LLM
class CSP_API ToolCallInfoResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    friend class CSPFoundation;
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Will return info for the ToolCall.
    /// @return ToolCallInfo : the ToolCall Info
    RequestedToolCalls& GetToolCallsInfo();

    const RequestedToolCalls& GetToolCallsInfo() const;

    CSP_NO_EXPORT ToolCallInfoResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) { };

    ToolCallInfoResult() = default;

private:
    ToolCallInfoResult(void*) { };

    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    RequestedToolCalls RequestedToolCallsInfo;
};

typedef std::function<void(const ToolCallInfoResult& Result)> ToolCallInfoCallback;

} // namespace csp::systems
