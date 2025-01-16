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

#include "EmscriptenWebClient.h"

#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"
#include "Debug/Logging.h"

#include <assert.h>
#include <emscripten/emscripten.h>
#include <emscripten/fetch.h>
#include <iostream>
#include <sstream>

namespace
{

void GetResponseHeaders(emscripten_fetch_t* Fetch, csp::common::Map<csp::common::String, csp::common::String>& OutHeadersMap)
{
    auto Length = emscripten_fetch_get_response_headers_length(Fetch);
    auto Buf = CSP_NEW char[Length + 1];
    emscripten_fetch_get_response_headers(Fetch, Buf, Length + 1);
    auto Headers = emscripten_fetch_unpack_response_headers(Buf);

    for (int i = 0; Headers[i] != nullptr; i += 2)
    {
        csp::common::String _Key = Headers[i];
        csp::common::String _Val = Headers[i + 1];

        auto Key = _Key.Trim();
        auto Val = _Val.Trim();

        OutHeadersMap[Key] = Val;
    }

    emscripten_fetch_free_unpacked_response_headers(Headers);
}

void OnFetchSuccessOrError(emscripten_fetch_t* Fetch)
{
    auto* Request = reinterpret_cast<csp::web::HttpRequest*>(Fetch->userData);
    Request->SetResponseCode(static_cast<csp::web::EResponseCodes>(Fetch->status));

    if (Fetch->numBytes)
    {
        Request->SetResponseData(Fetch->data, Fetch->numBytes);
    }

    csp::common::Map<csp::common::String, csp::common::String> Headers;
    GetResponseHeaders(Fetch, Headers);

    auto& Response = Request->GetMutableResponse();
    auto& Payload = Response.GetMutablePayload();
    auto* Keys = Headers.Keys();

    for (int i = 0; i < Keys->Size(); ++i)
    {
        auto Key = Keys->operator[](i).ToLower();
        auto Value = Headers[Key].ToLower();

        Payload.AddHeader(Key, Value);
    }

    CSP_DELETE(Keys);

    Request->GetCallback()->OnHttpResponse(Response);
    // DO NOT DO THIS! This will delete Payload.Content twice
    // CSP_DELETE_ARRAY(Fetch->__attributes.requestData);
    CSP_DELETE(Request);
    emscripten_fetch_close(Fetch);
}

void OnFetchError(emscripten_fetch_t* Fetch)
{
    auto* Request = reinterpret_cast<csp::web::HttpRequest*>(Fetch->userData);

    if (Request->Retry())
    {
        CSP_LOG_WARN_MSG("Retrying failed emscripten request\n");
    }
    else
    {
        OnFetchSuccessOrError(Fetch);
    }
}

void OnFetchProgress(emscripten_fetch_t* Fetch)
{
    if (Fetch->totalBytes)
    {
        auto* Request = reinterpret_cast<csp::web::HttpRequest*>(Fetch->userData);
        Request->SetResponseProgress(Fetch->dataOffset * 100.0 / Fetch->totalBytes);
    }
}

template <size_t N> constexpr size_t CStringLength(char const (&)[N]) { return N - 1; }

} // namespace

namespace csp::web
{

EmscriptenWebClient::EmscriptenWebClient(const Port InPort, const ETransferProtocol Tp, bool AutoRefresh)
    : WebClient(InPort, Tp, AutoRefresh)
{
    std::srand(std::time(nullptr));
}

std::string EmscriptenWebClient::MD5Hash(const void* Data, const size_t Size) { assert(false && "Not implemented!"); }

void EmscriptenWebClient::SetFileUploadContentFromFile(
    HttpPayload* Payload, const char* FilePath, const char* Version, const csp::common::String& MediaType)
{
    assert(false && "Not implemented!");
}

void EmscriptenWebClient::SetFileUploadContentFromString(HttpPayload* Payload, const csp::common::String& StringSource,
    const csp::common::String& FileName, const char* Version, const csp::common::String& MediaType)
{
    std::ostringstream strm;
    strm << "MIME_boundary_" << std::rand();
    std::string boundary = strm.str();

    strm.seekp(0);
    strm << "--" << boundary << "\r\nContent-Disposition: form-data; name=\"FormFile\"; filename=\"" << FileName.c_str()
         << "\"\r\nContent-Type: " << MediaType << "\r\n\r\n"
         << std::string(StringSource.c_str(), StringSource.Length()) << "\r\n--" << boundary << "--";
    std::string content = strm.str();

    Payload->SetContent(content.c_str(), content.length());
    Payload->SetBoundary(boundary.c_str());
}

// This function is deliberately written this way to reduce the number of allocations and string copying. Do not change it!
void EmscriptenWebClient::SetFileUploadContentFromBuffer(HttpPayload* Payload, const char* Buffer, size_t BufferLength,
    const csp::common::String& FileName, const char* Version, const csp::common::String& MediaType)
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

    auto i = 0;

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

void EmscriptenWebClient::Send(HttpRequest& Request)
{
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);

    switch (Request.GetVerb())
    {
    case ERequestVerb::Get:
        strcpy(attr.requestMethod, "GET");
        break;
    case ERequestVerb::Post:
        strcpy(attr.requestMethod, "POST");
        break;
    case ERequestVerb::Put:
        strcpy(attr.requestMethod, "PUT");
        break;
    case ERequestVerb::Delete:
        strcpy(attr.requestMethod, "DELETE");
        break;
    case ERequestVerb::Head:
        strcpy(attr.requestMethod, "HEAD");
        break;
    }

    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = OnFetchSuccessOrError;
    attr.onerror = OnFetchError;
    attr.onprogress = OnFetchProgress;

    // Don't send headers or content for HEAD requests
    if (Request.GetVerb() != ERequestVerb::Head)
    {
        auto& Payload = Request.GetPayload();
        auto& Headers = Payload.GetHeaders();
        int i = 0;
        auto** HeadersBuf = CSP_NEW char * [Headers.size() * 2 + 1];

        for (auto& Header : Headers)
        {
            auto Len = Header.first.size();
            auto* Key = CSP_NEW char[Len + 1];
            strcpy(Key, Header.first.c_str());

            HeadersBuf[i++] = Key;

            Len = Header.second.size();
            auto* Val = CSP_NEW char[Len + 1];
            strcpy(Val, Header.second.c_str());

            HeadersBuf[i++] = Val;
        }

        HeadersBuf[i] = nullptr;

        attr.requestHeaders = HeadersBuf;

        auto& Content = Payload.GetContent();
        auto Len = Content.Length();

        if (Len)
        {
            attr.requestData = Content.c_str();
            attr.requestDataSize = Len;
        }
    }

    attr.userData = &Request;

    auto Uri = Request.GetUri().GetAsStdString();
    emscripten_fetch(&attr, Uri.c_str());
}

} // namespace csp::web
