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

namespace csp::systems
{
void SequenceDtoToSequence(const chs::SequenceDto& dto, systems::Sequence& sequence)
{
    sequence.Key = csp::common::Decode::URI(dto.GetKey());
    sequence.ReferenceType = dto.GetReferenceType();
    sequence.ReferenceId = dto.GetReferenceId();
    sequence.Items = Convert(dto.GetItems());
    sequence.MetaData = Convert(dto.GetMetadata());
}

bool Sequence::operator==(const Sequence& other) const
{
    return Key == other.Key && ReferenceType == other.ReferenceType && ReferenceId == other.ReferenceId && Items == other.Items
        && MetaData == other.MetaData;
}

bool Sequence::operator!=(const Sequence& other) const { return !(*this == other); }

const Sequence& csp::systems::SequenceResult::GetSequence() const { return m_sequence; }

void SequenceResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* sequenceResponse = static_cast<chs::SequenceDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        sequenceResponse->FromJson(response->GetPayload().GetContent());
        SequenceDtoToSequence(*sequenceResponse, m_sequence);
    }
}

const csp::common::Array<Sequence>& SequencesResult::GetSequences() const { return m_sequences; }

void SequencesResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* sequencesResponse = static_cast<csp::services::DtoArray<chs::SequenceDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        sequencesResponse->FromJson(response->GetPayload().GetContent());

        const std::vector<chs::SequenceDto>& sequenceArray = sequencesResponse->GetArray();
        m_sequences = Array<systems::Sequence>(sequenceArray.size());

        for (size_t i = 0; i < sequenceArray.size(); ++i)
        {
            SequenceDtoToSequence(sequenceArray[i], m_sequences[i]);
        }
    }
}

} // namespace csp::systems
