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

#include "CSP/CSPFoundation.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "PlatformTestUtils.h"
#include "RAIIMockLogger.h"
#include "TestHelpers.h"

#ifdef CSP_WASM
#include "Common/Web/EmscriptenWebClient/EmscriptenWebClient.h"
#else
#include "Common/Web/POCOWebClient/POCOWebClient.h"
#endif

#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <gmock/gmock.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <thread>

using namespace csp::web;

// The WebClientTests will be reviewed as part of OF-1532.

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
        // We expect the callback to have come from a seperate Thread
        EXPECT_FALSE(std::this_thread::get_id() == ThreadId);

        Response = InResponse;
        ResponseReceived = true;
    }

    bool WaitForResponse()
    {
        return WaitFor([this] { return IsResponseReceived(); }, std::chrono::seconds(10));
    }

    bool IsResponseReceived() const { return ResponseReceived; }

    HttpResponse& GetResponse() { return Response; }

private:
    HttpResponse Response;

    std::atomic<bool> ResponseReceived;
    std::thread::id ThreadId;
};

#ifdef CSP_WASM
class TestWebClient : public EmscriptenWebClient
#else
class TestWebClient : public POCOWebClient
#endif
{
public:
    TestWebClient(const Port InPort, const ETransferProtocol Tp, csp::common::IAuthContext& AuthContext, csp::common::LogSystem* LogSystem)
        : POCOWebClient(InPort, Tp, AuthContext, LogSystem, false)
    {
    }

     TestWebClient(const Port InPort, const ETransferProtocol Tp, csp::common::LogSystem* LogSystem)
        : POCOWebClient(InPort, Tp, LogSystem, false)
    {
    }
};


void WebClientSendRequest(csp::web::WebClient* WebClient, const char* Url, ERequestVerb Verb, HttpPayload& Payload, IHttpResponseHandler* Receiver)
{
#ifndef CSP_WASM
    WebClient->SendRequest(Verb, Uri(Url), Payload, Receiver, csp::common::CancellationToken::Dummy());
#else

    std::thread TestThread([&]() { WebClient->SendRequest(Verb, Uri(Url), Payload, Receiver, csp::common::CancellationToken::Dummy()); });

    TestThread.join();
#endif
}

template <typename TReceiver>
void RunWebClientTest(const char* Url, ERequestVerb Verb, uint32_t Port, HttpPayload& Payload, EResponseCodes ExpectedResponse)
{
    TReceiver Receiver;
    auto& UserSystem = *csp::systems::SystemsManager::Get().GetUserSystem();
    csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
    TestWebClient WebClient(Port, ETransferProtocol::HTTP, UserSystem.GetAuthContext(), LogSystem);

    WebClientSendRequest(&WebClient, Url, Verb, Payload, &Receiver);

    //// Sleep thread until response is received
    if (Receiver.WaitForResponse())
    {
        EXPECT_TRUE(Receiver.GetResponse().GetResponseCode() == ExpectedResponse) << "Response was " << (int)Receiver.GetResponse().GetResponseCode();
    }
    else
    {
        FAIL() << "Response timeout" << std::endl;
    }
}

template <typename TReceiver>
void RunWebClientTestWithSetter(const char* Url, ERequestVerb Verb, uint32_t Port, HttpPayload& Payload, EResponseCodes ExpectedResponse)
{
    TReceiver Receiver;
    auto& UserSystem = *csp::systems::SystemsManager::Get().GetUserSystem();
    csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
    TestWebClient WebClient(Port, ETransferProtocol::HTTP, LogSystem);
    WebClient.SetAuthContext(UserSystem.GetAuthContext());

    WebClientSendRequest(&WebClient, Url, Verb, Payload, &Receiver);

    //// Sleep thread until response is received
    if (Receiver.WaitForResponse())
    {
        EXPECT_TRUE(Receiver.GetResponse().GetResponseCode() == ExpectedResponse) << "Response was " << (int)Receiver.GetResponse().GetResponseCode();
    }
    else
    {
        FAIL() << "Response timeout" << std::endl;
    }
}

CSP_INTERNAL_TEST(CSPEngine, WebClientTests, WebClientGetTestExt)
{
    InitialiseFoundation();

    HttpPayload Payload;

    Payload.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

    RunWebClientTest<ResponseReceiver>("https://reqres.in/api/users/2", ERequestVerb::Get, 80, Payload, EResponseCodes::ResponseOK);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, WebClientTests, WebClientGetSetterTestExt)
{
    InitialiseFoundation();

    HttpPayload Payload;

    Payload.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

    RunWebClientTestWithSetter<ResponseReceiver>("https://reqres.in/api/users/2", ERequestVerb::Get, 80, Payload, EResponseCodes::ResponseOK);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, WebClientTests, WebClientPutTestExt)
{
    InitialiseFoundation();

    HttpPayload Payload;

    rapidjson::Document JsonDoc(rapidjson::kObjectType);
    JsonDoc.AddMember("name", "bob", JsonDoc.GetAllocator());
    JsonDoc.AddMember("job", "builder", JsonDoc.GetAllocator());

    Payload.SetContent(JsonDoc);
    Payload.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

    RunWebClientTest<ResponseReceiver>("https://reqres.in/api/users/2", ERequestVerb::Put, 80, Payload, EResponseCodes::ResponseOK);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, WebClientTests, WebClientPostTestExt)
{
    InitialiseFoundation();

    HttpPayload Payload;

    rapidjson::Document JsonDoc(rapidjson::kObjectType);
    JsonDoc.AddMember("email", "eve.holt@reqres.in", JsonDoc.GetAllocator());
    JsonDoc.AddMember("password", "secret", JsonDoc.GetAllocator());

    Payload.SetContent(JsonDoc);

    Payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
    Payload.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

    RunWebClientTest<ResponseReceiver>("https://reqres.in/api/login", ERequestVerb::Post, 80, Payload, EResponseCodes::ResponseOK);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, WebClientTests, WebClientDeleteTestExt)
{
    InitialiseFoundation();

    HttpPayload Payload;
    Payload.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

    RunWebClientTest<ResponseReceiver>("https://reqres.in/api/users/1", ERequestVerb::Delete, 80, Payload, EResponseCodes::ResponseNoContent);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, WebClientTests, WebClientTestExtRequestResponseVeryVerboseLogging)
{
    InitialiseFoundation();

    {
        // We must set the log level to VeryVerbose to receive logs for the HTTP Requests and Responses
        csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
        LogSystem->SetSystemLevel(csp::common::LogLevel::VeryVerbose);

        RAIIMockLogger MockLogger {};

        // Request/Response logs we expect to receive for our HTTP calls.
        // We are only checking against a substring of the request/response logs.
        // Get logs
        csp::common::String CSPLogMsgGetRequestSubstring = "HTTP Request\nGET https://reqres.in/api/users/2";
        csp::common::String CSPLogMsgGetResponseSubstring = "HTTP Response\nGET https://reqres.in/api/users/2";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::HasSubstr(CSPLogMsgGetRequestSubstring)));
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::HasSubstr(CSPLogMsgGetResponseSubstring)));
        // Put logs
        csp::common::String CSPLogMsgPutRequestSubstring = "HTTP Request\nPUT https://reqres.in/api/users/2";
        csp::common::String CSPLogMsgPutResponseSubstring = "HTTP Response\nPUT https://reqres.in/api/users/2";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::HasSubstr(CSPLogMsgPutRequestSubstring)));
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::HasSubstr(CSPLogMsgPutResponseSubstring)));
        // Post logs
        csp::common::String CSPLogMsgPostRequestSubstring = "HTTP Request\nPOST https://reqres.in/api/login";
        csp::common::String CSPLogMsgPostResponseSubstring = "HTTP Response\nPOST https://reqres.in/api/login";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::HasSubstr(CSPLogMsgPostRequestSubstring)));
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::HasSubstr(CSPLogMsgPostResponseSubstring)));
        // Delete logs
        csp::common::String CSPLogMsgDeleteRequestSubstring = "HTTP Request\nDELETE https://reqres.in/api/users/1";
        csp::common::String CSPLogMsgDeleteResponseSubstring = "HTTP Response\nDELETE https://reqres.in/api/users/1";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::HasSubstr(CSPLogMsgDeleteRequestSubstring)));
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::HasSubstr(CSPLogMsgDeleteResponseSubstring)));

        // GET request
        HttpPayload PayloadGet;

        PayloadGet.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

        RunWebClientTest<ResponseReceiver>("https://reqres.in/api/users/2", ERequestVerb::Get, 80, PayloadGet, EResponseCodes::ResponseOK);

        // PUT request
        HttpPayload PayloadPut;

        rapidjson::Document JsonDocPut(rapidjson::kObjectType);
        JsonDocPut.AddMember("name", "bob", JsonDocPut.GetAllocator());
        JsonDocPut.AddMember("job", "builder", JsonDocPut.GetAllocator());

        PayloadPut.SetContent(JsonDocPut);
        PayloadPut.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

        RunWebClientTest<ResponseReceiver>("https://reqres.in/api/users/2", ERequestVerb::Put, 80, PayloadPut, EResponseCodes::ResponseOK);

        // POST request
        HttpPayload PayloadPost;

        rapidjson::Document JsonDocPost(rapidjson::kObjectType);
        JsonDocPost.AddMember("email", "eve.holt@reqres.in", JsonDocPost.GetAllocator());
        JsonDocPost.AddMember("password", "secret", JsonDocPost.GetAllocator());

        PayloadPost.SetContent(JsonDocPost);

        PayloadPost.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
        PayloadPost.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

        RunWebClientTest<ResponseReceiver>("https://reqres.in/api/login", ERequestVerb::Post, 80, PayloadPost, EResponseCodes::ResponseOK);

        // Delete request
        HttpPayload PayloadDelete;
        PayloadDelete.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

        RunWebClientTest<ResponseReceiver>(
            "https://reqres.in/api/users/1", ERequestVerb::Delete, 80, PayloadDelete, EResponseCodes::ResponseNoContent);
    }

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, WebClientTests, WebClientTestExtRequestResponseNoLogging)
{
    InitialiseFoundation();

    {
        RAIIMockLogger MockLogger {};

        // With the log level NOT set to VeryVerbose we expect to receive no logs for the HTTP Requests and Responses
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::_)).Times(0);

        // GET request
        HttpPayload PayloadGet;

        PayloadGet.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

        RunWebClientTest<ResponseReceiver>("https://reqres.in/api/users/2", ERequestVerb::Get, 80, PayloadGet, EResponseCodes::ResponseOK);

        // PUT request
        HttpPayload PayloadPut;

        rapidjson::Document JsonDocPut(rapidjson::kObjectType);
        JsonDocPut.AddMember("name", "bob", JsonDocPut.GetAllocator());
        JsonDocPut.AddMember("job", "builder", JsonDocPut.GetAllocator());

        PayloadPut.SetContent(JsonDocPut);
        PayloadPut.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

        RunWebClientTest<ResponseReceiver>("https://reqres.in/api/users/2", ERequestVerb::Put, 80, PayloadPut, EResponseCodes::ResponseOK);

        // POST request
        HttpPayload PayloadPost;

        rapidjson::Document JsonDocPost(rapidjson::kObjectType);
        JsonDocPost.AddMember("email", "eve.holt@reqres.in", JsonDocPost.GetAllocator());
        JsonDocPost.AddMember("password", "secret", JsonDocPost.GetAllocator());

        PayloadPost.SetContent(JsonDocPost);

        PayloadPost.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
        PayloadPost.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

        RunWebClientTest<ResponseReceiver>("https://reqres.in/api/login", ERequestVerb::Post, 80, PayloadPost, EResponseCodes::ResponseOK);

        // Delete request
        HttpPayload PayloadDelete;
        PayloadDelete.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

        RunWebClientTest<ResponseReceiver>(
            "https://reqres.in/api/users/1", ERequestVerb::Delete, 80, PayloadDelete, EResponseCodes::ResponseNoContent);
    }

    csp::CSPFoundation::Shutdown();
}

class PollingLoginResponseReceiver : public ResponseWaiter, public IHttpResponseHandler
{
public:
    PollingLoginResponseReceiver(std::thread::id InThreadId)
        : ResponseReceived(false)
        , ThreadId(InThreadId)
    {
    }

    void OnHttpResponse(HttpResponse& InResponse) override
    {
        // Check that callbacks are called from the same thread as we poll from
        EXPECT_TRUE(ThreadId == std::this_thread::get_id());

        Response = InResponse;
        ResponseReceived = true;

        if (Response.GetResponseCode() == EResponseCodes::ResponseOK)
        {
            rapidjson::Document ResponseJson;
            ResponseJson.Parse(Response.GetPayload().GetContent().c_str());

            if (ResponseJson.IsObject())
            {
                EXPECT_TRUE(ResponseJson["accessToken"].IsString());

                if (ResponseJson["accessToken"].IsString())
                {
                    AccessToken = ResponseJson["accessToken"].GetString();
                }
            }
            else
            {
                FAIL() << "Invalid response JSON" << std::endl;
            }
        }
        else
        {
            FAIL() << "Invalid response code " << (int)Response.GetResponseCode() << std::endl;
        }
    }

    bool WaitForResponse(csp::web::WebClient* WebClient)
    {
        return WaitFor(
            [this, WebClient]
            {
#ifndef CSP_WASM
                WebClient->ProcessResponses();
#endif
                return IsResponseReceived();
            },
            std::chrono::seconds(10));
    }

    bool IsResponseReceived() const { return ResponseReceived; }

    HttpResponse& GetResponse() { return Response; }
    const std::string& GetAccessToken() { return AccessToken; }

private:
    HttpResponse Response;
    std::string AccessToken;
    std::atomic<bool> ResponseReceived;

    std::thread::id ThreadId;
};

// This test will be fixed and reenabled as part of OF-1536
CSP_INTERNAL_TEST(DISABLED_CSPEngine, WebClientTests, WebClientPollingTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    PollingLoginResponseReceiver Receiver(std::this_thread::get_id());

    {
        auto& UserSystem = *csp::systems::SystemsManager::Get().GetUserSystem();
        csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
        WebClient* Client;
#ifdef CSP_WASM
        Client = new csp::web::EmscriptenWebClient(80, csp::web::ETransferProtocol::HTTPS, UserSystem.GetAuthContext(), LogSystem);
#else
        Client = new TestWebClient(80, csp::web::ETransferProtocol::HTTPS, UserSystem.GetAuthContext(), LogSystem);
#endif
        EXPECT_TRUE(Client != nullptr);

        HttpPayload Payload;

        Payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json-patch+json"));

        rapidjson::Document JsonDoc(rapidjson::kObjectType);
        JsonDoc.AddMember("deviceId", "CSPEngine", JsonDoc.GetAllocator());

        Payload.SetContent(JsonDoc);

        // Tell request not to callback until we poll with WebClient::ProcessResponses
        // This ensures that callbacks are issued from this thread (which we check
        // in the receiver above using the std::thread::id)
        bool AsyncResponse = false;

        Client->SendRequest(
            ERequestVerb::Post, Uri("api/v1/users/login"), Payload, &Receiver, csp::common::CancellationToken::Dummy(), AsyncResponse);

        if (Receiver.WaitForResponse(Client))
        {
            bool ResponseIsValid = Receiver.GetResponse().GetResponseCode() == EResponseCodes::ResponseOK;
            EXPECT_TRUE(ResponseIsValid);

            if (ResponseIsValid)
            {
                EXPECT_TRUE(Receiver.GetAccessToken().length() > 0);
            }
        }
        else
        {
            FAIL() << "Response timeout" << std::endl;
        }
    }

    csp::CSPFoundation::Shutdown();
}

class RetryResponseReceiver : public ResponseWaiter, public IHttpResponseHandler
{
public:
    RetryResponseReceiver()
        : ResponseReceived(false)
        , ThreadId(std::this_thread::get_id())
    {
    }

    void OnHttpResponse(HttpResponse& InResponse) override
    {
        // We expect the callback to have come from a seperate Thread
        EXPECT_FALSE(std::this_thread::get_id() == ThreadId);

        bool RetryIssued = false;
        constexpr uint32_t MaxNumRequestRetries = 3;

        if (InResponse.GetResponseCode() == EResponseCodes::ResponseNotFound)
        {
#ifdef CSP_WASM
            std::thread TestThread([&]() { RetryIssued = InResponse.GetRequest()->Retry(MaxNumRequestRetries); });

            TestThread.join();
#else
            RetryIssued = InResponse.GetRequest()->Retry(MaxNumRequestRetries);
#endif
        }

        if (!RetryIssued)
        {
            EXPECT_TRUE(InResponse.GetRequest()->GetRetryCount() >= MaxNumRequestRetries);

            Response = InResponse;
            ResponseReceived = true;
        }
        else
        {
            std::cerr << "Retrying Request" << std::endl;
        }
    }

    bool WaitForResponse()
    {
        return WaitFor([this] { return IsResponseReceived(); }, std::chrono::seconds(10));
    }

    bool IsResponseReceived() const { return ResponseReceived; }

    HttpResponse& GetResponse() { return Response; }

private:
    HttpResponse Response;

    std::atomic<bool> ResponseReceived;
    std::thread::id ThreadId;
};

CSP_INTERNAL_TEST(DISABLED_CSPEngine, WebClientTests, WebClientRetryTest)
{
    InitialiseFoundation();

    HttpPayload Payload;
    Payload.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

    RunWebClientTest<RetryResponseReceiver>("https://reqres.in/api/users/23", ERequestVerb::Get, 80, Payload, EResponseCodes::ResponseNotFound);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, WebClientTests, HttpFail404Test)
{
    InitialiseFoundation();

    HttpPayload Payload;
    Payload.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

    RunWebClientTest<ResponseReceiver>("https://reqres.in/apiiii/users/23", ERequestVerb::Get, 80, Payload, EResponseCodes::ResponseNotFound);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(DISABLED_CSPEngine, WebClientTests, HttpFail400Test)
{
    InitialiseFoundation();

    HttpPayload Payload;
    Payload.AddContent("{ \"email\": \"test@olympus\" }");
    Payload.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("reqres-free-v1"));

    RunWebClientTest<RetryResponseReceiver>("https://reqres.in/api/register", ERequestVerb::Post, 80, Payload, EResponseCodes::ResponseBadRequest);

    csp::CSPFoundation::Shutdown();
}

// Current fails on wasm platform tests due to CORS policy.
#ifndef CSP_WASM

CSP_INTERNAL_TEST(CSPEngine, WebClientTests, WebClientUserAgentTest)
{
    InitialiseFoundation();

    HttpPayload Payload;
    ResponseReceiver Receiver;

    auto& UserSystem = *csp::systems::SystemsManager::Get().GetUserSystem();
    csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
    auto* WebClient = new TestWebClient(80, ETransferProtocol::HTTP, UserSystem.GetAuthContext(), LogSystem);
    EXPECT_TRUE(WebClient != nullptr);

    WebClientSendRequest(WebClient, "https://postman-echo.com/get", ERequestVerb::Get, Payload, &Receiver);

    //// Sleep thread until response is received
    if (Receiver.WaitForResponse())
    {
        std::string ResponseContent = Receiver.GetResponse().GetPayload().GetContent().c_str();

        EXPECT_TRUE(ResponseContent.find(TESTS_CLIENT_SKU) != std::string::npos) << TESTS_CLIENT_SKU << " was not found.";
    }
    else
    {
        FAIL() << "Response timeout" << std::endl;
    }

    csp::CSPFoundation::Shutdown();
}
#endif

#include "CSP/Systems/SystemsManager.h"
#include "PublicAPITests/UserSystemTestHelpers.h"

CSP_INTERNAL_TEST(DISABLED_CSPEngine, WebClientTests, HttpFail403Test)
{
    InitialiseFoundation();

    auto* UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    HttpPayload Payload;
    RunWebClientTest<RetryResponseReceiver>(
        (std::string(EndpointBaseURI()) + "/mag-user/appsettings").c_str(), ERequestVerb::Get, 80, Payload, EResponseCodes::ResponseForbidden);

    csp::CSPFoundation::Shutdown();
}