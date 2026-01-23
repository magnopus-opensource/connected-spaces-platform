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

#include "CSP/Systems/ToolCalls/ToolCallsSystem.h"
#include "CSP/Systems/ToolCalls/ToolCallExecutor.h"

#include "CallHelpers.h"
#include "Services/ApiBase/ApiBase.h"
#include "Systems/ResultHelpers.h"
#include "Debug/Logging.h"
#include <Common/Logger.h>
#include <fstream>
// #include <rapidjson/document.h>
#include <filesystem>
#include <sstream>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "Common/Web/HttpResponse.h"
// #include <CSP/Systems/ToolCalls/ToolCallInfo.h>

// using ResponseHandlerPtr = csp::services::ApiResponseHandlerBase*;

namespace
{

csp::common::String Serialize(rapidjson::Document& doc)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}

// Add a helper to trim whitespace
csp::common::String Trim(const csp::common::String& InString)
{
    std::string s = InString.c_str();
    s.erase(s.find_last_not_of(" \n\r\t") + 1);
    s.erase(0, s.find_first_not_of(" \n\r\t"));
    return csp::common::String(s.c_str());
}

csp::common::String ReadFileToString(const csp::common::String& FilePath)
{
    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "Reading from filepath: %s", FilePath.c_str());

    std::ifstream Stream { FilePath.c_str() };

    if (!Stream)
    {
        CSP_LOG(csp::common::LogLevel::Warning, "Stream at specified path could not be read.");

        return "";
    }

    std::stringstream SStream;
    SStream << Stream.rdbuf();

    return SStream.str().c_str();
}

} // namespace

namespace csp::systems
{

/// @brief Info required for a Gemini request
class CSP_API GeminiRequestInfo
{
public:
    csp::common::String APIKey;
    csp::common::String RequestURL;
};

ToolCallsSystem::ToolCallsSystem()
    : SystemBase(nullptr, nullptr, nullptr)
{
}

ToolCallsSystem::ToolCallsSystem(web::WebClient* WebClient, csp::common::LogSystem& LogSystem)
    : SystemBase(WebClient, nullptr, &LogSystem)
{
    ToolCallExecutor = new csp::systems::ToolCallExecutor(this);

    csp::systems::ToolResponseCallbackHandler ResponseCallback = [this](const csp::common::String& ToolCallCHainId, const csp::common::Array<csp::common::String>& AccumulatedResponses)
    {
        // Do stuff with the response.
        CSP_LOG_FORMAT(
            csp::common::LogLevel::Log, "call tool chain completed: %s - Num responses: %i", ToolCallCHainId.c_str(), AccumulatedResponses.Size());
    };

    ToolCallExecutor->SetToolCallsCompletedResponseCallback(ResponseCallback);
}

ToolCallsSystem::~ToolCallsSystem() { delete (ToolCallExecutor); }

GeminiRequestInfo ToolCallsSystem::ConstructGeminiRequestInfo(const csp::common::String& LLMConfigJson)
{
    GeminiRequestInfo GeminiRequest;

    rapidjson::Document Doc;
    Doc.Parse(LLMConfigJson);

    if (!Doc.HasParseError())
    {
        if (Doc.HasMember("gemini_url") && Doc["gemini_url"].IsString() && (Doc.HasMember("api_key") && Doc["api_key"].IsString()))
        {
            GeminiRequest.RequestURL = Trim(Doc["gemini_url"].GetString());
            GeminiRequest.APIKey = Trim(Doc["api_key"].GetString());
        }
        else
        {
            CSP_LOG(csp::common::LogLevel::Error, "Json does not contain the required data.");
        }
    }
    else
    {
        CSP_LOG(csp::common::LogLevel::Error, "Error parsing rapidjson LLMConfig document.");
    }

    return GeminiRequest;
}

void ToolCallsSystem::SetRealtimeEngine(csp::common::IRealtimeEngine* InRealtimeEngine)
{
   ToolCallExecutor->SetRealtimeEngine(InRealtimeEngine);
}

void ToolCallsSystem::SendRequest(const csp::common::String& RequestURL, const csp::common::String& RequestBody, ToolCallInfoCallback Callback)
{
    const ToolCallInfoCallback GetToolCallInfoCallback = [this, Callback](const ToolCallInfoResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Warning, "Gemini 400 Error: %s", Result.GetResponseBody().c_str());

            ToolCallInfoResult res(csp::systems::EResultCode::Failed, Result.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, res);
            return;
        }

        if (Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            const auto& RequestedToolCallInfos = Result.GetToolCallsInfo();

            csp::common::String ResponseId = RequestedToolCallInfos.ResponseId;

            CSP_LOG_FORMAT(csp::common::LogLevel::Log, "Gemini function call Response Id: %s", ResponseId.c_str());

            for (auto& ToolCall : RequestedToolCallInfos.ToolCalls)
            {
                csp::common::String FunctionName = ToolCall.FunctionName;
                csp::common::String FunctionArgs = ToolCall.Arguments;

                CSP_LOG_FORMAT(csp::common::LogLevel::Log, "Gemini function call: %s - args: $s", FunctionName.c_str(), FunctionArgs.c_str());
            }

            // Call logic to execute the tool calls - RequestedToolCallsInfo
            //csp::systems::ToolCallExecutor* Executor = csp::systems::ToolCallsSystem->GetToolCallExecutor();

            this->ToolCallExecutor->InvokeToolCalls(RequestedToolCallInfos);
        }

        INVOKE_IF_NOT_NULL(Callback, Result);
    };

    csp::web::HttpPayload Payload;
    Payload.AddHeader("Content-Type", "application/json");
    Payload.SetContent(RequestBody.c_str(), RequestBody.Length());

    csp::web::Uri GetUri(RequestURL);

    // void, csp::services::NullDto
    auto* GeminiHandler = new csp::services::ApiResponseHandler<ToolCallInfoCallback, ToolCallInfoResult, ToolCallsSystem, csp::services::NullDto>(
        GetToolCallInfoCallback, this, csp::web::EResponseCodes::ResponseOK);

    static csp::common::CancellationToken Token;

    WebClient->SendRequest(csp::web::ERequestVerb::Post, GetUri, Payload, GeminiHandler, Token);
}

csp::common::String ToolCallsSystem::RetreiveUserPrompt(const csp::common::String& PromptFilename, const csp::common::String& GeminiConfigsPath)
{
    std::filesystem::path FullPromptPath = std::filesystem::path(GeminiConfigsPath.c_str()) / PromptFilename.c_str();

    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "Reading User Prompt from filepath: %s", FullPromptPath.u8string().c_str());

    csp::common::String UserPrompt = ReadFileToString(FullPromptPath.u8string().c_str());

    return Trim(UserPrompt);
}

csp::common::String ToolCallsSystem::ConstructRequestURL(const csp::common::String& LLMConfigFilename, const csp::common::String& LLMConfigPath)
{
    std::filesystem::path FullLLMConfigPath = std::filesystem::path(LLMConfigPath.c_str()) / LLMConfigFilename.c_str();

    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "Reading Request URL and API key from filepath: %s", FullLLMConfigPath.u8string().c_str());

    csp::common::String LLMConfigJson = ReadFileToString(FullLLMConfigPath.u8string().c_str());
    GeminiRequestInfo RequestInfo = ConstructGeminiRequestInfo(LLMConfigJson);

    csp::common::String APIKey = RequestInfo.APIKey;
    if (APIKey.IsEmpty())
    {
        CSP_LOG(csp::common::LogLevel::Warning, "Error retreiving API Key.");

        return "";
    }

    csp::common::String GeminiRequestURL = RequestInfo.RequestURL;
    if (GeminiRequestURL.IsEmpty())
    {
        CSP_LOG(csp::common::LogLevel::Warning, "Error retreiving Request URL.");

        return "";
    }

    csp::common::String CompleteRequestUrl = GeminiRequestURL + APIKey;

    return CompleteRequestUrl;
}

csp::common::String ToolCallsSystem::ConstructRequestBodyJson(
    const csp::common::String& ToolCallsFilename, const csp::common::String& LLMConfigPath, const csp::common::String& UserPrompt)
{
    std::filesystem::path ToolCallsPath = std::filesystem::path(LLMConfigPath.c_str()) / ToolCallsFilename.c_str();

    /*
     * Read Tool Calls from file
     */
    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "Reading Tool Calls from filepath: %s", ToolCallsPath.u8string().c_str());

    rapidjson::Document ToolCallsDoc;
    csp::common::String ToolCallsJson = ReadFileToString(ToolCallsPath.u8string().c_str());

    ToolCallsDoc.Parse(ToolCallsJson);

    if (ToolCallsDoc.HasParseError())
    {
        CSP_LOG(csp::common::LogLevel::Warning, "Error parsing ToolCalls RapidJson document.");

        return "";
    }

    /*
     * Construct the request body
     */
    // Create the main request document
    rapidjson::Document RequestDoc;
    RequestDoc.SetObject();
    auto& allocator = RequestDoc.GetAllocator();

    // Construct the request "contents"
    rapidjson::Value Contents(rapidjson::kArrayType); // Construct Conetents array
    rapidjson::Value ContentObj(rapidjson::kObjectType); // Construct Content object
    ContentObj.AddMember("role", "user", allocator); // Add the role member to the Content object

    rapidjson::Value Parts(rapidjson::kArrayType); // Construct Parts array
    rapidjson::Value PromptObj(rapidjson::kObjectType); // Construct Prompt object
    //PromptObj.AddMember(
    //    "text", "Can you get meeting info for the meeting John organised?", allocator); // Add the Prompt text member to the Prompt object
    PromptObj.AddMember("text", rapidjson::Value(UserPrompt.c_str(), allocator), allocator); // Add the Prompt text member to the Prompt object
    Parts.PushBack(PromptObj, allocator); // Add the Prompt object to the Parts array

    ContentObj.AddMember("parts", Parts, allocator); // Add the Parts array to the Content object
    Contents.PushBack(ContentObj, allocator); // Add the Content object to the Contents array
    RequestDoc.AddMember("contents", Contents, allocator); // Add the Contents array to the main request document

    // Construct request "tools"
    // Gemini structure: "tools": [ { "function_declarations": [ ... ] } ]
    rapidjson::Value ToolsArray(rapidjson::kArrayType); // Construct thw Tools array
    rapidjson::Value ToolContainer(rapidjson::kObjectType); // Construct the Tool container object

    rapidjson::Value FunctionDeclarations; // Construct the Function Declarations value
    FunctionDeclarations.CopyFrom(ToolCallsDoc, allocator); // Copy the ToolCalls document into Function Declarations

    ToolContainer.AddMember("function_declarations", FunctionDeclarations, allocator); // Add Function Declarations to the Tool container
    ToolsArray.PushBack(ToolContainer, allocator); // Add the Tool container to the Tools array
    RequestDoc.AddMember("tools", ToolsArray, allocator); // Add the Tools array to the main request document

    // Serialize the main request document to a JSON string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    RequestDoc.Accept(writer);

    return buffer.GetString();
}

} // namespace csp::systems
