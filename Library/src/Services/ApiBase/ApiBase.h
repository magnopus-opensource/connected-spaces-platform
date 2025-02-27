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

#include "CSP/Systems/WebService.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Services/DtoBase/DtoBase.h"
#include "Web/HttpResponse.h"
#include "Web/Json.h"
#include "Web/WebClient.h"

#include <async++.h>
#include <list>
#include <memory>
#include <vector>

namespace csp::services
{

enum class EResponseCode : uint8_t
{
    ResponseSuccess,
    ResponseFailed
};

/// @brief Abstract base class for an Api Response
///
/// Base class for Api Responses which handle the translation of Json returned
/// from an api call to a Dto
class ApiResponseBase
{
public:
    ApiResponseBase(DtoBase* InDto);
    virtual ~ApiResponseBase() = default;

    EResponseCode GetResponseCode() const;
    DtoBase* GetDto() const;

    void SetResponse(csp::web::HttpResponse* Response);
    csp::web::HttpResponse* GetMutableResponse();
    const csp::web::HttpResponse* GetResponse() const;

    void SetResponseCode(csp::web::EResponseCodes InResponseCode, csp::web::EResponseCodes InValidResponseCode);

protected:
    EResponseCode ResponseCode = EResponseCode::ResponseFailed;
    csp::web::EResponseCodes HttpResponseCode = csp::web::EResponseCodes::ResponseInit;
    DtoBase* Dto;
    csp::web::HttpResponse* Response = nullptr;

private:
    bool IsValidResponseCode(int ResponseCodeA, int ResponseCodeB);
};

/// @brief Type for returning Array of DTO objects from a web api call
/// @tparam DtoType
template <class DtoType> class DtoArray : public DtoBase
{
public:
    DtoArray() { }

    virtual ~DtoArray() { }

    virtual utility::string_t ToJson() const override { return utility::string_t(""); }

    virtual void FromJson(const utility::string_t& Json) override
    {
        rapidjson::Document JsonDoc;

        if (Json.c_str() == nullptr)
        {
            return;
        }

        JsonDoc.Parse(Json.c_str());

        if (JsonDoc.IsArray())
        {
            Array.resize(JsonDoc.Size());

            for (rapidjson::SizeType i = 0; i < JsonDoc.Size(); i++)
            {
                rapidjson::Value& Val = JsonDoc[i];

                DtoType& Dto = Array[i];
                Dto.FromJson(csp::web::JsonObjectToString(Val));
            }
        }
        else if (JsonDoc.HasMember("items"))
        {
            if (!JsonDoc["items"].IsArray())
            {
                return;
            }

            Array.resize(JsonDoc["items"].Size());

            for (rapidjson::SizeType i = 0; i < JsonDoc["items"].Size(); i++)
            {
                rapidjson::Value& Val = JsonDoc["items"][i];

                DtoType& Dto = Array[i];
                Dto.FromJson(csp::web::JsonObjectToString(Val));
            }
        }
    }

    const std::vector<DtoType>& GetArray() const { return Array; }

    std::vector<DtoType>& GetArray() { return Array; }

private:
    std::vector<DtoType> Array;
};

/// @brief Templated class to handle response codes
///
/// Defines how response codes are handled by different Dto types
/// @tparam DtoType
template <class DtoType> class ApiResponse : public ApiResponseBase
{
public:
    ApiResponse();
    virtual ~ApiResponse();
};

class ApiResponseHandlerBase : public csp::web::IHttpResponseHandler
{
public:
    ApiResponseHandlerBase();
    virtual ~ApiResponseHandlerBase();

    virtual void OnHttpProgress(csp::web::HttpRequest& Request) override = 0;
    virtual void OnHttpResponse(csp::web::HttpResponse& Response) override = 0;

    // Make sure these get deleted with the request
    bool ShouldDelete() const override { return true; }
};

/// @brief Templated api response handler
///
/// Handles the translation of HttpResponse content in Json format
/// into Dto objects which are then passed to the callback
///
/// @tparam CallbackType Templated type of callback to be issued for the response
/// @tparam ResponseType Type to be passed in the callback
/// @tparam ResponseDependType Dependency type to be injected into the Response
/// @tparam DtoType The Dto type to be used for the response
template <typename CallbackType, typename ResponseType, typename ResponseDependType, typename DtoType>
class ApiResponseHandler : public ApiResponseHandlerBase
{
public:
    ApiResponseHandler(const CallbackType& InCallback, ResponseDependType* InDepend, csp::web::EResponseCodes InValidResponse,
        async::event_task<ResponseType> InOnResponseEventTask = async::event_task<ResponseType> {})
        : ResponseObject(InDepend)
        , ResponseObjectPtr(&ResponseObject)
        , ValidResponse(InValidResponse)
        , Callback(InCallback)
        , OnResponseEventTask(std::move(InOnResponseEventTask))
    {
    }

    void OnHttpProgress(csp::web::HttpRequest& Request) override
    {
        ApiResp.SetResponse(&Request.GetMutableResponse());

        ResponseObjectPtr->OnProgress(&ApiResp);

        // Issue progress callback
        Callback(ResponseObject);
    }

    void OnHttpResponse(csp::web::HttpResponse& Response) override
    {
        ApiResp.SetResponse(&Response);

        // Set the appropriate response code from the Http response
        ApiResp.SetResponseCode(Response.GetResponseCode(), ValidResponse);

        // Let the response object extract what it needs
        ResponseObjectPtr->OnResponse(&ApiResp);

        // Issue final response callback
        Callback(ResponseObject);

        // Call any task continuations
        OnResponseEventTask.set(ResponseObject);
    }

private:
    ApiResponse<DtoType> ApiResp;
    ResponseType ResponseObject;
    csp::systems::ResultBase* ResponseObjectPtr;
    csp::web::EResponseCodes ValidResponse;
    CallbackType Callback;
    async::event_task<ResponseType> OnResponseEventTask;
};

/// @brief Response Handler Pointer Type
using ResponseHandlerPtr = ApiResponseHandlerBase*;

/// @brief Base class for CHS api definition
///
/// Base class for auto-generated Api definitions
class ApiBase
{
public:
    ApiBase(csp::web::WebClient* InWebClient, const csp::common::String* InRootUri)
        : WebClient(InWebClient)
        , RootUri(InRootUri)
    {
    }

    virtual ~ApiBase() { }

    template <typename CallbackType, typename ResponseType, typename ResponseDependType, typename DtoType>
    ResponseHandlerPtr CreateHandler(const CallbackType& InCallback, ResponseDependType* InDepend,
        csp::web::EResponseCodes InValidResponseCode = csp::web::EResponseCodes::ResponseOK,
        async::event_task<ResponseType> InOnResponseEventTask = async::event_task<ResponseType> {})
    {
        // This gets owned by the HttpRequest and gets deleted in it's destructor once the request is complete
        ResponseHandlerPtr Handler = CSP_NEW ApiResponseHandler<CallbackType, ResponseType, ResponseDependType, DtoType>(
            InCallback, InDepend, InValidResponseCode, std::move(InOnResponseEventTask));
        return Handler;
    }

    csp::web::WebClient* WebClient;
    const csp::common::String* RootUri;
};

//
// Template definitions
//

template <class DtoType>
ApiResponse<DtoType>::ApiResponse()
    : ApiResponseBase(CSP_NEW DtoType())
{
}

template <class DtoType> ApiResponse<DtoType>::~ApiResponse() { CSP_DELETE(Dto); }

} // namespace csp::services
