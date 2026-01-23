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

#include "CSP/Common/Optional.h"
#include "CSP/Systems/SystemBase.h"
#include "CSP/Systems/SystemsResult.h"

#include "ToolCallInfo.h"
#include <rapidjson/document.h>

namespace csp::services
{

class ApiBase;

} // namespace csp::services

namespace csp::web
{

class WebClient;

} // namespace csp::web

namespace csp::common
{
class IRealtimeEngine;
} // namespace csp::common

namespace csp::systems
{

class GeminiRequestInfo;
class ToolCallExecutor;

/// @ingroup ToolCalls System
/// @brief Public facing system that allows interfacing with Google Gemini.
/// This system can be used to register tool calls with the Gemini LLM and to requests.
class CSP_API ToolCallsSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */
    CSP_END_IGNORE

public:
    // void RegisterTool(const csp::common::String& ToolName, csp::systems::InvokeRegisteredToolCallback InvokeToolCallback);
    void SetRealtimeEngine(csp::common::IRealtimeEngine* InRealtimeEngine);

    CSP_ASYNC_RESULT void SendRequest(const csp::common::String& RequestURL, const csp::common::String& RequestBody, ToolCallInfoCallback Callback);

    // This method is a hackathon only method. Allows us to read our prompt from a file for easier testing.
    csp::common::String RetreiveUserPrompt(const csp::common::String& PromptFilename, const csp::common::String& GeminiConfigsPath);

    csp::common::String ConstructRequestURL(const csp::common::String& LLMConfigFilename, const csp::common::String& LLMConfigPath);

    csp::common::String ConstructRequestBodyJson(
        const csp::common::String& ToolCallsFilename, const csp::common::String& LLMConfigPath, const csp::common::String& UserPrompt);

    ToolCallExecutor* GetToolCallExecutor() { return ToolCallExecutor; };

private:
    GeminiRequestInfo ConstructGeminiRequestInfo(const csp::common::String& LLMConfigJson);

    ToolCallsSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    ToolCallsSystem(csp::web::WebClient* WebClient, csp::common::LogSystem& LogSystem);
    ~ToolCallsSystem();

    ToolCallExecutor* ToolCallExecutor;
    // csp::common::Map<csp::common::String, csp::systems::InvokeRegisteredToolCallback> RegisteredTools;
};

} // namespace csp::systems
