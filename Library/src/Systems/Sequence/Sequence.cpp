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

#include "CSP/Systems/Sequence/Sequence.h"

#include "CSP/Systems/Sequence/SequenceSystem.h"
#include "Common/Convert.h"
#include "Common/Encode.h"
#include "Services/AggregationService/Api.h"

using namespace csp;
using namespace csp::common;

namespace chs = csp::services::generated::aggregationservice;

namespace
{
void SequenceDtoToSequence(const chs::SequenceDto& Dto, systems::Sequence& Sequence)
{
    Sequence.Key = csp::common::Decode::URI(Dto.GetKey());
    Sequence.ReferenceType = Dto.GetReferenceType();
    Sequence.ReferenceId = Dto.GetReferenceId();
    Sequence.Items = Convert(Dto.GetItems());
    Sequence.MetaData = Convert(Dto.GetMetadata());
}

} // namespace

namespace csp::systems
{
const Sequence& csp::systems::SequenceResult::GetSequence() const { return Sequence; }

void SequenceResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* SequenceResponse = static_cast<chs::SequenceDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        SequenceResponse->FromJson(Response->GetPayload().GetContent());
        SequenceDtoToSequence(*SequenceResponse, Sequence);
    }
}

const csp::common::Array<Sequence>& SequencesResult::GetSequences() const { return Sequences; }

void SequencesResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* SequencesResponse = static_cast<csp::services::DtoArray<chs::SequenceDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        SequencesResponse->FromJson(Response->GetPayload().GetContent());

        const std::vector<chs::SequenceDto>& SequenceArray = SequencesResponse->GetArray();
        Sequences = Array<systems::Sequence>(SequenceArray.size());

        for (size_t i = 0; i < SequenceArray.size(); ++i)
        {
            SequenceDtoToSequence(SequenceArray[i], Sequences[i]);
        }
    }
}

} // namespace csp::systems
