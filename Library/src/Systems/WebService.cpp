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
#include "CSP/Systems/WebService.h"

#include "Services/ApiBase/ApiBase.h"

namespace csp::systems
{

ResultBase::ResultBase() : FailureReason(0)
{
}

ResultBase::ResultBase(csp::systems::EResultCode ResCode, uint16_t HttpResCode) : Result(ResCode), HttpResponseCode(HttpResCode), FailureReason(0)
{
}

void ResultBase::OnProgress(const services::ApiResponseBase* ApiResponse)
{
	if (ApiResponse)
	{
		const csp::web::HttpRequest* Request = ApiResponse->GetResponse()->GetRequest();

		Result = EResultCode::InProgress;

		RequestProgress	 = Request->GetRequestProgressPercentage();
		ResponseProgress = Request->GetResponseProgressPercentage();
	}
}

/// @brief Standard response handler
/// @param ApiResponse
void ResultBase::OnResponse(const services::ApiResponseBase* ApiResponse)
{
	if (ApiResponse->GetResponseCode() == services::EResponseCode::ResponseSuccess)
	{
		Result = EResultCode::Success;
	}
	else
	{
		Result = EResultCode::Failed;
	}

	HttpResponseCode = (uint16_t) ApiResponse->GetResponse()->GetResponseCode();

	const auto* HttpResponse	= ApiResponse->GetResponse();
	const auto& ResponsePayload = HttpResponse->GetPayload();
	ResponseBody				= ResponsePayload.GetContent();

	const auto& Headers = ResponsePayload.GetHeaders();

	if (Result == EResultCode::Failed && Headers.count("x-errorcode") > 0 && !Headers.at("x-errorcode").empty())
	{
		FailureReason = ParseErrorCode(Headers.at("x-errorcode").c_str());
	}
}

const EResultCode ResultBase::GetResultCode() const
{
	return Result;
}

const uint16_t ResultBase::GetHttpResultCode() const
{
	return HttpResponseCode;
}

const csp::common::String& ResultBase::GetResponseBody() const
{
	return ResponseBody;
}

float ResultBase::GetRequestProgress() const
{
	return RequestProgress;
}

float ResultBase::GetResponseProgress() const
{
	return ResponseProgress;
}

int ResultBase::GetFailureReason() const
{
	return FailureReason;
}

void ResultBase::SetResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
{
	Result			 = ResCode;
	HttpResponseCode = HttpResCode;
}

int ResultBase::ParseErrorCode(const csp::common::String& Value)
{
	return (int) EResultBaseFailureReason::Unknown;
}

} // namespace csp::systems
