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
#include "CSP/Systems/SystemsResult.h"

#include "Services/ApiBase/ApiBase.h"

namespace csp::systems
{

bool BooleanResult::GetValue() const { return m_value; }

void BooleanResult::SetValue(bool inValue) { m_value = inValue; }

const csp::common::String& StringResult::GetValue() const { return m_value; }

void StringResult::SetValue(const csp::common::String& inValue) { m_value = inValue; }

const csp::common::Array<csp::common::String>& StringArrayResult::GetValue() const { return m_value; }

void StringArrayResult::SetValue(const csp::common::Array<csp::common::String>& inValue) { m_value = csp::common::Array<csp::common::String>(inValue); }

uint64_t UInt64Result::GetValue() const { return m_value; }

void UInt64Result::SetValue(uint64_t inValue) { m_value = inValue; }

void HTTPHeadersResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseFailed)
    {
        return;
    }

    auto response = apiResponse->GetResponse();
    auto headers = response->GetPayload().GetHeaders();

    for (auto& header : headers)
    {
        csp::common::String key(header.first.c_str());
        csp::common::String val(header.second.c_str());

        m_value[key] = val;
    }
}

const csp::common::Map<csp::common::String, csp::common::String>& HTTPHeadersResult::GetValue() const { return m_value; }

} // namespace csp::systems
