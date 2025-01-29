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
#include "ApiBase.h"

namespace csp::services
{

ApiResponseBase::ApiResponseBase(DtoBase* InDto)
    : Dto(InDto)
{
}

EResponseCode ApiResponseBase::GetResponseCode() const { return ResponseCode; }

DtoBase* ApiResponseBase::GetDto() const { return Dto; }

void ApiResponseBase::SetResponse(csp::web::HttpResponse* InResponse) { Response = InResponse; }

csp::web::HttpResponse* ApiResponseBase::GetMutableResponse() { return Response; }

const csp::web::HttpResponse* ApiResponseBase::GetResponse() const { return Response; }

void ApiResponseBase::SetResponseCode(csp::web::EResponseCodes InResponseCode, csp::web::EResponseCodes InValidResponseCode)
{
    bool IsValidCode = IsValidResponseCode(static_cast<int>(InResponseCode), static_cast<int>(InValidResponseCode));

    if (IsValidCode)
    {
        ResponseCode = EResponseCode::ResponseSuccess;
    }
    else
    {
        ResponseCode = EResponseCode::ResponseFailed;
    }
    HttpResponseCode = InResponseCode;
}

bool ApiResponseBase::IsValidResponseCode(int ResponseCodeA, int ResponseCodeB)
{
    auto ReturnFirstDigit = [](int x)
    {
        while (x >= 10)
            x /= 10;
        return x;
    };

    return ReturnFirstDigit(ResponseCodeA) == ReturnFirstDigit(ResponseCodeB);
}

ApiResponseHandlerBase::ApiResponseHandlerBase() { }

ApiResponseHandlerBase::~ApiResponseHandlerBase() { }

} // namespace csp::services
