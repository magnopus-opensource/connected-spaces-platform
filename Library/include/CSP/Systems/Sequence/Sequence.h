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
#pragma once

#include "CSP/Systems/SystemsResult.h"

namespace csp::systems
{
/// @ingroup Sequence System
/// @brief A basic class abstraction for a sequence, including key, and reference variables, and items.
class CSP_API Sequence
{
public:
    csp::common::String Key;
    csp::common::String ReferenceType;
    csp::common::String ReferenceId;
    csp::common::Array<csp::common::String> Items;
    csp::common::Map<csp::common::String, csp::common::String> MetaData;
};

/// @ingroup Sequence System
/// @brief Result structure for a sequence result
class CSP_API SequenceResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    const Sequence& GetSequence() const;
    CSP_NO_EXPORT SequenceResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};
    CSP_NO_EXPORT SequenceResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode, csp::systems::ERequestFailureReason Reason)
        : csp::systems::ResultBase(ResCode, HttpResCode, Reason) {};

private:
    SequenceResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    Sequence Sequence;
};

/// @ingroup Sequence System
/// @brief Data class used to contain information when attempting to get an array of sequences.
class CSP_API SequencesResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    const csp::common::Array<Sequence>& GetSequences() const;

    CSP_NO_EXPORT SequencesResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};
    CSP_NO_EXPORT SequencesResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode, csp::systems::ERequestFailureReason Reason)
        : csp::systems::ResultBase(ResCode, HttpResCode, Reason) {};

private:
    SequencesResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<Sequence> Sequences;
};

/// @brief Callback containing a sequence.
/// @param Result SequenceResult : result class
typedef std::function<void(const SequenceResult& Result)> SequenceResultCallback;

/// @brief Callback containing array of sequences.
/// @param Result SequenceResult : result class
typedef std::function<void(const SequencesResult& Result)> SequencesResultCallback;

} // namespace csp::systems
