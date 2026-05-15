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

#include "CallHelpers.h"
#include "Common/Web/HttpRequest.h"
#include "Common/Web/WebClient.h"
#include "gtest/gtest.h"

#ifdef CSP_WASM
#include "Common/Web/EmscriptenWebClient/EmscriptenWebClient.h"
#else
#include "Common/Web/POCOWebClient/POCOWebClient.h"
#endif

using namespace csp::common;
using namespace csp::systems;

typedef std::function<void(const NullResult& result)> NullResultCallback;

#ifdef CSP_WASM

class TestWebClient : public csp::web::EmscriptenWebClient
{
public:
    TestWebClient(
        const csp::web::Port InPort, const csp::web::ETransferProtocol Tp, csp::common::IAuthContext& AuthContext, csp::common::LogSystem* LogSystem)
        : EmscriptenWebClient(InPort, Tp, AuthContext, LogSystem, false)
    {
    }
};

#else

class TestWebClient : public csp::web::POCOWebClient
{
public:
    TestWebClient(
        const csp::web::Port inPort, const csp::web::ETransferProtocol tp, csp::common::IAuthContext& authContext, csp::common::LogSystem* logSystem)
        : POCOWebClient(inPort, tp, authContext, logSystem, false)
    {
    }
};

#endif

class ResponseReceiver : public ResponseWaiter, public csp::web::IHttpResponseHandler
{
public:
    ResponseReceiver()
        : m_responseReceived(false)
        , m_threadId(std::this_thread::get_id())
    {
    }

    void OnHttpResponse(csp::web::HttpResponse& inResponse) override
    {
        m_response = inResponse;
        m_responseReceived = true;
    }

    void OnHttpProgress(csp::web::HttpRequest& request) override
    {
        if (request.GetProgress().GetProgressPercentage() >= 1.0f)
        {
            OnHttpResponse(request.GetMutableResponse());
        }
    }

private:
    csp::web::HttpResponse m_response;

    std::atomic<bool> m_responseReceived;
    std::thread::id m_threadId;
};

void NullResultTestFunction(NullResultCallback callback)
{
    try
    {
        csp::systems::NullResult internalResult(csp::systems::EResultCode::Success, (uint16_t)csp::web::EResponseCodes::ResponseNoContent);
        INVOKE_IF_NOT_NULL(callback, internalResult);
    }
    catch (const std::exception& e)
    {
        std::cout << e.what();
        ADD_FAILURE();
    }
}

CSP_PUBLIC_TEST(CSPEngine, SystemResultTests, NullResultTest)
{
    NullResultCallback nullTestCallback
        = [this](const NullResult& result) { EXPECT_EQ(result.GetResultCode(), csp::systems::EResultCode::Success); };
    NullResultTestFunction(nullTestCallback);
}

// BaseResult
CSP_PUBLIC_TEST(CSPEngine, SystemResultTests, BaseResultTest)
{
    csp::common::LogSystem* logSystem = csp::systems::SystemsManager::Get().GetLogSystem();

    const csp::web::EResponseCodes myTestResponseCode = csp::web::EResponseCodes::ResponseOK;
    const csp::common::String myTestPayload = "1234";
    TestAuthContext authContext;
    auto* webClient = new TestWebClient(80, csp::web::ETransferProtocol::HTTP, authContext, logSystem);
    EXPECT_TRUE(webClient != nullptr);

    ResponseReceiver receiver;

    // Synthesise a response to feed to ApiResponseBase
    csp::web::HttpPayload myHttpPayload(myTestPayload);
    csp::web::HttpRequest myTestRequest = csp::web::HttpRequest(
        webClient, csp::web::ERequestVerb::GET, csp::web::Uri(), myHttpPayload, &receiver, csp::common::CancellationToken::Dummy());
    myTestRequest.SetRequestProgress(1.0f);
    myTestRequest.SetResponseCode(myTestResponseCode);

    csp::web::HttpResponse myTestResponse(&myTestRequest);

    // make a response object
    csp::services::DtoBase* inDto = nullptr;
    csp::services::ApiResponseBase responseBase = csp::services::ApiResponseBase(inDto);
    responseBase.SetResponse(&myTestResponse);
    responseBase.SetResponseCode(myTestResponseCode, csp::web::EResponseCodes::ResponseOK);

    csp::web::HttpResponse* returnedResponse = &myTestResponse;
    EXPECT_EQ(responseBase.GetResponse(), returnedResponse);
    csp::web::HttpRequest* myRequest = responseBase.GetResponse()->GetRequest();
    EXPECT_EQ(myRequest->GetPayload().GetContent(), "1234");
    EXPECT_EQ(responseBase.GetResponseCode(), csp::services::EResponseCode::ResponseSuccess);
}