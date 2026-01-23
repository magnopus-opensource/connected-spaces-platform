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

#include "CSP/Multiplayer/SpaceEntity.h"

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
    // CreateEntity Tool
    const csp::systems::InvokeRegisteredToolCallback Invoke_CreateEntity_Callback
        = [this](const csp::common::String& ToolCallChainId, const csp::common::String& ToolArguments)
    { return CreateEntity(ToolCallChainId, ToolArguments); };

    RegisterTool("create_entity", Invoke_CreateEntity_Callback);

    // UpdateEntity Tool
    const csp::systems::InvokeRegisteredToolCallback Invoke_UpdateEntity_Callback
        = [this](const csp::common::String& ToolCallChainId, const csp::common::String& ToolArguments)
    { return UpdateEntity(ToolCallChainId, ToolArguments); };

    RegisterTool("update_entity", Invoke_UpdateEntity_Callback);

    // AddComponentToEntity Tool
    const csp::systems::InvokeRegisteredToolCallback Invoke_AddComponentToEntity_Callback
        = [this](const csp::common::String& ToolCallChainId, const csp::common::String& ToolArguments)
    { return AddComponentToEntity(ToolCallChainId, ToolArguments); };

    RegisterTool("add_component_to_entity", Invoke_AddComponentToEntity_Callback);

    // RegisterComponentType Tool
    const csp::systems::InvokeRegisteredToolCallback Invoke_RegisterComponent_Callback
        = [this](const csp::common::String& ToolCallChainId, const csp::common::String& ToolArguments)
    { return RegisterComponent(ToolCallChainId, ToolArguments); };

    RegisterTool("register_component", Invoke_RegisterComponent_Callback);

    // UpdateComponent Tool
    const csp::systems::InvokeRegisteredToolCallback Invoke_UpdateComponent_Callback
        = [this](const csp::common::String& ToolCallChainId, const csp::common::String& ToolArguments)
    { return UpdateComponent(ToolCallChainId, ToolArguments); };

    RegisterTool("update_component", Invoke_UpdateComponent_Callback);


    // Used for the initial hackathon testing.
    //const csp::systems::InvokeRegisteredToolCallback Invoke_GetMeetingInfo_Callback
    //    = [this](const csp::common::String& ToolCallChainId, const csp::common::String& ToolArguments)
    //{
    //    return GetMeetingInfo(ToolCallChainId, ToolArguments);
    //};
    //
    //RegisterTool("get_meeting_info", Invoke_GetMeetingInfo_Callback);
}

void ToolCallExecutor::SetRealtimeEngine(csp::common::IRealtimeEngine* InRealtimeEngine) { RealtimeEngine = InRealtimeEngine; }

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
std::future<csp::common::String> ToolCallExecutor::CreateEntity(const csp::common::String& ToolCallChainId, const csp::common::String& ArgumentsJson)
{
    // {"name":"MainBoard","rotation":[0,0,0,1],"position":[0,0,0]}
    auto Promise = std::make_shared<std::promise<csp::common::String>>();

    CSP_LOG_FORMAT(
        csp::common::LogLevel::Log, "CreateEntity called. ToolCallChainId: %s - Schema: %s", ToolCallChainId.c_str(), ArgumentsJson.c_str());

    rapidjson::Document doc;

    // 1. Parse the string
    if (doc.Parse(ArgumentsJson.c_str()).HasParseError())
    {
        CSP_LOG_ERROR_FORMAT("Failed to parse JSON arguments for CreateEntity Tool Call. ToolCallChainId: %s", ToolCallChainId.c_str());

        Promise->set_value("Error: Failed to parse JSON arguments.");
        return;
    }

    csp::common::String Name;

    // 2. Read 'name' (String)
    if (doc.HasMember("name") && doc["name"].IsString())
    {
        Name = doc["name"].GetString();
    }

    csp::common::Vector3 Position;

    // 3. Read 'position' (Array of Numbers)
    if (doc.HasMember("position") && doc["position"].IsArray())
    {
        const rapidjson::Value& posArray = doc["position"];

        Position.X = posArray[0].GetFloat();
        Position.Y = posArray[1].GetFloat();
        Position.Z = posArray[2].GetFloat();
    }

    csp::common::Vector4 Rotation;

    // 3. Read 'rotation' (Array of Numbers)
    if (doc.HasMember("rotation") && doc["rotation"].IsArray())
    {
        const rapidjson::Value& rotArray = doc["rotation"];

        Rotation.X = rotArray[0].GetFloat();
        Rotation.Y = rotArray[1].GetFloat();
        Rotation.Z = rotArray[2].GetFloat();
        Rotation.W = rotArray[3].GetFloat();
    }

    csp::common::Vector3 Scale;

    // 3. Read 'position' (Array of Numbers)
    if (doc.HasMember("scale") && doc["scale"].IsArray())
    {
        const rapidjson::Value& scaleArray = doc["scale"];

        Scale.X = scaleArray[0].GetFloat();
        Scale.Y = scaleArray[1].GetFloat();
        Scale.Z = scaleArray[2].GetFloat();
    }

    csp::common::String ParentName;

    // 2. Read 'parentId' (String)
    if (doc.HasMember("parentId") && doc["parentId"].IsString())
    {
        ParentName = doc["parentId"].GetString();
    }

    csp::multiplayer::SpaceTransform EntityTransform(Position, Rotation, Scale);

    auto* ParentEntity = RealtimeEngine->FindSpaceEntity(ParentName);

    RealtimeEngine->CreateEntity(Name, EntityTransform, ParentEntity->GetId(),
        [&Promise](csp::multiplayer::SpaceEntity* CreatedEntity)
        {
            csp::common::String Result = "Success";
            csp::common::String EntityId = std::to_string(CreatedEntity->GetId()).c_str();

            rapidjson::Document doc;
            doc.SetObject();
            rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

            doc.AddMember("Result", rapidjson::Value(Result, allocator).Move(), allocator);
            doc.AddMember("EntityId", rapidjson::Value(EntityId, allocator).Move(), allocator);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            csp::common::String ResponseJson = buffer.GetString();

            Promise->set_value(ResponseJson);
        });

    return Promise->get_future();
}

// EntitySchema will contain all the information needed to update an Entity - may only expose the transform for now
// return the Status
std::future<csp::common::String> ToolCallExecutor::UpdateEntity(const csp::common::String& ToolCallChainId, const csp::common::String& ArgumentsJson)
{
    CSP_LOG_FORMAT(
        csp::common::LogLevel::Log, "UpdateEntity called. ToolCallChainId: %s - Schema: %s", ToolCallChainId.c_str(), ArgumentsJson.c_str());

    auto* ParentEntity = RealtimeEngine->FindSpaceEntity("NameFromArgs");
    ParentEntity->SetName("NewNameFromArgs");
    // etc

    auto Promise = std::make_shared<std::promise<csp::common::String>>();

    csp::common::String ResponseJson = "UpdateEntity: " + ArgumentsJson;

    Promise->set_value(ResponseJson);

    return Promise->get_future();
}

// ComponentSchema will contain all the information needed to create the component [EntityId, CompnonentType, InitialPropertyValues]
// return the created ComponentId and Status
std::future<csp::common::String> ToolCallExecutor::AddComponentToEntity(
    const csp::common::String& ToolCallChainId, const csp::common::String& ArgumentsJson)
{
    CSP_LOG_FORMAT(
        csp::common::LogLevel::Log, "AddComponentToEntity called. ToolCallChainId: %s - Schema: %s", ToolCallChainId.c_str(), ArgumentsJson.c_str());

    auto* ParentEntity = RealtimeEngine->FindSpaceEntity("NameFromArgs");
    auto* Component = ParentEntity->AddComponent2("ComponentTypeFromArgs");
    

    auto Promise = std::make_shared<std::promise<csp::common::String>>();

    csp::common::String ResponseJson = "AddComponentToEntity: " + ArgumentsJson;

    Promise->set_value(ResponseJson);

    return Promise->get_future();
}

// ComponentSchema will contain all the information needed to register a new component [CompnonentType, InitialPropertyValues]
// ComponentSchema can be array of components
// return the Status
std::future<csp::common::String> ToolCallExecutor::RegisterComponent(
    const csp::common::String& ToolCallChainId, const csp::common::String& ArgumentsJson)
{
    CSP_LOG_FORMAT(
        csp::common::LogLevel::Log, "RegisterComponentType called. ToolCallChainId: %s - Schema: %s", ToolCallChainId.c_str(), ArgumentsJson.c_str());

    RealtimeEngine->RegisterComponents("Json Schema");

    auto Promise = std::make_shared<std::promise<csp::common::String>>();

    csp::common::String ResponseJson = "RegisterComponent: " + ArgumentsJson;

    Promise->set_value(ResponseJson);

    return Promise->get_future();
}

// ComponentSchema will contain all the information needed to register a new component [CompnonentType, InitialPropertyValues]
// ComponentSchema can be array of components to update
// return the Status
std::future<csp::common::String> ToolCallExecutor::UpdateComponent(
    const csp::common::String& ToolCallChainId, const csp::common::String& ArgumentsJson)
{
    CSP_LOG_FORMAT(
        csp::common::LogLevel::Log, "UpdateComponent called. ToolCallChainId: %s - Schema: %s", ToolCallChainId.c_str(), ArgumentsJson.c_str());

    auto* ParentEntity = RealtimeEngine->FindSpaceEntity("NameFromArgs");
    // loop through all components to find one with the matching name
    //FoundComponent->SetProperty("NameOfProperty", ReplicatedValue_from_json);

    auto Promise = std::make_shared<std::promise<csp::common::String>>();

    csp::common::String ResponseJson = "UpdateComponent: " + ArgumentsJson;

    Promise->set_value(ResponseJson);

    return Promise->get_future();
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
