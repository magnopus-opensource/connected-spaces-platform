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
#pragma once

#ifndef CSP_WASM

#include "Common/Web/WebClient.h"

#include <Poco/Net/HTTPCookie.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/PartSource.h>
#include <Poco/Net/PrivateKeyPassphraseHandler.h>

namespace csp::common
{
class LogSystem;
}
namespace csp::web
{

class PocoPrivateKeyHandler : public Poco::Net::PrivateKeyPassphraseHandler
{
public:
    PocoPrivateKeyHandler(bool onServerSide)
        : Poco::Net::PrivateKeyPassphraseHandler(onServerSide)
    {
    }

    virtual void onPrivateKeyRequested(const void* /*pSender*/, std::string& /*privateKey*/) override { }
};

class POCOWebClient : public WebClient
{
public:
    virtual ~POCOWebClient();

    using WebClient::WebClient;

    std::string MD5Hash(const void* data, const size_t size) override;

    void SetFileUploadContentFromFile(HttpPayload* payload, const char* filePath, const char* version, const csp::common::String& mediaType) override;
    void SetFileUploadContentFromString(HttpPayload* payload, const csp::common::String& stringSource, const csp::common::String& fileName,
        const char* version, const csp::common::String& mediaType) override;
    void SetFileUploadContentFromBuffer(HttpPayload* payload, const char* buffer, size_t bufferLength, const csp::common::String& fileName,
        const char* version, const csp::common::String& mediaType) override;

    // Instances of POCOWebClient should not be created. You should instead rely on the instance that `csp::systems::SystemsManager` holds.
    POCOWebClient(const Port inPort, const ETransferProtocol tp, csp::common::LogSystem* logSystem, bool autoRefresh = true);
    POCOWebClient(const Port inPort, const ETransferProtocol tp, csp::common::IAuthContext& authContext, csp::common::LogSystem* logSystem, bool autoRefresh = true);

protected:
    void SetFileUploadContent(HttpPayload* payload, Poco::Net::PartSource* source, const char* version);

    void Send(HttpRequest& request) override;

    void Get(HttpRequest& request);
    void AddCookie(Poco::Net::HTTPRequest& pocoRequest);
    void Post(HttpRequest& request);
    void Put(HttpRequest& request);
    void Delete(HttpRequest& request);
    void Head(HttpRequest& request);
    void Patch(HttpRequest& request);

    void ProcessResponseAsync(
        Poco::Net::HTTPClientSession& clientSession, Poco::Net::HTTPResponse& pocoResponse, std::istream& responseStream, HttpRequest& request);
    void ProcessRequestAsync(
        Poco::Net::HTTPClientSession& clientSession, Poco::Net::HTTPRequest& pocoResponse, std::ostream& requestStream, HttpRequest& request);

    Poco::Net::Context::Ptr m_pocoContext;

    std::vector<Poco::Net::HTTPCookie>* m_cookies;
    std::mutex m_cookiesMutex;

private:
    /// @brief Specifies how the request body will be sent.
    enum class ERequestBodyMode
    {
        // No body is sent with the request
        None,
        // Body is streamed in chunks with progress tracking and cancellation support
        Streamed,
        // Body is written inline to the request stream in a single operation
        Inline
    };

    bool PrepareAndSendRequest(
        HttpRequest& request, Poco::Net::HTTPRequest pocoRequest, Poco::Net::HTTPClientSession* clientSession, ERequestBodyMode sendBodyMode);

    std::istream& ReceiveResponse(
        Poco::Net::HTTPClientSession* clientSession, Poco::Net::HTTPResponse& pocoResponse, HttpRequest& request);
};

} // namespace csp::web
#endif
