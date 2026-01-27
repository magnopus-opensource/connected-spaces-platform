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

#include "CSP/Systems/ToolCalls/ToolCallInfo.h"

#include "Services/ApiBase/ApiBase.h"

#include "Debug/Logging.h"
#include <Common/Logger.h>

namespace
{
void WriteGeminiResponseToDisk(csp::common::String ResponseContent)
{
    csp::common::String OutputFilename = "GeminiFunctionCallResponse.json";

    const char* userProfile = std::getenv("USERPROFILE");
    if (!userProfile)
    {
        CSP_LOG(csp::common::LogLevel::Warning, "Could not retrieve use profile.");
    }
    std::filesystem::path GeminiConfigsPath = std::filesystem::path(userProfile) / "Desktop" / "GeminiConfigs";
    std::filesystem::path ResponseOutputPath = std::filesystem::path(GeminiConfigsPath.c_str()) / OutputFilename.c_str();

    try
    {
        // Ensure the directory exists (creates folders if they don't exist)
        std::filesystem::create_directories(ResponseOutputPath.parent_path());

        // Open the file stream
        std::ofstream OutFile(ResponseOutputPath, std::ios::out | std::ios::trunc);

        if (OutFile.is_open())
        {
            OutFile << ResponseContent.c_str();
            OutFile.close();

            CSP_LOG_FORMAT(csp::common::LogLevel::Log, "Successfully wrote Gemini response to: %s", ResponseOutputPath.u8string().c_str());
        }
        else
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Warning, "Failed to open file for writing: %s", ResponseOutputPath.u8string().c_str());
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        CSP_LOG_FORMAT(csp::common::LogLevel::Warning, "Filesystem error: %s", e.what());
    }
}

} // namespace

namespace csp::systems
{

// void ToolCallInfoResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
//{
//     ResultBase::OnResponse(ApiResponse);
//
//     if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
//     {
//         rapidjson::Document JsonDoc;
//
//         JsonDoc.Parse(ApiResponse->GetResponse()->GetPayload().GetContent());
//
//         ToolCallInfoResponse.ToolCallData = JsonDoc.GetString();
//     }
// }

RequestedToolCalls& ToolCallInfoResult::GetToolCallsInfo()
{
    CSP_LOG_MSG(common::LogLevel::Log, "Returning ToolCallInfo Array.");

    return RequestedToolCallsInfo;
}

const RequestedToolCalls& ToolCallInfoResult::GetToolCallsInfo() const
{
    CSP_LOG_MSG(common::LogLevel::Log, "Returning ToolCallInfo const Array.");

    return RequestedToolCallsInfo;
}

void ToolCallInfoResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        csp::common::String ResponseContent = ApiResponse->GetResponse()->GetPayload().GetContent();

        /*
        * Output to file for debugging
        */
        WriteGeminiResponseToDisk(ResponseContent);
        //

        rapidjson::Document JsonDoc;
        JsonDoc.Parse(ResponseContent.c_str());

        if (JsonDoc.HasParseError())
        {
            CSP_LOG_MSG(csp::common::LogLevel::Warning, "Error parsing json doc.");
            return;
        }

        // Extract the top-level ResponseId which is unique to this session
        if (JsonDoc.HasMember("responseId") && JsonDoc["responseId"].IsString())
        {
            RequestedToolCallsInfo.ResponseId = JsonDoc["responseId"].GetString();
        }

        // Gemini Path: candidates[0].content.parts[...]
        if (JsonDoc.HasMember("candidates") && JsonDoc["candidates"].IsArray() && JsonDoc["candidates"].Size() > 0)
        {
            const auto& candidate = JsonDoc["candidates"][0];

            if (candidate.HasMember("content") && candidate["content"].HasMember("parts") && candidate["content"]["parts"].IsArray())
            {
                const auto& parts = candidate["content"]["parts"];

                // Construct ToolCallsInfo array to store the requested function calls
                RequestedToolCallsInfo.ToolCalls = csp::common::Array<ToolCallInfo>(parts.Size());

                // Each 'part' may contain a function call
                for (rapidjson::SizeType i = 0; i < parts.Size(); ++i)
                {
                    const auto& part = parts[i];

                    if (part.HasMember("functionCall"))
                    {
                        const auto& call = part["functionCall"];
                        ToolCallInfo Info;

                        if (call.HasMember("name") && call["name"].IsString())
                        {
                            // Store the function name
                            Info.FunctionName = call["name"].GetString();
                        }

                        if (call.HasMember("args") && call["args"].IsObject())
                        {
                            // Store the arguments as a JSON string
                            rapidjson::StringBuffer buffer;
                            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                            call["args"].Accept(writer);
                            Info.Arguments = buffer.GetString();
                        }

                        // Add the Info object to our array
                        RequestedToolCallsInfo.ToolCalls[i] = Info;

                        CSP_LOG_FORMAT(csp::common::LogLevel::Log, "ResponseId: %s | Found Tool Call: %s - Args: %s", RequestedToolCallsInfo.ResponseId.c_str(),
                            Info.FunctionName.c_str(), Info.Arguments.c_str());
                    }
                }
            }
        }
    }
}

} // namespace csp::systems
