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
#include "CSP/Systems/ExternalServices/ExternalServiceInvocation.h"
#include "Debug/Logging.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/AggregationService/Dto.h"
#include "Json/JsonParseHelper.h"

#include <regex>

namespace chs_aggregation = csp::services::generated::aggregationservice;
namespace csp::systems
{

void ExternalServiceInvocationResult::OnResponse(const services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto authResponse = static_cast<chs_aggregation::ServiceResponse*>(apiResponse->GetDto());
    const web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == services::EResponseCode::ResponseSuccess)
    {
        rapidjson::Document jsonPayload;
        jsonPayload.Parse(response->GetPayload().GetContent());

        if (jsonPayload.HasParseError() == false)
        {
            authResponse->FromJson(response->GetPayload().GetContent());
            const std::shared_ptr<rapidjson::Document> operationResult = authResponse->GetOperationResult();

            if (operationResult)
            {
                // The external service proxy services endpoint directly routes data back from the external service it is proxying for,
                // so we do not assume format/content and just return the result directly to the client.
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                operationResult->Accept(writer);

                const csp::common::String jsonString = std::string(buffer.GetString(), buffer.GetSize()).c_str();
                SetValue(jsonString);
            }
            else
            {
                CSP_LOG_FORMAT(csp::common::LogLevel::Error,
                    "The operation result Json field for an external service proxy invocation was invalid/missing. Response: %s",
                    response->GetPayload().GetContent().c_str());
            }
        }
        else
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Error,
                "Encountered a malformed operation result Json string when attempting to process the response from an external service proxy "
                "invocation result. Payload: %s",
                response->GetPayload().GetContent().c_str());
        }
    }
}

void GetAgoraTokenResult::OnResponse(const services::ApiResponseBase* apiResponse)
{
    // Run generic processing of the response.
    ExternalServiceInvocationResult::OnResponse(apiResponse);

    // If the string is not empty, we can assume it contains valid Json as ExternalServiceInvocationResult has already parsed it for the operation
    // result.
    const csp::common::String operationResult(GetValue());
    if (operationResult.IsEmpty() == false)
    {
        // Extract the Agora token from the observed operation result.
        rapidjson::Document operationResultJson;
        rapidjson::ParseResult ok = csp::json::ParseWithErrorLogging(operationResultJson, operationResult, "GetAgoraTokenResult::OnResponse");
        if (!ok)
        {
            return;
        }

        // As this is a specialized function for Agora, we know the format to expect.
        if (operationResultJson.HasMember("token") && operationResultJson["token"].IsString())
        {
            // For GetAgoraTokeResult, the result value is just the token.
            SetValue(operationResultJson["token"].GetString());
        }
        else
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Error,
                "Whilst the operation result for an Agora token request was valid Json, there was no 'token' result returned. Operation result: %s",
                operationResult.c_str());
        }
    }
}

} // namespace csp::systems
