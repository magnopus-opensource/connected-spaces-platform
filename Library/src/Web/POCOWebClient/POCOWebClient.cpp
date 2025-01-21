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
#include "POCOWebClient.h"

#include "Debug/Logging.h"

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

} // namespace

namespace csp::web
{

/// @brief Size of stack space used for async streaming upload and download
const uint32_t kPOCOAsyncBufferSize = 2 * 1024;

EResponseCodes GetOlyResponseCode(Poco::Net::HTTPResponse::HTTPStatus PocoResponseCode) { return (EResponseCodes)PocoResponseCode; }

POCOWebClient::POCOWebClient(const Port InPort, const ETransferProtocol Tp, bool AutoRefresh)
    : WebClient(InPort, Tp, AutoRefresh)
{
    Poco::Net::initializeSSL();

    auto CertHandler = Poco::makeShared<Poco::Net::AcceptCertificateHandler>(false);
    auto PrivateKeyHandler = Poco::makeShared<PocoPrivateKeyHandler>(false);

    PocoContext
        = Poco::makeAuto<Poco::Net::Context>(Poco::Net::Context::CLIENT_USE, "", Poco::Net::Context::VerificationMode::VERIFY_RELAXED, 9, true);

    Poco::Net::HTTPSessionFactory::defaultFactory().registerProtocol("http", new Poco::Net::HTTPSessionInstantiator);
    Poco::Net::HTTPSessionFactory::defaultFactory().registerProtocol("https", new Poco::Net::HTTPSSessionInstantiator);

    // TODO: Get rid of singleton usage entirely. Until then, we can't create multiple instances of Connected Spaces Platform.
    Poco::Net::SSLManager::instance().initializeClient(PrivateKeyHandler, CertHandler, PocoContext);

    Cookies = new std::remove_pointer_t<decltype(Cookies)>();
}

POCOWebClient::~POCOWebClient() { delete Cookies; }

void POCOWebClient::Send(HttpRequest& Request)
{
    try
    {
        const ERequestVerb Verb = Request.GetVerb();

        switch (Verb)
        {
        case ERequestVerb::Get:
            Get(Request);
            break;
        case ERequestVerb::Post:
            Post(Request);
            break;
        case ERequestVerb::Put:
            Put(Request);
            break;
        case ERequestVerb::Delete:
            Delete(Request);
            break;
        case ERequestVerb::Head:
            Head(Request);
            break;
        default:
            break;
        }
    }
    catch (const Poco::Exception& Ex)
    {
        throw WebClientException(Ex.displayText());
    }
}

void POCOWebClient::Get(HttpRequest& Request)
{
    CSP_PROFILE_SCOPED_FORMAT("GET %s", Request.GetUri().GetAsStdString().c_str());

    Poco::URI Uri(Request.GetUri().GetAsStdString());

    Poco::Net::HTTPClientSession* ClientSession = Poco::Net::HTTPSessionFactory::defaultFactory().createClientSession(Uri);
    Poco::Net::HTTPRequest PocoRequest(Poco::Net::HTTPRequest::HTTP_GET, Uri.getPathAndQuery(), Poco::Net::HTTPRequest::HTTP_1_1);

    for (auto Header : Request.GetPayload().GetHeaders())
    {
        PocoRequest.add(Header.first.c_str(), Header.second.c_str());
    }

    AddCookie(PocoRequest);

    ClientSession->sendRequest(PocoRequest);

    Poco::Net::HTTPResponse PocoResponse;
    std::istream& ResponseStream = ClientSession->receiveResponse(PocoResponse);
    Request.SetResponseCode(GetOlyResponseCode(PocoResponse.getStatus()));

    {
        std::scoped_lock Lock(CookiesMutex);

        PocoResponse.getCookies(*Cookies);
    }

    if (PocoResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_OK)
    {
        ProcessResponseAsync(*ClientSession, PocoResponse, ResponseStream, Request);
    }
}

void POCOWebClient::AddCookie(Poco::Net::HTTPRequest& PocoRequest)
{
    {
        std::scoped_lock Lock(CookiesMutex);

        if (Cookies->size() == 0)
        {
            return;
        }
    }

    // create a collection to build up with Cookies to attach to the request
    Poco::Net::NameValueCollection CookieCollection = Poco::Net::NameValueCollection();

    // Iterate our stored cookies and convert into a collection entry to attach to the request
    {
        std::scoped_lock Lock(CookiesMutex);

        for (const Poco::Net::HTTPCookie& Cookie : *Cookies)
        {
            CookieCollection.add(Cookie.getName(), Cookie.getValue());
        }
    }

    // Set the cookies on our request
    PocoRequest.setCookies(CookieCollection);
}

void POCOWebClient::Post(HttpRequest& Request)
{
    CSP_PROFILE_SCOPED_FORMAT("POST %s", Request.GetUri().GetAsStdString().c_str());

    Poco::URI Uri(Request.GetUri().GetAsStdString());

    Poco::Net::HTTPClientSession* ClientSession = Poco::Net::HTTPSessionFactory::defaultFactory().createClientSession(Uri);
    Poco::Net::HTTPRequest PocoRequest(Poco::Net::HTTPRequest::HTTP_POST, Uri.getPathAndQuery(), Poco::Net::HTTPRequest::HTTP_1_1);

    for (auto Header : Request.GetPayload().GetHeaders())
    {
        PocoRequest.add(Header.first.c_str(), Header.second.c_str());
    }

    AddCookie(PocoRequest);

    size_t ContentLength = Request.GetPayload().GetContent().Length();
    PocoRequest.setContentLength(ContentLength);
    std::ostream& RequestStream = ClientSession->sendRequest(PocoRequest);
    ProcessRequestAsync(*ClientSession, PocoRequest, RequestStream, Request);

    if (Request.Cancelled())
    {
        return;
    }

    Poco::Net::HTTPResponse PocoResponse;
    std::istream& ResponseStream = ClientSession->receiveResponse(PocoResponse);
    Request.SetResponseCode(GetOlyResponseCode(PocoResponse.getStatus()));

    {
        std::scoped_lock Lock(CookiesMutex);

        PocoResponse.getCookies(*Cookies);
    }

    std::string ResponseString;
    Poco::StreamCopier::copyToString(ResponseStream, ResponseString);
    Request.SetResponseData(ResponseString.c_str(), ResponseString.length());
    auto& Payload = ((HttpResponse&)Request.GetResponse()).GetMutablePayload();

    // Get all response headers
    for (auto Iter = PocoResponse.begin(); Iter != PocoResponse.end(); ++Iter)
    {
        std::string Key = Iter->first;
        std::string Val = Iter->second;

        // Make Key and Val lower-case
        std::transform(Key.begin(), Key.end(), Key.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(Val.begin(), Val.end(), Val.begin(), [](unsigned char c) { return std::tolower(c); });

        Payload.AddHeader(Key.c_str(), Val.c_str());
    }
}

void POCOWebClient::Put(HttpRequest& Request)
{
    CSP_PROFILE_SCOPED_FORMAT("PUT %s", Request.GetUri().GetAsStdString().c_str());

    Poco::URI Uri(Request.GetUri().GetAsStdString());

    Poco::Net::HTTPClientSession* ClientSession = Poco::Net::HTTPSessionFactory::defaultFactory().createClientSession(Uri);
    Poco::Net::HTTPRequest PocoRequest(Poco::Net::HTTPRequest::HTTP_PUT, Uri.getPathAndQuery(), Poco::Net::HTTPRequest::HTTP_1_1);

    for (auto Header : Request.GetPayload().GetHeaders())
    {
        PocoRequest.add(Header.first.c_str(), Header.second.c_str());
    }

    AddCookie(PocoRequest);

    size_t ContentLength = Request.GetPayload().GetContent().Length();
    PocoRequest.setContentLength(ContentLength);
    std::ostream& RequestStream = ClientSession->sendRequest(PocoRequest);
    ProcessRequestAsync(*ClientSession, PocoRequest, RequestStream, Request);

    if (Request.Cancelled())
    {
        return;
    }

    Poco::Net::HTTPResponse PocoResponse;
    std::istream& ResponseStream = ClientSession->receiveResponse(PocoResponse);
    Request.SetResponseCode(GetOlyResponseCode(PocoResponse.getStatus()));

    {
        std::scoped_lock Lock(CookiesMutex);

        PocoResponse.getCookies(*Cookies);
    }

    if (PocoResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_OK)
    {
        std::string ResponseString;
        Poco::StreamCopier::copyToString(ResponseStream, ResponseString);
        Request.SetResponseData(ResponseString.c_str(), ResponseString.length());
    }
}

void POCOWebClient::Delete(HttpRequest& Request)
{
    CSP_PROFILE_SCOPED_FORMAT("DELETE %s", Request.GetUri().GetAsStdString().c_str());

    Poco::URI Uri(Request.GetUri().GetAsStdString());

    Poco::Net::HTTPClientSession* ClientSession = Poco::Net::HTTPSessionFactory::defaultFactory().createClientSession(Uri);
    Poco::Net::HTTPRequest PocoRequest(Poco::Net::HTTPRequest::HTTP_DELETE, Uri.getPathAndQuery(), Poco::Net::HTTPRequest::HTTP_1_1);

    for (auto Header : Request.GetPayload().GetHeaders())
    {
        PocoRequest.add(Header.first.c_str(), Header.second.c_str());
    }

    AddCookie(PocoRequest);

    const std::string Body(Request.GetPayload().GetContent().c_str());
    PocoRequest.setContentLength(Body.length());
    ClientSession->sendRequest(PocoRequest) << Body;

    Poco::Net::HTTPResponse PocoResponse;
    std::istream& ResponseStream = ClientSession->receiveResponse(PocoResponse);
    Request.SetResponseCode(GetOlyResponseCode(PocoResponse.getStatus()));

    {
        std::scoped_lock Lock(CookiesMutex);

        PocoResponse.getCookies(*Cookies);
    }

    if (PocoResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_OK)
    {
        std::string ResponseString;
        Poco::StreamCopier::copyToString(ResponseStream, ResponseString);
        Request.SetResponseData(ResponseString.c_str(), ResponseString.length());
    }
}

void POCOWebClient::Head(HttpRequest& Request)
{
    CSP_PROFILE_SCOPED_FORMAT("HEAD %s", Request.GetUri().GetAsStdString().c_str());

    Poco::URI Uri(Request.GetUri().GetAsStdString());

    Poco::Net::HTTPClientSession* ClientSession = Poco::Net::HTTPSessionFactory::defaultFactory().createClientSession(Uri);
    Poco::Net::HTTPRequest PocoRequest(Poco::Net::HTTPRequest::HTTP_HEAD, Uri.getPathAndQuery(), Poco::Net::HTTPRequest::HTTP_1_1);

    for (auto Header : Request.GetPayload().GetHeaders())
    {
        PocoRequest.add(Header.first.c_str(), Header.second.c_str());
    }

    AddCookie(PocoRequest);

    ClientSession->sendRequest(PocoRequest);

    Poco::Net::HTTPResponse PocoResponse;
    std::istream& ResponseStream = ClientSession->receiveResponse(PocoResponse);
    Request.SetResponseCode(GetOlyResponseCode(PocoResponse.getStatus()));

    {
        std::scoped_lock Lock(CookiesMutex);

        PocoResponse.getCookies(*Cookies);
    }

    if (PocoResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_OK)
    {
        ProcessResponseAsync(*ClientSession, PocoResponse, ResponseStream, Request);
    }
}

void POCOWebClient::ProcessResponseAsync(
    Poco::Net::HTTPClientSession& ClientSession, Poco::Net::HTTPResponse& PocoResponse, std::istream& ResponseStream, HttpRequest& Request)
{
    CSP_PROFILE_SCOPED();

    std::streamsize ContentLength = PocoResponse.getContentLength();

    if (ContentLength == Poco::Net::HTTPMessage::UNKNOWN_CONTENT_LENGTH || ContentLength == 0)
    {
        return;
    }

    std::streamsize TotalRead = 0;

    Request.AllocateResponseData(ContentLength);

    char Buffer[kPOCOAsyncBufferSize];

    while (ResponseStream.good() && TotalRead < ContentLength)
    {
        if (Request.Cancelled())
        {
            ClientSession.abort();
            return;
        }

        ResponseStream.read(Buffer, sizeof(Buffer));
        std::streamsize Read = ResponseStream.gcount();
        Request.WriteResponseData(TotalRead, Buffer, Read);

        TotalRead += Read;
        float Progress = 100.0f * static_cast<float>(TotalRead) / ContentLength;
        // RWD_FORMATTED_LOG("Response Progress %f %d %d\n", Progress, TotalRead, ContentLength);
        Request.SetResponseProgress(Progress);
    }

    auto& Response = Request.GetResponse();
    auto& Payload = ((HttpResponse&)Response).GetMutablePayload();

    // Get all response headers
    for (auto Iter = PocoResponse.begin(); Iter != PocoResponse.end(); ++Iter)
    {
        std::string Key = Iter->first;
        std::string Val = Iter->second;

        // Make Key and Val lower-case
        std::transform(Key.begin(), Key.end(), Key.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(Val.begin(), Val.end(), Val.begin(), [](unsigned char c) { return std::tolower(c); });

        Payload.AddHeader(Key.c_str(), Val.c_str());
    }
}

void POCOWebClient::ProcessRequestAsync(
    Poco::Net::HTTPClientSession& ClientSession, Poco::Net::HTTPRequest& PocoRequest, std::ostream& RequestStream, HttpRequest& Request)
{
    CSP_PROFILE_SCOPED();

    size_t ContentLength = Request.GetPayload().GetContent().Length();

    if (ContentLength == 0)
    {
        return;
    }

    size_t TotalWritten = 0;

    char buffer[kPOCOAsyncBufferSize];

    while (RequestStream.good() && TotalWritten < ContentLength)
    {
        if (Request.Cancelled())
        {
            ClientSession.abort();
            return;
        }

        size_t Length = Request.GetPayload().ReadContent(TotalWritten, buffer, sizeof(buffer));

        RequestStream.write(buffer, Length);
        TotalWritten += Length;

        float Progress = 100.0f * static_cast<float>(TotalWritten) / ContentLength;
        // RWD_FORMATTED_LOG("Request Progress %f %d %d\n", Progress, TotalWritten, ContentLength);
        Request.SetRequestProgress(Progress);
    }
}

std::string POCOWebClient::MD5Hash(const void* Data, const size_t Size)
{
    Poco::MD5Engine MD5Hasher;

    MD5Hasher.update(Data, Size);

    const Poco::DigestEngine::Digest& Digest = MD5Hasher.digest();

    std::string DigestHex = Poco::DigestEngine::digestToHex(Digest);
    return DigestHex;
}

void POCOWebClient::SetFileUploadContentFromFile(
    HttpPayload* Payload, const char* FilePath, const char* Version, const csp::common::String& MediaType)
{
    Poco::File* File = new Poco::File(FilePath);
    if (File->exists())
    {
        /// @note this must be newed as it is deleted by the HTMLForm
        Poco::Net::FilePartSource* Source = new Poco::Net::FilePartSource(FilePath, MediaType.c_str());
        SetFileUploadContent(Payload, Source, Version);
    }
    else
    {
        CSP_LOG_WARN_FORMAT("File not found. Path given: %s", FilePath);
    }
}

void POCOWebClient::SetFileUploadContentFromString(HttpPayload* Payload, const csp::common::String& StringSource, const csp::common::String& FileName,
    const char* Version, const csp::common::String& MediaType)
{
    std::string str(StringSource.c_str(), StringSource.Length());
    /// @note this must be newed as it is deleted by the HTMLForm
    Poco::Net::StringPartSource* Source = new Poco::Net::StringPartSource(str, MediaType.c_str(), FileName.c_str());
    SetFileUploadContent(Payload, Source, Version);
}

// This function is deliberately written this way to reduce the number of allocations and string copying. Do not change it!
void POCOWebClient::SetFileUploadContentFromBuffer(HttpPayload* Payload, const char* Buffer, size_t BufferLength, const csp::common::String& FileName,
    const char* Version, const csp::common::String& MediaType)
{
    constexpr const char Boundary[] = "MIME_boundary_FileFromBuffer";
    constexpr size_t BoundaryLen = CStringLength(Boundary);
    constexpr const char Content_ContentDisposition[] = "\r\nContent-Disposition: form-data; name=\"FormFile\"; filename=\"";
    constexpr size_t Content_ContentDispositionLen = CStringLength(Content_ContentDisposition);
    constexpr const char Content_ContentType[] = "\"\r\nContent-Type: ";
    constexpr size_t Content_ContentTypeLen = CStringLength(Content_ContentType);

    Payload->SetBoundary(Boundary);

    auto ContentLength = 2 + BoundaryLen + Content_ContentDispositionLen + FileName.Length() + Content_ContentTypeLen + MediaType.Length() + 4
        + BufferLength + 4 + BoundaryLen + 2;

    Payload->AllocateContent(ContentLength);

    size_t i = 0;

    Payload->WriteContent(i, "--", 2);
    i += 2;
    Payload->WriteContent(i, Boundary, BoundaryLen);
    i += BoundaryLen;
    Payload->WriteContent(i, Content_ContentDisposition, Content_ContentDispositionLen);
    i += Content_ContentDispositionLen;
    Payload->WriteContent(i, FileName.c_str(), FileName.Length());
    i += FileName.Length();
    Payload->WriteContent(i, Content_ContentType, Content_ContentTypeLen);
    i += Content_ContentTypeLen;
    Payload->WriteContent(i, MediaType.c_str(), MediaType.Length());
    i += MediaType.Length();
    Payload->WriteContent(i, "\r\n\r\n", 4);
    i += 4;
    Payload->WriteContent(i, Buffer, BufferLength);
    i += BufferLength;
    Payload->WriteContent(i, "\r\n--", 4);
    i += 4;
    Payload->WriteContent(i, Boundary, BoundaryLen);
    i += BoundaryLen;
    Payload->WriteContent(i, "--", 2);
    i += 2;
}

void POCOWebClient::SetFileUploadContent(HttpPayload* Payload, Poco::Net::PartSource* Source, const char* Version)
{
    Poco::Net::HTMLForm Form("multipart/form-data");
    Poco::MD5Engine MD5Hasher;

    Source->stream().seekg(0);
    std::string FileAsString(std::istreambuf_iterator<char>(Source->stream()), {});
    MD5Hasher.update(FileAsString);

    const Poco::DigestEngine::Digest& Digest = MD5Hasher.digest();

    std::string DigestHex = Poco::DigestEngine::digestToHex(Digest);

    Form.add("Checksum", DigestHex);
    Form.add("Version", Version);

    Form.addPart("FormFile", Source);

    Source->stream().seekg(0);
    std::stringstream strm;
    Form.write(strm);

    Payload->SetContent(strm.str().c_str(), strm.str().size());
    Payload->SetBoundary(CSP_TEXT(Form.boundary().c_str()));
}

} // namespace csp::web
