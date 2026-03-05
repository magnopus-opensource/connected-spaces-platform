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
#include "Services/aggregationservice/Dto.h"
#include "Json/JsonParseHelper.h"

#include <regex>

namespace chs_aggregation = csp::services::generated::aggregationservice;
namespace csp::systems
{

void ExternalServiceInvocationResult::OnResponse(const services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto AuthResponse = static_cast<chs_aggregation::ServiceResponse*>(ApiResponse->GetDto());
    const web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == services::EResponseCode::ResponseSuccess)
    {
        rapidjson::Document JsonPayload;
        JsonPayload.Parse(Response->GetPayload().GetContent());

        if (JsonPayload.HasParseError() == false)
        {
            AuthResponse->FromJson(Response->GetPayload().GetContent());
            const std::shared_ptr<rapidjson::Document> OperationResult = AuthResponse->GetOperationResult();

            if (OperationResult)
            {
                // The external service proxy services endpoint directly routes data back from the external service it is proxying for,
                // so we do not assume format/content and just return the result directly to the client.
                rapidjson::StringBuffer Buffer;
                rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
                OperationResult->Accept(Writer);

                const csp::common::String JsonString = std::string(Buffer.GetString(), Buffer.GetSize()).c_str();
                SetValue(JsonString);
            }
            else
            {
                CSP_LOG_FORMAT(csp::common::LogLevel::Error,
                    "The operation result Json field for an external service proxy invocation was invalid/missing. Response: %s",
                    Response->GetPayload().GetContent().c_str());
            }
        }
        else
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Error,
                "Encountered a malformed operation result Json string when attempting to process the response from an external service proxy "
                "invocation result. Payload: %s",
                Response->GetPayload().GetContent().c_str());
        }
    }
}

void GetAgoraTokenResult::OnResponse(const services::ApiResponseBase* ApiResponse)
{
    // Run generic processing of the response.
    ExternalServiceInvocationResult::OnResponse(ApiResponse);

    // If the string is not empty, we can assume it contains valid Json as ExternalServiceInvocationResult has already parsed it for the operation
    // result.
    const csp::common::String OperationResult(GetValue());
    if (OperationResult.IsEmpty() == false)
    {
        // Extract the Agora token from the observed operation result.
        rapidjson::Document OperationResultJson;
        rapidjson::ParseResult ok = csp::json::ParseWithErrorLogging(OperationResultJson, OperationResult, "GetAgoraTokenResult::OnResponse");
        if (!ok)
        {
            return;
        }

        // As this is a specialized function for Agora, we know the format to expect.
        if (OperationResultJson.HasMember("token") && OperationResultJson["token"].IsString())
        {
            // For GetAgoraTokeResult, the result value is just the token.
            SetValue(OperationResultJson["token"].GetString());
        }
        else
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Error,
                "Whilst the operation result for an Agora token request was valid Json, there was no 'token' result returned. Operation result: %s",
                OperationResult.c_str());
        }
    }
}

} // namespace csp::systems
