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

#include "Awaitable.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/SharedEnums.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "Services/ApiBase/ApiBase.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <CallHelpers.h>
#include <Common/Web/HttpRequest.h>
#include <Common/Web/WebClient.h>

#ifdef CSP_WASM
#include "Common/Web/EmscriptenWebClient/EmscriptenWebClient.h"
#else
#include "Common/Web/POCOWebClient/POCOWebClient.h"
#endif

using namespace csp::common;
using namespace csp::systems;

typedef std::function<void(const NullResult& Result)> NullResultCallback;

#ifdef CSP_WASM

class TestWebClient : public csp::web::EmscriptenWebClient
{
public:
    TestWebClient(const csp::web::Port InPort, const csp::web::ETransferProtocol Tp, csp::common::IAuthContext& AuthContext, csp::common::LogSystem* LogSystem)
        : EmscriptenWebClient(InPort, Tp, AuthContext, LogSystem, false)
    {
    }
};

#else

class TestWebClient : public csp::web::POCOWebClient
{
public:
    TestWebClient(const csp::web::Port InPort, const csp::web::ETransferProtocol Tp, csp::common::IAuthContext& AuthContext, csp::common::LogSystem* LogSystem)
        : POCOWebClient(InPort, Tp, AuthContext, LogSystem, false)
    {
    }
};

#endif

class TestAuthContext : public IAuthContext
{
public:
    const csp::common::LoginState& GetLoginState() const override { return State; }
    void RefreshToken(std::function<void(bool)> Success) override { Success(true); }

private:
    csp::common::LoginState State;
};

class ResponseReceiver : public ResponseWaiter, public csp::web::IHttpResponseHandler
{
public:
    ResponseReceiver()
        : ResponseReceived(false)
        , ThreadId(std::this_thread::get_id())
    {
    }

    void OnHttpResponse(csp::web::HttpResponse& InResponse) override
    {
        Response = InResponse;
        ResponseReceived = true;
    }

    void OnHttpProgress(csp::web::HttpRequest& Request) override
    {
        if (Request.GetProgress().GetProgressPercentage() >= 1.0f)
        {
            OnHttpResponse(Request.GetMutableResponse());
        }
    }

private:
    csp::web::HttpResponse Response;

    std::atomic<bool> ResponseReceived;
    std::thread::id ThreadId;
};

void NullResultTestFunction(NullResultCallback Callback)
{
    try
    {
        csp::systems::NullResult InternalResult(csp::systems::EResultCode::Success, (uint16_t)csp::web::EResponseCodes::ResponseNoContent);
        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    }
    catch (const std::exception& e)
    {
        std::cout << e.what();
        ADD_FAILURE();
    }
}

CSP_PUBLIC_TEST(CSPEngine, SystemResultTests, NullResultTest)
{
    NullResultCallback NullTestCallback
        = [this](const NullResult& _Result) { EXPECT_EQ(_Result.GetResultCode(), csp::systems::EResultCode::Success); };
    NullResultTestFunction(NullTestCallback);
}

// BaseResult
CSP_PUBLIC_TEST(CSPEngine, SystemResultTests, BaseResultTest)
{
    csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();

    const csp::web::EResponseCodes MyTestResponseCode = csp::web::EResponseCodes::ResponseOK;
    const csp::common::String MyTestPayload = "1234";
    TestAuthContext AuthContext;
    auto* WebClient = new TestWebClient(80, csp::web::ETransferProtocol::HTTP, AuthContext, LogSystem);
    EXPECT_TRUE(WebClient != nullptr);

    ResponseReceiver Receiver;

    // Synthesise a response to feed to ApiResponseBase
    csp::web::HttpPayload MyHttpPayload(MyTestPayload);
    csp::web::HttpRequest MyTestRequest = csp::web::HttpRequest(
        WebClient, csp::web::ERequestVerb::GET, csp::web::Uri(), MyHttpPayload, &Receiver, csp::common::CancellationToken::Dummy());
    MyTestRequest.SetRequestProgress(1.0f);
    MyTestRequest.SetResponseCode(MyTestResponseCode);

    csp::web::HttpResponse MyTestResponse(&MyTestRequest);

    // make a response object
    csp::services::DtoBase* InDto = nullptr;
    csp::services::ApiResponseBase ResponseBase = csp::services::ApiResponseBase(InDto);
    ResponseBase.SetResponse(&MyTestResponse);
    ResponseBase.SetResponseCode(MyTestResponseCode, csp::web::EResponseCodes::ResponseOK);

    csp::web::HttpResponse* ReturnedResponse = &MyTestResponse;
    EXPECT_EQ(ResponseBase.GetResponse(), ReturnedResponse);
    csp::web::HttpRequest* MyRequest = ResponseBase.GetResponse()->GetRequest();
    EXPECT_EQ(MyRequest->GetPayload().GetContent(), "1234");
    EXPECT_EQ(ResponseBase.GetResponseCode(), csp::services::EResponseCode::ResponseSuccess);
}