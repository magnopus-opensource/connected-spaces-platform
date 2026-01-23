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


#include "ToolCallInfo.h"
#include <rapidjson/document.h>
#include "CSP/Systems/SystemsResult.h"
#include <future>

namespace csp::systems
{

class ToolCallsSystem;


//typedef std::function<void(const csp::common::String& ToolCallChainId, const csp::common::String& ToolArguments)> InvokeRegisteredToolCallback;
typedef std::function<std::future<csp::common::String>(const csp::common::String&, const csp::common::String&)> InvokeRegisteredToolCallback;

//typedef std::function<void(const csp::common::String&, const csp::common::String&)> ToolResponseCallbackHandler;
typedef std::function<void(const csp::common::String&, const csp::common::Array<csp::common::String>&)> ToolResponseCallbackHandler;

/// @ingroup ToolCalls System
/// @brief The Tools that can be called by Gemini
class CSP_API ToolCallExecutor
{

public:
    ToolCallExecutor();
    ToolCallExecutor(ToolCallsSystem* InToolCallsSystem);

    CSP_EVENT void SetToolCallsCompletedResponseCallback(ToolResponseCallbackHandler ResponseCallback);

    

    void InvokeToolCalls(const csp::systems::RequestedToolCalls& RequestedToolCallInfos);

    

private:
    //void OnToolCallChainElementCompleted(
    //    const csp::common::String& ToolCallChainId, const csp::common::String& ToolName, const csp::common::String& ToolResponse);
    void RegisterTool(const csp::common::String& ToolName, csp::systems::InvokeRegisteredToolCallback InvokeToolCallback);

    // This was an test tool used at the start of the hackathon
    std::future<csp::common::String> GetMeetingInfo(const csp::common::String& ToolCallChainId, const csp::common::String& ArgumentsJson);

    /*
    * Tools Calls being exposed to Gemini
    */

    // EntitySchema will contain all the information needed to create the entity [name, transform, parentId]
    // return the created EntityId and Status
    std::future<csp::common::String> CreateEntity(const csp::common::String& ToolCallChainId, const csp::common::String& ArgumentsJson);

    // EntitySchema will contain all the information needed to update an Entity - may only expose the transform for now
    // return the Status
    std::future<csp::common::String> UpdateEntity(const csp::common::String& ToolCallChainId, const csp::common::String& ArgumentsJson);

    // ComponentSchema will contain all the information needed to create the component [EntityId, CompnonentType, InitialPropertyValues]
    // return the created ComponentId and Status
    std::future<csp::common::String> AddComponentToEntity(const csp::common::String& ToolCallChainId, const csp::common::String& ArgumentsJson);

    // ComponentSchema will contain all the information needed to register a new component [CompnonentType, InitialPropertyValues]
    // ComponentSchema can be array of components
    // return the Status
    std::future<csp::common::String> RegisterComponent(const csp::common::String& ToolCallChainId, const csp::common::String& ArgumentsJson);

    // ComponentSchema will contain all the information needed to register a new component [CompnonentType, InitialPropertyValues]
    // ComponentSchema can be array of components to update
    // return the Status
    std::future<csp::common::String> UpdateComponent(const csp::common::String& ToolCallChainId, const csp::common::String& ArgumentsJson);

    ToolCallsSystem* ToolCallsSystem;
    ToolResponseCallbackHandler ToolResponseCallback;
    csp::common::Map<csp::common::String, csp::systems::InvokeRegisteredToolCallback> RegisteredTools;
};

} // namespace csp::systems
