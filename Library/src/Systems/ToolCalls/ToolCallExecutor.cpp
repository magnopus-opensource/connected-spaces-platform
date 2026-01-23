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

#include "CSP/Systems/ToolCalls/ToolCallExecutor.h"
#include "CSP/Systems/ToolCalls/ToolCallsSystem.h"

#include "CallHelpers.h"
#include "Debug/Logging.h"
#include "Services/ApiBase/ApiBase.h"
#include "Systems/ResultHelpers.h"
#include <Common/Logger.h>
#include <fstream>
// #include <rapidjson/document.h>
#include <filesystem>
#include <sstream>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "CSP/Common/Array.h"
#include "CSP/Systems/SystemsResult.h"
#include "Common/Web/HttpResponse.h"

#include <future>

namespace csp::systems
{
ToolCallExecutor::ToolCallExecutor()
    : ToolCallsSystem(nullptr)
{
}

ToolCallExecutor::ToolCallExecutor(csp::systems::ToolCallsSystem* InToolCallsSystem)
    : ToolCallsSystem(InToolCallsSystem)
{
    const csp::systems::InvokeRegisteredToolCallback Invoke_GetMeetingInfo_Callback
        = [this](const csp::common::String& ToolCallChainId, const csp::common::String& ToolArguments)
    {
        return GetMeetingInfo(ToolCallChainId, ToolArguments);
    };
    
    RegisterTool("get_meeting_info", Invoke_GetMeetingInfo_Callback);
}

void ToolCallExecutor::SetToolCallsCompletedResponseCallback(ToolResponseCallbackHandler ResponseCallback)
{
    ToolResponseCallback = std::move(ResponseCallback);
}

void ToolCallExecutor::RegisterTool(const csp::common::String& ToolName, csp::systems::InvokeRegisteredToolCallback InvokeToolCallback)
{
    RegisteredTools[ToolName] = InvokeToolCallback;
}

void ToolCallExecutor::InvokeToolCalls(const csp::systems::RequestedToolCalls& RequestedToolCallInfos)
{
    // Register the tool call chain, perhaps with number of calls in chain.
    csp::common::Array<csp::common::String> AccumulatedResponses = csp::common::Array<csp::common::String>(RequestedToolCallInfos.ToolCalls.Size());

    for (size_t i = 0; i < RequestedToolCallInfos.ToolCalls.Size(); ++i)
    {
        /*
        1. Check if tool is registered.
        2. If registered, invoke the tool with the json arguments.
        */
        const auto& ToolCall = RequestedToolCallInfos.ToolCalls[i];

        if (RegisteredTools.HasKey(ToolCall.FunctionName))
        {
            //RegisteredTools[RequestedToolCallInfos.ToolCalls[i].FunctionName](
            //    RequestedToolCallInfos.ResponseId, RequestedToolCallInfos.ToolCalls[i].Arguments);

            std::future<csp::common::String> ToolFuture
                = RegisteredTools[ToolCall.FunctionName](RequestedToolCallInfos.ResponseId, ToolCall.Arguments);

            csp::common::String Response = ToolFuture.get();

            // 4. Store the result
            AccumulatedResponses[i] = Response;
            
        }
        else
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Warning, "Tool not registered: %s.", RequestedToolCallInfos.ToolCalls[i].FunctionName.c_str());
        }
    }

    // Once the loop finishes, all tools are done.
    // Send the full collection back via your callback.
    if (ToolResponseCallback)
    {
        // You may need to concatenate these into one large JSON array for Gemini
        ToolResponseCallback(RequestedToolCallInfos.ResponseId, AccumulatedResponses);
    }
}

/*
 * Start of Tools Calls being exposed to Gemini
 */

// EntitySchema will contain all the information needed to create the entity [name, transform, parentId]
// return the created EntityId and Status
std::future<csp::common::String> ToolCallExecutor::CreateEntity(const csp::common::String& EntitySchema)
{
    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "CreateEntity called with schema: %s", EntitySchema.c_str());
}

// EntitySchema will contain all the information needed to update an Entity - may only expose the transform for now
// return the Status
std::future<csp::common::String> ToolCallExecutor::UpdateEntity(const csp::common::String& EntitySchema)
{
    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "UpdateEntity called with schema: %s", EntitySchema.c_str());
}

// ComponentSchema will contain all the information needed to create the component [EntityId, CompnonentType, InitialPropertyValues]
// return the created ComponentId and Status
std::future<csp::common::String> ToolCallExecutor::AddComponentToEntity(const csp::common::String& ComponentSchema)
{
    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "AddComponentToEntity called with schema: %s", ComponentSchema.c_str());
}

// ComponentSchema will contain all the information needed to register a new component [CompnonentType, InitialPropertyValues]
// ComponentSchema can be array of components
// return the Status
std::future<csp::common::String> ToolCallExecutor::RegisterComponentType(const csp::common::String& ComponentSchema)
{
    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "RegisterComponentType called with schema: %s", ComponentSchema.c_str());
}

// ComponentSchema will contain all the information needed to register a new component [CompnonentType, InitialPropertyValues]
// ComponentSchema can be array of components to update
// return the Status
std::future<csp::common::String> ToolCallExecutor::UpdateComponent(const csp::common::String& ComponentSchema)
{
    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "UpdateComponent called with schema: %s", ComponentSchema.c_str());
}

/*
 * End of Tools Calls being exposed to Gemini
 */

// This is a test tool used at the start of the hackathon
std::future<csp::common::String> ToolCallExecutor::GetMeetingInfo(
    const csp::common::String& ToolCallChainId, const csp::common::String& ArgumentsJson)
{
    /*
     * 1. Extract properties and their values from ArgumentsJson.
     * 2. Call internal method/s passing the properties.
     * 3. Construct a response json string.
     * 4. Return the response json string via the callback.
     */

    // Use std::promise to return the value asynchronously
    auto Promise = std::make_shared<std::promise<csp::common::String>>();

    CSP_LOG_FORMAT(csp::common::LogLevel::Warning, "GetMeetingInfo called. ToolChainId: %s - Arguments: %s", ToolCallChainId.c_str(), ArgumentsJson.c_str());

    csp::common::String ResponseJson = "{\"role\": \"function\", \"parts\": [{\"functionResponse\": {\"name\": \"get_meeting_info\", \"response\": { "
                                       "\"time\": \"2:00 PM\", \"location\": \"Room 302\" }}}]}";

    // We need to be accumulating the responses for each tool call and sending them back to Gemini.

    //ToolResponseCallback("get_meeting_info", ResponseJson);
    
    // Set the value so the .get() call in InvokeToolCalls can unblock
    Promise->set_value(ResponseJson);

    return Promise->get_future();
}

//void ToolCallExecutor::OnToolCallChainElementCompleted(
//    const csp::common::String& ToolCallChainId, const csp::common::String& ToolName, const csp::common::String& ToolResponse)
//{
//    
//}

} // namespace csp::systems
