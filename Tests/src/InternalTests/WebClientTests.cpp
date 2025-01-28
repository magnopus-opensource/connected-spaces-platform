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
#include "PlatformTestUtils.h"
#include "TestHelpers.h"

#ifdef CSP_WASM
#include "Web/EmscriptenWebClient/EmscriptenWebClient.h"
#else
#include "Web/POCOWebClient/POCOWebClient.h"
#endif

#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
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
{
public:
    TestWebClient(const Port InPort, const ETransferProtocol Tp)
        : EmscriptenWebClient(InPort, Tp, false)
    {
    }
};

#else

class TestWebClient : public POCOWebClient
{
public:
    TestWebClient(const Port InPort, const ETransferProtocol Tp)
        : POCOWebClient(InPort, Tp, false)
    {
    }
};

#endif

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

    TestWebClient WebClient(Port, ETransferProtocol::HTTP);

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

#if RUN_ALL_UNIT_TESTS || RUN_PLATFORM_TESTS || RUN_WEB_CLIENT_GET_TEST_EXT_TEST
CSP_INTERNAL_TEST(CSPEngine, WebClientTests, WebClientGetTestExt)
{
    InitialiseFoundation();

    HttpPayload Payload;

    RunWebClientTest<ResponseReceiver>("https://reqres.in/api/users", ERequestVerb::Get, 80, Payload, EResponseCodes::ResponseOK);

    csp::CSPFoundation::Shutdown();
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_PLATFORM_TESTS || RUN_WEB_CLIENT_PUT_TEST_EXT_TEST
CSP_INTERNAL_TEST(CSPEngine, WebClientTests, WebClientPutTestExt)
{
    InitialiseFoundation();

    HttpPayload Payload;

    rapidjson::Document JsonDoc(rapidjson::kObjectType);
    JsonDoc.AddMember("name", "bob", JsonDoc.GetAllocator());
    JsonDoc.AddMember("job", "builder", JsonDoc.GetAllocator());

    Payload.SetContent(JsonDoc);

    RunWebClientTest<ResponseReceiver>("https://reqres.in/api/users/2", ERequestVerb::Put, 80, Payload, EResponseCodes::ResponseOK);

    csp::CSPFoundation::Shutdown();
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_PLATFORM_TESTS || RUN_WEB_CLIENT_POST_TEST_EXT_TEST
CSP_INTERNAL_TEST(CSPEngine, WebClientTests, WebClientPostTestExt)
{
    InitialiseFoundation();

    HttpPayload Payload;

    rapidjson::Document JsonDoc(rapidjson::kObjectType);
    JsonDoc.AddMember("email", "eve.holt@reqres.in", JsonDoc.GetAllocator());
    JsonDoc.AddMember("password", "secret", JsonDoc.GetAllocator());

    Payload.SetContent(JsonDoc);

    Payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));

    RunWebClientTest<ResponseReceiver>("https://reqres.in/api/login", ERequestVerb::Post, 80, Payload, EResponseCodes::ResponseOK);

    csp::CSPFoundation::Shutdown();
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_PLATFORM_TESTS || RUN_WEB_CLIENT_DELETE_TEST_EXT_TEST
CSP_INTERNAL_TEST(CSPEngine, WebClientTests, WebClientDeleteTestExt)
{
    InitialiseFoundation();

    HttpPayload Payload;

    RunWebClientTest<ResponseReceiver>("https://reqres.in/api/users/1", ERequestVerb::Delete, 80, Payload, EResponseCodes::ResponseNoContent);

    csp::CSPFoundation::Shutdown();
}
#endif

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
#if RUN_ALL_UNIT_TESTS || RUN_PLATFORM_TESTS || RUN_WEB_CLIENT_POLLING_TEST
CSP_INTERNAL_TEST(DISABLED_CSPEngine, WebClientTests, WebClientPollingTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    PollingLoginResponseReceiver Receiver(std::this_thread::get_id());

    {

        WebClient* Client;
#ifdef CSP_WASM
        Client = CSP_NEW csp::web::EmscriptenWebClient(80, csp::web::ETransferProtocol::HTTPS);
#else
        Client = CSP_NEW TestWebClient(80, csp::web::ETransferProtocol::HTTPS);
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
#endif

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

#if RUN_ALL_UNIT_TESTS || RUN_PLATFORM_TESTS || RUN_WEB_CLIENT_RETRY_TEST
CSP_INTERNAL_TEST(DISABLED_CSPEngine, WebClientTests, WebClientRetryTest)
{
    InitialiseFoundation();

    HttpPayload Payload;

    RunWebClientTest<RetryResponseReceiver>("https://reqres.in/api/users/23", ERequestVerb::Get, 80, Payload, EResponseCodes::ResponseNotFound);

    csp::CSPFoundation::Shutdown();
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_PLATFORM_TESTS || RUN_HTTP_FAIL_404_TEST
CSP_INTERNAL_TEST(CSPEngine, WebClientTests, HttpFail404Test)
{
    InitialiseFoundation();

    HttpPayload Payload;

    RunWebClientTest<ResponseReceiver>("https://reqres.in/apiiii/users/23", ERequestVerb::Get, 80, Payload, EResponseCodes::ResponseNotFound);

    csp::CSPFoundation::Shutdown();
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_PLATFORM_TESTS || RUN_HTTP_FAIL_400_TEST
CSP_INTERNAL_TEST(DISABLED_CSPEngine, WebClientTests, HttpFail400Test)
{
    InitialiseFoundation();

    HttpPayload Payload;
    Payload.AddContent("{ \"email\": \"test@olympus\" }");

    RunWebClientTest<RetryResponseReceiver>("https://reqres.in/api/register", ERequestVerb::Post, 80, Payload, EResponseCodes::ResponseBadRequest);

    csp::CSPFoundation::Shutdown();
}
#endif

// Current fails on wasm platform tests due to CORS policy.
#ifndef CSP_WASM

#if RUN_ALL_UNIT_TESTS || RUN_PLATFORM_TESTS || RUN_WEB_CLIENT_USER_AGENT_TEST
CSP_INTERNAL_TEST(CSPEngine, WebClientTests, WebClientUserAgentTest)
{
    InitialiseFoundation();

    HttpPayload Payload;
    ResponseReceiver Receiver;

    auto* WebClient = CSP_NEW TestWebClient(80, ETransferProtocol::HTTP);
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
#endif

#include "CSP/Systems/SystemsManager.h"
#include "PublicAPITests/UserSystemTestHelpers.h"

#if RUN_ALL_UNIT_TESTS || RUN_PLATFORM_TESTS || RUN_HTTP_FAIL_403_TEST
CSP_INTERNAL_TEST(DISABLED_CSPEngine, WebClientTests, HttpFail403Test)
{
    InitialiseFoundation();

    auto* UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    HttpPayload Payload;
    RunWebClientTest<RetryResponseReceiver>(
        "https://ogs-internal.magnopus-dev.cloud/mag-user/appsettings", ERequestVerb::Get, 80, Payload, EResponseCodes::ResponseForbidden);

    csp::CSPFoundation::Shutdown();
}
#endif