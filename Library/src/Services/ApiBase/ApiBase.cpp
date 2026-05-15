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

ApiResponseBase::ApiResponseBase(DtoBase* inDto)
    : m_dto(inDto)
{
}

EResponseCode ApiResponseBase::GetResponseCode() const { return m_responseCode; }

DtoBase* ApiResponseBase::GetDto() const { return m_dto; }

void ApiResponseBase::SetResponse(csp::web::HttpResponse* inResponse) { m_response = inResponse; }

csp::web::HttpResponse* ApiResponseBase::GetMutableResponse() { return m_response; }

const csp::web::HttpResponse* ApiResponseBase::GetResponse() const { return m_response; }

void ApiResponseBase::SetResponseCode(csp::web::EResponseCodes inResponseCode, csp::web::EResponseCodes inValidResponseCode)
{
    bool isValidCode = IsValidResponseCode(static_cast<int>(inResponseCode), static_cast<int>(inValidResponseCode));

    if (isValidCode)
    {
        m_responseCode = EResponseCode::ResponseSuccess;
    }
    else
    {
        m_responseCode = EResponseCode::ResponseFailed;
    }
    m_httpResponseCode = inResponseCode;
}

bool ApiResponseBase::IsValidResponseCode(int responseCodeA, int responseCodeB)
{
    auto returnFirstDigit = [](int x)
    {
        while (x >= 10)
            x /= 10;
        return x;
    };

    return returnFirstDigit(responseCodeA) == returnFirstDigit(responseCodeB);
}

ApiResponseHandlerBase::ApiResponseHandlerBase() { }

ApiResponseHandlerBase::~ApiResponseHandlerBase() { }

} // namespace csp::services
