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

/**/
#include <Awaitable.h>
/**/

#include "CSP/CSPFoundation.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/ToolCalls/ToolCallsSystem.h"
#include "CSP/Systems/ToolCalls/ToolCallInfo.h"
#include "CSP/Systems/ToolCalls/ToolCallExecutor.h"
#include "RAIIMockLogger.h"

#include "Systems/ResultHelpers.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <Common/Logger.h>
#include <Debug/Logging.h>
#include <filesystem>
#include <fstream>
#include <rapidjson/document.h>

#include "CSP/Common/CSPAsyncScheduler.h"
#include "CSP/Common/ContinuationUtils.h"
#include "CSP/Common/Optional.h"
#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Multiplayer/SignalR/SignalRConnection.h"
#include "Multiplayer/SpaceEntityKeys.h"
#include "MultiplayerTestRunnerProcess.h"
#include "SpaceSystemTestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "signalrclient/signalr_value.h"
#include <array>
#include <chrono>
#include <thread>

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

bool RequestPredicateWithProgress(const csp::systems::ResultBase& Result)
{
    if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
    {
        PrintProgress(Result.GetRequestProgress());

        return false;
    }

    return true;
}

} // namespace

void RetrieveLocalAPIKey(csp::common::String& APIKey)
{
    const char* userProfile = std::getenv("USERPROFILE");
    if (!userProfile)
    {
        CSP_LOG(csp::common::LogLevel::Warning, "Could not retrieve use profile.");

        FAIL();
    }

    std::filesystem::path LLMConfigPath = std::filesystem::path(userProfile) / "Desktop" / "LLMConfig.json";

    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "Reading from filepath: %s", LLMConfigPath.u8string().c_str());

    std::ifstream Stream { LLMConfigPath.u8string().c_str() };

    if (!Stream)
    {
        CSP_LOG(csp::common::LogLevel::Warning, "Stream at specified path could not be read.");
        FAIL();
    }

    std::stringstream SStream;
    SStream << Stream.rdbuf();

    rapidjson::Document Doc;
    Doc.Parse(SStream.str().c_str());

    if (Doc.HasParseError())
    {
        CSP_LOG(csp::common::LogLevel::Warning, "RapidJson document parsing error.");
        FAIL();
    }

    if (Doc.HasMember("api_key") && Doc["api_key"].IsString())
    {
        CSP_LOG_FORMAT(csp::common::LogLevel::Log, "Successfully read API Key: %s", Doc["api_key"].GetString());

        APIKey = Doc["api_key"].GetString();
    }
    else
    {
        CSP_LOG(csp::common::LogLevel::Warning, "Failed to read API Key.");

        APIKey = "";
    }

    return;
}

CSP_PUBLIC_TEST(CSPEngine, ToolCallsSystemTests, GetConfigTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* ToolCallsSystem = SystemsManager.GetToolCallsSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) { });

    ToolCallsSystem->SetRealtimeEngine(RealtimeEngine.get());

    // TOOL CALL STUFF BELOW
    const char* userProfile = std::getenv("USERPROFILE");
    if (!userProfile)
    {
        CSP_LOG(csp::common::LogLevel::Warning, "Could not retrieve use profile.");

        FAIL();
    }

    // Define the LLM Config path
    std::filesystem::path GeminiConfigsPath = std::filesystem::path(userProfile) / "Desktop" / "GeminiConfigs";

    /*
     * Retrieve the Request URL
     */    

    const csp::common::String& LLMConfigFilename = "LLMConfig.json";

    const csp::common::String& RequestURL = ToolCallsSystem->ConstructRequestURL(LLMConfigFilename, GeminiConfigsPath.u8string().c_str());

    EXPECT_FALSE(RequestURL.IsEmpty());

    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "Request URL: %s", RequestURL.c_str());

    /*
     * Retrieve the prompt
     */

    const csp::common::String& PromptFilename = "Prompt.txt";

    const csp::common::String& UserPrompt = ToolCallsSystem->RetreiveUserPrompt(PromptFilename, GeminiConfigsPath.u8string().c_str());

    /*
     * Retrieve the Request Body
     */

    const csp::common::String& ToolCallsFilename = "ToolCallsSchema.json";

    //const csp::common::String& UserPrompt = "Who is meeting with John today?";

    const csp::common::String& RequestBodyJson
        = ToolCallsSystem->ConstructRequestBodyJson(ToolCallsFilename, GeminiConfigsPath.u8string().c_str(), UserPrompt);

    EXPECT_FALSE(RequestBodyJson.IsEmpty());

    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "Request Body: %s", RequestBodyJson.c_str());

    /*
     * Send a request
     */
    
    auto [Result]
        = AWAIT_PRE(ToolCallsSystem, SendRequest, RequestPredicate, RequestURL, RequestBodyJson);
    
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    
    //const auto& RequestedToolCalls = Result.GetToolCallsInfo();

    // For testing
    //csp::systems::RequestedToolCalls RequestedToolCallInfos;
    //RequestedToolCallInfos.ResponseId = "test_response_id_12345";
    //
    //RequestedToolCallInfos.ToolCalls = csp::common::Array<csp::systems::ToolCallInfo>(2);
    //csp::systems::ToolCallInfo Info;
    //Info.FunctionName = "get_meeting_info";
    //Info.Arguments = "{ \"organizer\": \"John\" }";
    //RequestedToolCallInfos.ToolCalls[0] = Info;
    //csp::systems::ToolCallInfo Info2;
    //Info2.FunctionName = "get_meeting_info";
    //Info2.Arguments = "{ \"organizer\": \"Matt\" }";
    //RequestedToolCallInfos.ToolCalls[1] = Info2;
    //
    //csp::systems::ToolCallExecutor* Executor = ToolCallsSystem->GetToolCallExecutor();
    //
    //Executor->InvokeToolCalls(RequestedToolCallInfos);

    //

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ToolCallsSystemTests, GetConfigTest2)
{
    SetRandSeed();

    csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
    LogSystem->SetSystemLevel(csp::common::LogLevel::VeryVerbose);

    //auto& SystemsManager = csp::systems::SystemsManager::Get();
    //auto* ToolCallsSystem = SystemsManager.GetToolCallsSystem();

    const char* userProfile = std::getenv("USERPROFILE");
    if (!userProfile)
    {
        CSP_LOG(csp::common::LogLevel::Warning, "Could not retrieve use profile.");

        FAIL();
    }

    std::filesystem::path ToolCallsPath = std::filesystem::path(userProfile) / "Desktop" / "GeminiConfigs" / "tools.json";

    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "Reading from filepath: %s", ToolCallsPath.u8string().c_str());

    csp::common::String Path = ToolCallsPath.u8string().c_str();

    // auto [Result] = AWAIT_PRE(ToolCallsSystem, CreateRequestWithExternalTools, RequestPredicate, Path);
    //
    // EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    // EXPECT_TRUE(1);
}