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

#ifndef CSP_WASM

#include "POCOWebClient.h"

#include "Debug/Logging.h"

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Common/fmt_Formatters.h"

#include <Poco/File.h>
#include <Poco/MD5Engine.h>
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/Context.h>
#include <Poco/Net/FilePartSource.h>
#include <Poco/Net/HTMLForm.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSSessionInstantiator.h>
#include <Poco/Net/HTTPSessionFactory.h>
#include <Poco/Net/HTTPSessionInstantiator.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/StringPartSource.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>
#include <chrono>
#include <codecvt>
#include <iostream>
#include <istream>
#include <string>
#include <thread>

namespace
{

template <size_t N> constexpr size_t CStringLength(char const (&)[N]) { return N - 1; }

void LogHttpResponseIfLoglevelVeryVerbose(
    csp::common::LogSystem* logSystem, const char* verb, const csp::web::HttpRequest& request, const Poco::Net::HTTPResponse& pocoResponse)
{
    // If the LogSystem LogLevel has been set to VeryVerbose, log the response.
    if (logSystem != nullptr && logSystem->GetSystemLevel() == csp::common::LogLevel::VeryVerbose)
    {
        logSystem->LogMsg(csp::common::LogLevel::VeryVerbose,
            fmt::format("HTTP Response\n{0} {1}\nStatus: {2} - {3}", verb, request.GetUri().GetAsString(), static_cast<int>(pocoResponse.getStatus()),
                pocoResponse.getReason())
                .c_str());
    }
}

/// @brief Copies headers from a Poco HTTPResponse to requests response payload.
void CopyResponseHeaders(const Poco::Net::HTTPResponse& pocoResponse, csp::web::HttpPayload& payload)
{
    for (auto iter = pocoResponse.begin(); iter != pocoResponse.end(); ++iter)
    {
        std::string key = iter->first;
        std::string val = iter->second;

        // Make Key and Val lower-case
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(val.begin(), val.end(), val.begin(), [](unsigned char c) { return std::tolower(c); });

        payload.AddHeader(key.c_str(), val.c_str());
    }
}

} // namespace

namespace csp::web
{

/// @brief Size of stack space used for async streaming upload and download
const uint32_t kPOCOAsyncBufferSize = 2 * 1024;

EResponseCodes GetOlyResponseCode(Poco::Net::HTTPResponse::HTTPStatus pocoResponseCode) { return (EResponseCodes)pocoResponseCode; }

/// @brief Prepares a Poco HTTPRequest by copying headers and cookies from the provided HttpRequest.
/// If specified it will send a request body either streamed or inline.
bool POCOWebClient::PrepareAndSendRequest(
    HttpRequest& request, Poco::Net::HTTPRequest pocoRequest, Poco::Net::HTTPClientSession* clientSession, ERequestBodyMode sendBodyMode)
{
    for (auto header : request.GetPayload().GetHeaders())
    {
        pocoRequest.add(header.first.c_str(), header.second.c_str());
    }

    AddCookie(pocoRequest);

    switch (sendBodyMode)
    {
    case ERequestBodyMode::None:
    {
        // No body is being sent with the request
        clientSession->sendRequest(pocoRequest);
        break;
    }
    case ERequestBodyMode::Streamed:
    {
        // The request body is being sent as a stream, which allows for progress tracking and cancellation support
        size_t contentLength = request.GetPayload().GetContent().Length();
        pocoRequest.setContentLength(contentLength);
        std::ostream& requestStream = clientSession->sendRequest(pocoRequest);
        ProcessRequestAsync(*clientSession, pocoRequest, requestStream, request);

        if (request.Cancelled())
        {
            return false;
        }
        break;
    }
    case ERequestBodyMode::Inline:
    {
        // The request body is being sent inline in a single operation
        const std::string body(request.GetPayload().GetContent().c_str());
        pocoRequest.setContentLength(body.length());
        clientSession->sendRequest(pocoRequest) << body;
        break;
    }
    }

    return true;
}

/// @brief Receives a response to a sent request, and copies cookies to the provided HttpRequest.
std::istream& POCOWebClient::ReceiveResponse(Poco::Net::HTTPClientSession* clientSession, Poco::Net::HTTPResponse& pocoResponse, HttpRequest& request)
{
    std::istream& responseStream = clientSession->receiveResponse(pocoResponse);
    request.SetResponseCode(GetOlyResponseCode(pocoResponse.getStatus()));

    {
        std::scoped_lock lock(m_cookiesMutex);
        pocoResponse.getCookies(*m_cookies);
    }

    return responseStream;
}

POCOWebClient::POCOWebClient(const Port inPort, const ETransferProtocol tp, csp::common::LogSystem* logSystem, bool autoRefresh)
    : WebClient(inPort, tp, logSystem, autoRefresh)
{
    Poco::Net::initializeSSL();

    auto certHandler = Poco::makeShared<Poco::Net::AcceptCertificateHandler>(false);
    auto privateKeyHandler = Poco::makeShared<PocoPrivateKeyHandler>(false);

    m_pocoContext
        = Poco::makeAuto<Poco::Net::Context>(Poco::Net::Context::CLIENT_USE, "", Poco::Net::Context::VerificationMode::VERIFY_RELAXED, 9, true);

    Poco::Net::HTTPSessionFactory::defaultFactory().registerProtocol("http", new Poco::Net::HTTPSessionInstantiator);
    Poco::Net::HTTPSessionFactory::defaultFactory().registerProtocol("https", new Poco::Net::HTTPSSessionInstantiator);

    // TODO: Get rid of singleton usage entirely. Until then, we can't create multiple instances of Connected Spaces Platform.
    Poco::Net::SSLManager::instance().initializeClient(privateKeyHandler, certHandler, m_pocoContext);

    m_cookies = new std::remove_pointer_t<decltype(m_cookies)>();
}

POCOWebClient::POCOWebClient(
    const Port inPort, const ETransferProtocol tp, csp::common::IAuthContext& authContext, csp::common::LogSystem* logSystem, bool autoRefresh)
    : POCOWebClient(inPort, tp, logSystem, autoRefresh)
{
    SetAuthContext(authContext);
}

POCOWebClient::~POCOWebClient() { delete m_cookies; }

void POCOWebClient::Send(HttpRequest& request)
{
    try
    {
        const ERequestVerb verb = request.GetVerb();

        switch (verb)
        {
        case ERequestVerb::Get:
            Get(request);
            break;
        case ERequestVerb::Post:
            Post(request);
            break;
        case ERequestVerb::Put:
            Put(request);
            break;
        case ERequestVerb::Delete:
            Delete(request);
            break;
        case ERequestVerb::Head:
            Head(request);
            break;
        case ERequestVerb::Patch:
            Patch(request);
            break;
        default:
            break;
        }
    }
    catch (const Poco::Exception& ex)
    {
        throw WebClientException(ex.displayText());
    }
}

void POCOWebClient::AddCookie(Poco::Net::HTTPRequest& pocoRequest)
{
    {
        std::scoped_lock lock(m_cookiesMutex);

        if (m_cookies->size() == 0)
        {
            return;
        }
    }

    // create a collection to build up with Cookies to attach to the request
    Poco::Net::NameValueCollection cookieCollection = Poco::Net::NameValueCollection();

    // Iterate our stored cookies and convert into a collection entry to attach to the request
    {
        std::scoped_lock lock(m_cookiesMutex);

        for (const Poco::Net::HTTPCookie& cookie : *m_cookies)
        {
            cookieCollection.add(cookie.getName(), cookie.getValue());
        }
    }

    // Set the cookies on our request
    pocoRequest.setCookies(cookieCollection);
}

void POCOWebClient::Get(HttpRequest& request)
{
    CSP_PROFILE_SCOPED_FORMAT("GET %s", request.GetUri().GetAsStdString().c_str());

    Poco::URI uri(request.GetUri().GetAsStdString());

    // HTTPSessionFactory returns an unmanaged raw pointer - unique_ptr ensures the session is safely cleaned up after use.
    std::unique_ptr<Poco::Net::HTTPClientSession> clientSession(Poco::Net::HTTPSessionFactory::defaultFactory().createClientSession(uri));
    Poco::Net::HTTPRequest pocoRequest(Poco::Net::HTTPRequest::HTTP_GET, uri.getPathAndQuery(), Poco::Net::HTTPRequest::HTTP_1_1);

    PrepareAndSendRequest(request, pocoRequest, clientSession.get(), ERequestBodyMode::None);

    Poco::Net::HTTPResponse pocoResponse;
    std::istream& responseStream = ReceiveResponse(clientSession.get(), pocoResponse, request);

    if (pocoResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_OK)
    {
        ProcessResponseAsync(*clientSession, pocoResponse, responseStream, request);
    }

    LogHttpResponseIfLoglevelVeryVerbose(m_logSystem, "GET", request, pocoResponse);
}

void POCOWebClient::Post(HttpRequest& request)
{
    CSP_PROFILE_SCOPED_FORMAT("POST %s", request.GetUri().GetAsStdString().c_str());

    Poco::URI uri(request.GetUri().GetAsStdString());

    // HTTPSessionFactory returns an unmanaged raw pointer - unique_ptr ensures the session is safely cleaned up after use.
    std::unique_ptr<Poco::Net::HTTPClientSession> clientSession(Poco::Net::HTTPSessionFactory::defaultFactory().createClientSession(uri));
    Poco::Net::HTTPRequest pocoRequest(Poco::Net::HTTPRequest::HTTP_POST, uri.getPathAndQuery(), Poco::Net::HTTPRequest::HTTP_1_1);

    if (!PrepareAndSendRequest(request, pocoRequest, clientSession.get(), ERequestBodyMode::Streamed))
    {
        return;
    }

    Poco::Net::HTTPResponse pocoResponse;
    std::istream& responseStream = ReceiveResponse(clientSession.get(), pocoResponse, request);

    std::string responseString;
    Poco::StreamCopier::copyToString(responseStream, responseString);
    request.SetResponseData(responseString.c_str(), responseString.length());
    auto& payload = ((HttpResponse&)request.GetResponse()).GetMutablePayload();

    // Get all response headers
    CopyResponseHeaders(pocoResponse, payload);

    LogHttpResponseIfLoglevelVeryVerbose(m_logSystem, "POST", request, pocoResponse);
}

void POCOWebClient::Put(HttpRequest& request)
{
    CSP_PROFILE_SCOPED_FORMAT("PUT %s", request.GetUri().GetAsStdString().c_str());

    Poco::URI uri(request.GetUri().GetAsStdString());

    // HTTPSessionFactory returns an unmanaged raw pointer - unique_ptr ensures the session is safely cleaned up after use.
    std::unique_ptr<Poco::Net::HTTPClientSession> clientSession(Poco::Net::HTTPSessionFactory::defaultFactory().createClientSession(uri));
    Poco::Net::HTTPRequest pocoRequest(Poco::Net::HTTPRequest::HTTP_PUT, uri.getPathAndQuery(), Poco::Net::HTTPRequest::HTTP_1_1);

    if (!PrepareAndSendRequest(request, pocoRequest, clientSession.get(), ERequestBodyMode::Streamed))
    {
        return;
    }

    Poco::Net::HTTPResponse pocoResponse;
    std::istream& responseStream = ReceiveResponse(clientSession.get(), pocoResponse, request);

    if (pocoResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_OK)
    {
        std::string responseString;
        Poco::StreamCopier::copyToString(responseStream, responseString);
        request.SetResponseData(responseString.c_str(), responseString.length());
    }

    LogHttpResponseIfLoglevelVeryVerbose(m_logSystem, "PUT", request, pocoResponse);
}

void POCOWebClient::Delete(HttpRequest& request)
{
    CSP_PROFILE_SCOPED_FORMAT("DELETE %s", request.GetUri().GetAsStdString().c_str());

    Poco::URI uri(request.GetUri().GetAsStdString());

    // HTTPSessionFactory returns an unmanaged raw pointer - unique_ptr ensures the session is safely cleaned up after use.
    std::unique_ptr<Poco::Net::HTTPClientSession> clientSession(Poco::Net::HTTPSessionFactory::defaultFactory().createClientSession(uri));
    Poco::Net::HTTPRequest pocoRequest(Poco::Net::HTTPRequest::HTTP_DELETE, uri.getPathAndQuery(), Poco::Net::HTTPRequest::HTTP_1_1);

    PrepareAndSendRequest(request, pocoRequest, clientSession.get(), ERequestBodyMode::Inline);

    Poco::Net::HTTPResponse pocoResponse;
    std::istream& responseStream = ReceiveResponse(clientSession.get(), pocoResponse, request);

    if (pocoResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_OK)
    {
        std::string responseString;
        Poco::StreamCopier::copyToString(responseStream, responseString);
        request.SetResponseData(responseString.c_str(), responseString.length());
    }

    LogHttpResponseIfLoglevelVeryVerbose(m_logSystem, "DELETE", request, pocoResponse);
}

void POCOWebClient::Head(HttpRequest& request)
{
    CSP_PROFILE_SCOPED_FORMAT("HEAD %s", request.GetUri().GetAsStdString().c_str());

    Poco::URI uri(request.GetUri().GetAsStdString());

    // HTTPSessionFactory returns an unmanaged raw pointer - unique_ptr ensures the session is safely cleaned up after use.
    std::unique_ptr<Poco::Net::HTTPClientSession> clientSession(Poco::Net::HTTPSessionFactory::defaultFactory().createClientSession(uri));
    Poco::Net::HTTPRequest pocoRequest(Poco::Net::HTTPRequest::HTTP_HEAD, uri.getPathAndQuery(), Poco::Net::HTTPRequest::HTTP_1_1);

    PrepareAndSendRequest(request, pocoRequest, clientSession.get(), ERequestBodyMode::None);

    Poco::Net::HTTPResponse pocoResponse;
    std::istream& responseStream = ReceiveResponse(clientSession.get(), pocoResponse, request);

    if (pocoResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_OK)
    {
        ProcessResponseAsync(*clientSession, pocoResponse, responseStream, request);
    }

    LogHttpResponseIfLoglevelVeryVerbose(m_logSystem, "HEAD", request, pocoResponse);
}

void POCOWebClient::Patch(HttpRequest& request)
{
    CSP_PROFILE_SCOPED_FORMAT("PATCH %s", request.GetUri().GetAsStdString().c_str());

    Poco::URI uri(request.GetUri().GetAsStdString());

    // HTTPSessionFactory returns an unmanaged raw pointer - unique_ptr ensures the session is safely cleaned up after use.
    std::unique_ptr<Poco::Net::HTTPClientSession> clientSession(Poco::Net::HTTPSessionFactory::defaultFactory().createClientSession(uri));
    Poco::Net::HTTPRequest pocoRequest(Poco::Net::HTTPRequest::HTTP_PATCH, uri.getPathAndQuery(), Poco::Net::HTTPRequest::HTTP_1_1);

    if (!PrepareAndSendRequest(request, pocoRequest, clientSession.get(), ERequestBodyMode::Streamed))
    {
        return;
    }

    Poco::Net::HTTPResponse pocoResponse;
    std::istream& responseStream = ReceiveResponse(clientSession.get(), pocoResponse, request);

    std::string responseString;
    Poco::StreamCopier::copyToString(responseStream, responseString);
    request.SetResponseData(responseString.c_str(), responseString.length());
    auto& payload = ((HttpResponse&)request.GetResponse()).GetMutablePayload();

    // Get all response headers
    CopyResponseHeaders(pocoResponse, payload);

    LogHttpResponseIfLoglevelVeryVerbose(m_logSystem, "PATCH", request, pocoResponse);
}

void POCOWebClient::ProcessResponseAsync(
    Poco::Net::HTTPClientSession& clientSession, Poco::Net::HTTPResponse& pocoResponse, std::istream& responseStream, HttpRequest& request)
{
    CSP_PROFILE_SCOPED();

    std::streamsize contentLength = pocoResponse.getContentLength();

    if (contentLength == Poco::Net::HTTPMessage::UNKNOWN_CONTENT_LENGTH || contentLength == 0)
    {
        return;
    }

    std::streamsize totalRead = 0;

    request.AllocateResponseData(contentLength);

    char buffer[kPOCOAsyncBufferSize];

    while (responseStream.good() && totalRead < contentLength)
    {
        if (request.Cancelled())
        {
            clientSession.abort();
            return;
        }

        responseStream.read(buffer, sizeof(buffer));
        std::streamsize read = responseStream.gcount();
        request.WriteResponseData(totalRead, buffer, read);

        totalRead += read;
        float progress = 100.0f * static_cast<float>(totalRead) / contentLength;
        // RWD_FORMATTED_LOG("Response Progress %f %d %d\n", Progress, TotalRead, ContentLength);
        request.SetResponseProgress(progress);
    }

    auto& response = request.GetResponse();
    auto& payload = ((HttpResponse&)response).GetMutablePayload();

    // Get all response headers
    for (auto iter = pocoResponse.begin(); iter != pocoResponse.end(); ++iter)
    {
        std::string key = iter->first;
        std::string val = iter->second;

        // Make Key and Val lower-case
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(val.begin(), val.end(), val.begin(), [](unsigned char c) { return std::tolower(c); });

        payload.AddHeader(key.c_str(), val.c_str());
    }
}

void POCOWebClient::ProcessRequestAsync(
    Poco::Net::HTTPClientSession& clientSession, Poco::Net::HTTPRequest& /*PocoRequest*/, std::ostream& requestStream, HttpRequest& request)
{
    CSP_PROFILE_SCOPED();

    size_t contentLength = request.GetPayload().GetContent().Length();

    if (contentLength == 0)
    {
        return;
    }

    size_t totalWritten = 0;

    char buffer[kPOCOAsyncBufferSize];

    while (requestStream.good() && totalWritten < contentLength)
    {
        if (request.Cancelled())
        {
            clientSession.abort();
            return;
        }

        size_t length = request.GetPayload().ReadContent(totalWritten, buffer, sizeof(buffer));

        requestStream.write(buffer, length);
        totalWritten += length;

        float progress = 100.0f * static_cast<float>(totalWritten) / contentLength;
        // RWD_FORMATTED_LOG("Request Progress %f %d %d\n", Progress, TotalWritten, ContentLength);
        request.SetRequestProgress(progress);
    }
}

std::string POCOWebClient::MD5Hash(const void* data, const size_t size)
{
    Poco::MD5Engine mD5Hasher;

    mD5Hasher.update(data, size);

    const Poco::DigestEngine::Digest& digest = mD5Hasher.digest();

    std::string digestHex = Poco::DigestEngine::digestToHex(digest);
    return digestHex;
}

void POCOWebClient::SetFileUploadContentFromFile(
    HttpPayload* payload, const char* filePath, const char* version, const csp::common::String& mediaType)
{
    Poco::File* file = new Poco::File(filePath);
    if (file->exists())
    {
        /// @note this must be newed as it is deleted by the HTMLForm
        Poco::Net::FilePartSource* source = new Poco::Net::FilePartSource(filePath, mediaType.c_str());
        SetFileUploadContent(payload, source, version);
    }
    else
    {
        CSP_LOG_WARN_FORMAT("File not found. Path given: %s", filePath);
    }
}

void POCOWebClient::SetFileUploadContentFromString(HttpPayload* payload, const csp::common::String& stringSource, const csp::common::String& fileName,
    const char* version, const csp::common::String& mediaType)
{
    std::string str(stringSource.c_str(), stringSource.Length());
    /// @note this must be newed as it is deleted by the HTMLForm
    Poco::Net::StringPartSource* source = new Poco::Net::StringPartSource(str, mediaType.c_str(), fileName.c_str());
    SetFileUploadContent(payload, source, version);
}

// This function is deliberately written this way to reduce the number of allocations and string copying. Do not change it!
void POCOWebClient::SetFileUploadContentFromBuffer(HttpPayload* payload, const char* buffer, size_t bufferLength, const csp::common::String& fileName,
    const char* /*Version*/, const csp::common::String& mediaType)
{
    constexpr const char boundary[] = "MIME_boundary_FileFromBuffer";
    constexpr size_t boundaryLen = CStringLength(boundary);
    constexpr const char contentContentDisposition[] = "\r\nContent-Disposition: form-data; name=\"FormFile\"; filename=\"";
    constexpr size_t contentContentDispositionLen = CStringLength(contentContentDisposition);
    constexpr const char contentContentType[] = "\"\r\nContent-Type: ";
    constexpr size_t contentContentTypeLen = CStringLength(contentContentType);

    payload->SetBoundary(boundary);

    auto contentLength = 2 + boundaryLen + contentContentDispositionLen + fileName.Length() + contentContentTypeLen + mediaType.Length() + 4
        + bufferLength + 4 + boundaryLen + 2;

    payload->AllocateContent(contentLength);

    size_t i = 0;

    payload->WriteContent(i, "--", 2);
    i += 2;
    payload->WriteContent(i, boundary, boundaryLen);
    i += boundaryLen;
    payload->WriteContent(i, contentContentDisposition, contentContentDispositionLen);
    i += contentContentDispositionLen;
    payload->WriteContent(i, fileName.c_str(), fileName.Length());
    i += fileName.Length();
    payload->WriteContent(i, contentContentType, contentContentTypeLen);
    i += contentContentTypeLen;
    payload->WriteContent(i, mediaType.c_str(), mediaType.Length());
    i += mediaType.Length();
    payload->WriteContent(i, "\r\n\r\n", 4);
    i += 4;
    payload->WriteContent(i, buffer, bufferLength);
    i += bufferLength;
    payload->WriteContent(i, "\r\n--", 4);
    i += 4;
    payload->WriteContent(i, boundary, boundaryLen);
    i += boundaryLen;
    payload->WriteContent(i, "--", 2);
    i += 2;
}

void POCOWebClient::SetFileUploadContent(HttpPayload* payload, Poco::Net::PartSource* source, const char* version)
{
    Poco::Net::HTMLForm form("multipart/form-data");
    Poco::MD5Engine mD5Hasher;

    source->stream().seekg(0);
    std::string fileAsString(std::istreambuf_iterator<char>(source->stream()), {});
    mD5Hasher.update(fileAsString);

    const Poco::DigestEngine::Digest& digest = mD5Hasher.digest();

    std::string digestHex = Poco::DigestEngine::digestToHex(digest);

    form.add("Checksum", digestHex);
    form.add("Version", version);

    form.addPart("FormFile", source);

    source->stream().seekg(0);
    std::stringstream strm;
    form.write(strm);

    payload->SetContent(strm.str().c_str(), strm.str().size());
    payload->SetBoundary(CSP_TEXT(form.boundary().c_str()));
}

} // namespace csp::web
#endif
