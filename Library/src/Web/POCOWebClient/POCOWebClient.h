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

#include "Web/WebClient.h"

#include <Poco/Net/HTTPCookie.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/PartSource.h>
#include <Poco/Net/PrivateKeyPassphraseHandler.h>

namespace csp::systems
{

class SystemsManager;

}

namespace csp::multiplayer
{

class CSPHttpClient;

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

    virtual void onPrivateKeyRequested(const void* pSender, std::string& privateKey) override { }
};

class POCOWebClient : public WebClient
{
    friend class csp::systems::SystemsManager;
    friend class csp::multiplayer::CSPHttpClient;

public:
    virtual ~POCOWebClient();

    std::string MD5Hash(const void* Data, const size_t Size) override;

    void SetFileUploadContentFromFile(HttpPayload* Payload, const char* FilePath, const char* Version, const csp::common::String& MediaType) override;
    void SetFileUploadContentFromString(HttpPayload* Payload, const csp::common::String& StringSource, const csp::common::String& FileName,
        const char* Version, const csp::common::String& MediaType) override;
    void SetFileUploadContentFromBuffer(HttpPayload* Payload, const char* Buffer, size_t BufferLength, const csp::common::String& FileName,
        const char* Version, const csp::common::String& MediaType) override;

protected:
    // Instances of POCOWebClient should not be created. You should instead rely on the instance that `csp::systems::SystemsManager` holds.
    POCOWebClient(const Port InPort, const ETransferProtocol Tp, bool AutoRefresh = true);

    void SetFileUploadContent(HttpPayload* Payload, Poco::Net::PartSource* Source, const char* Version);

    void Send(HttpRequest& Request) override;

    void Get(HttpRequest& Request);
    void AddCookie(Poco::Net::HTTPRequest& PocoRequest);
    void Post(HttpRequest& Request);
    void Put(HttpRequest& Request);
    void Delete(HttpRequest& Request);
    void Head(HttpRequest& Request);

    void ProcessResponseAsync(
        Poco::Net::HTTPClientSession& ClientSession, Poco::Net::HTTPResponse& PocoResponse, std::istream& ResponseStream, HttpRequest& Request);
    void ProcessRequestAsync(
        Poco::Net::HTTPClientSession& ClientSession, Poco::Net::HTTPRequest& PocoResponse, std::ostream& RequestStream, HttpRequest& Request);

    Poco::Net::Context::Ptr PocoContext;

    std::vector<Poco::Net::HTTPCookie>* Cookies;
    std::mutex CookiesMutex;
};

} // namespace csp::web
