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
        AuthResponse->FromJson(Response->GetPayload().GetContent());
        std::shared_ptr<rapidjson::Document> OperationResult = AuthResponse->GetOperationResult();

        if (!OperationResult)
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Error, "The operation result for an external service proxy invocation was invalid. Response: %s",
                Response->GetPayload().GetContent().c_str());

            return;
        }

        if (!OperationResult->HasMember("token"))
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Error,
                "Whilst the operation result for an external service proxy invocation was valid, there was no 'token' result returned. Response: %s",
                Response->GetPayload().GetContent().c_str());

            return;
        }

        SetValue(OperationResult->operator[]("token").GetString());
    }
}

} // namespace csp::systems
