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

#include "CSP/Common/CSPAsyncScheduler.h"
#include "CSP/Systems/WebService.h"
#include "Common/Web/HttpResponse.h"
#include "Common/Web/Json.h"
#include "Common/Web/WebClient.h"
#include "Debug/Logging.h"
#include "Json/JsonParseHelper.h"
#include "Services/DtoBase/DtoBase.h"

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
    ApiResponseBase(DtoBase* inDto);
    virtual ~ApiResponseBase() = default;

    EResponseCode GetResponseCode() const;
    DtoBase* GetDto() const;

    void SetResponse(csp::web::HttpResponse* response);
    csp::web::HttpResponse* GetMutableResponse();
    const csp::web::HttpResponse* GetResponse() const;

    void SetResponseCode(csp::web::EResponseCodes inResponseCode, csp::web::EResponseCodes inValidResponseCode);

protected:
    EResponseCode m_responseCode = EResponseCode::ResponseFailed;
    csp::web::EResponseCodes m_httpResponseCode = csp::web::EResponseCodes::ResponseInit;
    DtoBase* m_dto;
    csp::web::HttpResponse* m_response = nullptr;

private:
    bool IsValidResponseCode(int responseCodeA, int responseCodeB);
};

/// @brief Type for returning Array of DTO objects from a web api call
/// @tparam DtoType
template <class DtoType> class DtoArray : public DtoBase
{
public:
    DtoArray() { }

    virtual ~DtoArray() { }

    virtual utility::string_t ToJson() const override { return utility::string_t(""); }

    virtual void FromJson(const utility::string_t& json) override
    {
        assert(json.c_str());

        rapidjson::Document jsonDoc;
        rapidjson::ParseResult ok = csp::json::ParseWithErrorLogging(jsonDoc, json, "DtoArray::FromJson");
        if (!ok)
        {
            return;
        }

        if (jsonDoc.IsArray())
        {
            m_array.resize(jsonDoc.Size());

            for (rapidjson::SizeType i = 0; i < jsonDoc.Size(); i++)
            {
                rapidjson::Value& val = jsonDoc[i];

                DtoType& dto = m_array[i];
                dto.FromJson(csp::web::JsonObjectToString(val));
            }
        }
        else if (jsonDoc.HasMember("items"))
        {
            if (!jsonDoc["items"].IsArray())
            {
                return;
            }

            m_array.resize(jsonDoc["items"].Size());

            for (rapidjson::SizeType i = 0; i < jsonDoc["items"].Size(); i++)
            {
                rapidjson::Value& val = jsonDoc["items"][i];

                DtoType& dto = m_array[i];
                dto.FromJson(csp::web::JsonObjectToString(val));
            }
        }
    }

    const std::vector<DtoType>& GetArray() const { return m_array; }

    std::vector<DtoType>& GetArray() { return m_array; }

private:
    std::vector<DtoType> m_array;
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

    virtual void OnHttpProgress(csp::web::HttpRequest& request) override = 0;
    virtual void OnHttpResponse(csp::web::HttpResponse& response) override = 0;

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
    ApiResponseHandler(const CallbackType& inCallback, ResponseDependType* inDepend, csp::web::EResponseCodes inValidResponse,
        async::event_task<ResponseType> inOnResponseEventTask = async::event_task<ResponseType> {})
        : m_responseObject(inDepend)
        , m_responseObjectPtr(&m_responseObject)
        , m_validResponse(inValidResponse)
        , m_callback(inCallback)
        , m_onResponseEventTask(std::move(inOnResponseEventTask))
    {
    }

    void OnHttpProgress(csp::web::HttpRequest& request) override
    {
        m_apiResp.SetResponse(&request.GetMutableResponse());

        m_responseObjectPtr->OnProgress(&m_apiResp);

        // Issue progress callback
        m_callback(m_responseObject);
    }

    void OnHttpResponse(csp::web::HttpResponse& response) override
    {
        m_apiResp.SetResponse(&response);

        // Set the appropriate response code from the Http response
        m_apiResp.SetResponseCode(response.GetResponseCode(), m_validResponse);

        // Let the response object extract what it needs
        m_responseObjectPtr->OnResponse(&m_apiResp);

        // Issue final response callback
        m_callback(m_responseObject);

        // Call any task continuations
        m_onResponseEventTask.set(m_responseObject);
    }

private:
    ApiResponse<DtoType> m_apiResp;
    ResponseType m_responseObject;
    csp::systems::ResultBase* m_responseObjectPtr;
    csp::web::EResponseCodes m_validResponse;
    CallbackType m_callback;
    async::event_task<ResponseType> m_onResponseEventTask;
};

/// @brief Response Handler Pointer Type
using ResponseHandlerPtr = ApiResponseHandlerBase*;

/// @brief Base class for CHS api definition
///
/// Base class for auto-generated Api definitions
class ApiBase
{
public:
    ApiBase(csp::web::WebClient* inWebClient, const csp::ServiceDefinition& inServiceDefinition)
        : WebClient(inWebClient)
        , ServiceDefinition(inServiceDefinition)
    {
    }

    virtual ~ApiBase() { }

    template <typename CallbackType, typename ResponseType, typename ResponseDependType, typename DtoType>
    ResponseHandlerPtr CreateHandler(const CallbackType& inCallback, ResponseDependType* inDepend,
        csp::web::EResponseCodes inValidResponseCode = csp::web::EResponseCodes::ResponseOK,
        async::event_task<ResponseType> inOnResponseEventTask = async::event_task<ResponseType> {})
    {
        // This gets owned by the HttpRequest and gets deleted in it's destructor once the request is complete
        ResponseHandlerPtr handler = new ApiResponseHandler<CallbackType, ResponseType, ResponseDependType, DtoType>(
            inCallback, inDepend, inValidResponseCode, std::move(inOnResponseEventTask));
        return handler;
    }

    csp::web::WebClient* WebClient;
    const ServiceDefinition& ServiceDefinition;
};

//
// Template definitions
//

template <class DtoType>
ApiResponse<DtoType>::ApiResponse()
    : ApiResponseBase(new DtoType())
{
}

template <class DtoType> ApiResponse<DtoType>::~ApiResponse() { delete (m_dto); }

} // namespace csp::services
