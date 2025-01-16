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

bool BooleanResult::GetValue() const { return Value; }

void BooleanResult::SetValue(bool InValue) { Value = InValue; }

const csp::common::String& StringResult::GetValue() const { return Value; }

void StringResult::SetValue(const csp::common::String& InValue) { Value = InValue; }

const csp::common::Array<csp::common::String>& StringArrayResult::GetValue() const { return Value; }

void StringArrayResult::SetValue(const csp::common::Array<csp::common::String>& InValue) { Value = csp::common::Array<csp::common::String>(InValue); }

uint64_t UInt64Result::GetValue() const { return Value; }

void UInt64Result::SetValue(uint64_t InValue) { Value = InValue; }

void HTTPHeadersResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseFailed)
    {
        return;
    }

    auto Response = ApiResponse->GetResponse();
    auto Headers = Response->GetPayload().GetHeaders();

    for (auto& Header : Headers)
    {
        csp::common::String Key(Header.first.c_str());
        csp::common::String Val(Header.second.c_str());

        Value[Key] = Val;
    }
}

const csp::common::Map<csp::common::String, csp::common::String>& HTTPHeadersResult::GetValue() const { return Value; }

} // namespace csp::systems
