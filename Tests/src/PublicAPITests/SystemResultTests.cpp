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
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Web/HTTPResponseCodes.h"
#include "Services/ApiBase/ApiBase.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <CallHelpers.h>
#include <Web/HttpRequest.h>
#include <Web/WebClient.h>

#ifdef CSP_WASM
#include "Web/EmscriptenWebClient/EmscriptenWebClient.h"
#else
#include "Web/POCOWebClient/POCOWebClient.h"
#endif

using namespace csp::common;
using namespace csp::systems;

namespace SystemResultsTest
{

typedef std::function<void(const NullResult& Result)> NullResultCallback;

#ifdef CSP_WASM

class TestWebClient : public csp::web::EmscriptenWebClient
{
public:
    TestWebClient(const csp::web::Port InPort, const csp::web::ETransferProtocol Tp)
        : EmscriptenWebClient(InPort, Tp, false)
    {
    }
};

#else

class TestWebClient : public csp::web::POCOWebClient
{
public:
    TestWebClient(const csp::web::Port InPort, const csp::web::ETransferProtocol Tp)
        : POCOWebClient(InPort, Tp, false)
    {
    }
};

#endif

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

#if RUN_ALL_UNIT_TESTS || RUN_SYSTEMRESULT_TESTS || RUN_SYSTEMRESULT_NULLRESULT_TEST
CSP_PUBLIC_TEST(CSPEngine, SystemResultTests, NullResultTest)
{
    NullResultCallback NullTestCallback
        = [this](const NullResult& _Result) { EXPECT_EQ(_Result.GetResultCode(), csp::systems::EResultCode::Success); };
    NullResultTestFunction(NullTestCallback);
}
#endif

// BaseResult
#if RUN_ALL_UNIT_TESTS || RUN_SYSTEMRESULT_TESTS || RUN_SYSTEMRESULT_BASERESULT_TEST
CSP_PUBLIC_TEST(CSPEngine, SystemResultTests, BaseResultTest)
{
    const csp::web::EResponseCodes MyTestResponseCode = csp::web::EResponseCodes::ResponseOK;
    const csp::common::String MyTestPayload = "1234";

    auto* WebClient = CSP_NEW TestWebClient(80, csp::web::ETransferProtocol::HTTP);
    EXPECT_TRUE(WebClient != nullptr);

    ResponseReceiver Receiver;

    // Synthesise a response to feed to ApiResponseBase
    csp::web::HttpRequest MyTestRequest = csp::web::HttpRequest(WebClient, csp::web::ERequestVerb::GET, csp::web::Uri(),
        csp::web::HttpPayload(MyTestPayload), &Receiver, csp::common::CancellationToken::Dummy());
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
#endif

} // namespace SystemResultsTest