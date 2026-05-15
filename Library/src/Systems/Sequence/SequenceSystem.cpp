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

#include "CSP/Systems/Sequence/SequenceSystem.h"

#include "CSP/Common/NetworkEventData.h"
#include "CallHelpers.h"
#include "Common/Convert.h"
#include "Common/Encode.h"
#include "Multiplayer/NetworkEventSerialisation.h"
#include "Services/AggregationService/Api.h"
#include "Systems/ResultHelpers.h"

using namespace csp;
using namespace csp::common;

namespace chs = csp::services::generated::aggregationservice;

namespace
{
std::shared_ptr<chs::SequenceDto> CreateSequenceDto(const String& sequenceKey, const String& referenceType, const String& referenceId,
    const Array<String>& items, const csp::common::Map<csp::common::String, csp::common::String>& metaData)
{
    auto sequenceInfo = std::make_shared<chs::SequenceDto>();
    sequenceInfo->SetKey(csp::common::Encode::URI(sequenceKey)); // We encode the sequence key in order to allow keys to include reserved characters.
    sequenceInfo->SetReferenceType(referenceType);
    sequenceInfo->SetReferenceId(referenceId);
    sequenceInfo->SetItems(Convert(items));
    sequenceInfo->SetMetadata(Convert(metaData));

    return sequenceInfo;
}

bool ValidateKey(const String& key)
{
    std::string str = key.c_str();
    if (str.find("/") != std::string::npos)
    {
        return false;
    }
    if (str.find("#") != std::string::npos)
    {
        return false;
    }
    if (str.find("%") != std::string::npos)
    {
        return false;
    }
    return true;
}

} // namespace

namespace csp::systems
{
void SequenceSystem::CreateSequence(const String& sequenceKey, const String& referenceType, const String& referenceId, const Array<String>& items,
    const csp::common::Map<csp::common::String, csp::common::String>& metaData, SequenceResultCallback callback)
{
    if (!ValidateKey(sequenceKey))
    {
        CSP_LOG_FORMAT(csp::common::LogLevel::Error,
            "Cannot create Sequence. Key: %s contains invalid characters. Invalid characters are \"/\", \"#\", \"%%\"", sequenceKey.c_str());
        INVOKE_IF_NOT_NULL(callback, MakeInvalid<SequenceResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
        return;
    }

    const auto sequenceInfo = CreateSequenceDto(sequenceKey, referenceType, referenceId, items, metaData);

    csp::services::ResponseHandlerPtr responseHandler
        = m_sequenceApi->CreateHandler<SequenceResultCallback, SequenceResult, void, chs::SequenceDto>(callback, nullptr);

    static_cast<chs::SequenceApi*>(m_sequenceApi)
        ->sequencesPut(
            {
                std::nullopt, // NewKey
                sequenceInfo // Dto
            },
            responseHandler, // ResponseHandler
            CancellationToken::Dummy() // CancellationToken
        );
}

void SequenceSystem::UpdateSequence(const String& sequenceKey, const String& referenceType, const String& referenceId, const Array<String>& items,
    const csp::common::Map<csp::common::String, csp::common::String>& metaData, SequenceResultCallback callback)
{
    if (!ValidateKey(sequenceKey))
    {
        CSP_LOG_FORMAT(csp::common::LogLevel::Error,
            "Cannot create Sequence. Key: %s contains invalid characters. Invalid characters are \"/\", \"#\", \"%%\"", sequenceKey.c_str());
        INVOKE_IF_NOT_NULL(callback, MakeInvalid<SequenceResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
        return;
    }

    const auto sequenceInfo = CreateSequenceDto(sequenceKey, referenceType, referenceId, items, metaData);

    csp::services::ResponseHandlerPtr responseHandler
        = m_sequenceApi->CreateHandler<SequenceResultCallback, SequenceResult, void, chs::SequenceDto>(callback, nullptr);

    static_cast<chs::SequenceApi*>(m_sequenceApi)
        ->sequencesPut(
            {
                csp::common::Encode::URI(sequenceKey), // NewKey
                sequenceInfo // Dto
            },
            responseHandler, // ResponseHandler
            CancellationToken::Dummy() // CancellationToken
        );
}

void SequenceSystem::RenameSequence(const String& oldSequenceKey, const String& newSequenceKey, SequenceResultCallback callback)
{
    if (!ValidateKey(oldSequenceKey))
    {
        CSP_LOG_FORMAT(csp::common::LogLevel::Error,
            "Cannot rename Sequence. Old Key: %s contains invalid characters. Invalid characters are \"/\", \"#\", \"%%\"", oldSequenceKey.c_str());
        INVOKE_IF_NOT_NULL(callback, MakeInvalid<SequenceResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
        return;
    }
    if (!ValidateKey(newSequenceKey))
    {
        CSP_LOG_FORMAT(csp::common::LogLevel::Error,
            "Cannot rename Sequence. New Key: %s contains invalid characters. Invalid characters are \"/\", \"#\", \"%%\"", newSequenceKey.c_str());
        INVOKE_IF_NOT_NULL(callback, MakeInvalid<SequenceResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
        return;
    }

    auto cb = [callback, newSequenceKey, this](const SequenceResult& result)
    {
        if (result.GetResultCode() != csp::systems::EResultCode::Success)
        {
            callback(result);
            return;
        }

        csp::services::ResponseHandlerPtr responseHandler
            = m_sequenceApi->CreateHandler<SequenceResultCallback, SequenceResult, void, chs::SequenceDto>(callback, nullptr);

        const auto sequence = result.GetSequence();

        const auto sequenceInfo = CreateSequenceDto(sequence.Key, sequence.ReferenceType, sequence.ReferenceId, sequence.Items, sequence.MetaData);

        static_cast<chs::SequenceApi*>(m_sequenceApi)
            ->sequencesPut(
                {
                    csp::common::Encode::URI(newSequenceKey), // NewKey
                    sequenceInfo // Dto
                },
                responseHandler, // ResponseHandler
                CancellationToken::Dummy() // CancellationToken
            );
    };

    GetSequence(oldSequenceKey, cb);
}

void SequenceSystem::GetSequencesByCriteria(const Array<String>& inSequenceKeys, const Optional<String>& inKeyRegex,
    const Optional<String>& inReferenceType, const Array<String>& inReferenceIds,
    const csp::common::Map<csp::common::String, csp::common::String>& /*MetaData*/, SequencesResultCallback callback)
{
    Array<String> encodedSequenceKeys(inSequenceKeys.Size());
    for (size_t i = 0; i < inSequenceKeys.Size(); i++)
    {
        if (!ValidateKey(inSequenceKeys[i]))
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Error,
                "Cannot get Sequence. Key: %s contains invalid characters. Invalid characters are \"/\", \"#\", \"%%\"", inSequenceKeys[i].c_str());
            INVOKE_IF_NOT_NULL(callback, MakeInvalid<SequencesResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
            return;
        }

        // We URI-encode sequence keys to support reserved characters.
        encodedSequenceKeys[i] = csp::common::Encode::URI(inSequenceKeys[i]);
    }

    // Regexes may also include reserved characters which require encoding.
    Optional<String> regex;
    if (inKeyRegex.HasValue())
    {
        regex = csp::common::Encode::URI(*inKeyRegex);
    }

    if ((inReferenceType.HasValue() && inReferenceIds.IsEmpty()) || (!inReferenceIds.IsEmpty() && !inReferenceType.HasValue()))
    {
        CSP_LOG_ERROR_MSG("InReferenceType and InReferenceIds need to be used together");
        INVOKE_IF_NOT_NULL(callback, MakeInvalid<SequencesResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
        return;
    }

    std::optional<std::vector<String>> sequenceKeys = Convert(encodedSequenceKeys);
    std::optional<String> keyRegex = Convert(regex);
    std::optional<String> referenceType = Convert(inReferenceType);
    std::optional<std::vector<String>> referenceIds = Convert(inReferenceIds);

    csp::services::ResponseHandlerPtr responseHandler
        = m_sequenceApi->CreateHandler<SequencesResultCallback, SequencesResult, void, csp::services::DtoArray<chs::SequenceDto>>(callback, nullptr);

    static_cast<chs::SequenceApi*>(m_sequenceApi)
        ->sequencesGet(
            {
                sequenceKeys, // Keys
                keyRegex, // Regex
                referenceType, // ReferenceType
                referenceIds, // ReferenceIds
                std::nullopt, // Items
                std::nullopt, // MetaData
                std::nullopt, // Skip
                std::nullopt // Limit
            },
            responseHandler, // ResponseHandler
            CancellationToken::Dummy() // CancellationToken
        );
}

void SequenceSystem::GetAllSequencesContainingItems(
    const Array<String>& inItems, const Optional<String>& inReferenceType, const Array<String>& inReferenceIds, SequencesResultCallback callback)
{
    std::optional<std::vector<String>> items = Convert(inItems);
    std::optional<String> referenceType = Convert(inReferenceType);
    std::optional<std::vector<String>> referenceIds = Convert(inReferenceIds);

    csp::services::ResponseHandlerPtr responseHandler
        = m_sequenceApi->CreateHandler<SequencesResultCallback, SequencesResult, void, csp::services::DtoArray<chs::SequenceDto>>(callback, nullptr);

    static_cast<chs::SequenceApi*>(m_sequenceApi)
        ->sequencesGet(
            {
                std::nullopt, // Keys
                std::nullopt, // Regex
                referenceType, // ReferenceType
                referenceIds, // ReferenceIds
                items, // Items
                std::nullopt, // MetaData
                std::nullopt, // Skip
                std::nullopt // Limit
            },
            responseHandler, // ResponseHandler
            CancellationToken::Dummy() // CancellationToken
        );
}

void SequenceSystem::GetSequence(const String& sequenceKey, SequenceResultCallback callback)
{
    if (!ValidateKey(sequenceKey))
    {
        CSP_LOG_FORMAT(csp::common::LogLevel::Error,
            "Cannot get Sequence. Key: %s contains invalid characters. Invalid characters are \"/\", \"#\", \"%%\"", sequenceKey.c_str());
        INVOKE_IF_NOT_NULL(callback, MakeInvalid<SequenceResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
        return;
    }
    csp::services::ResponseHandlerPtr responseHandler
        = m_sequenceApi->CreateHandler<SequenceResultCallback, SequenceResult, void, chs::SequenceDto>(callback, nullptr);

    static_cast<chs::SequenceApi*>(m_sequenceApi)
        ->sequencesKeysKeyGet({ csp::common::Encode::URI(sequenceKey, true) }, // Key
            responseHandler, // ResponseHandler
            CancellationToken::Dummy() // CancellationToken
        );
}

void SequenceSystem::DeleteSequences(const Array<String>& inSequenceKeys, NullResultCallback callback)
{
    Array<String> encodedSequenceKeys(inSequenceKeys.Size());
    for (size_t i = 0; i < inSequenceKeys.Size(); i++)
    {
        if (!ValidateKey(inSequenceKeys[i]))
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Error,
                "Cannot delete Sequence. Key: %s contains invalid characters. Invalid characters are \"/\", \"#\", \"%%\"",
                inSequenceKeys[i].c_str());
            INVOKE_IF_NOT_NULL(callback, MakeInvalid<NullResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
            return;
        }

        // We URI-encode sequence keys to support reserved characters.
        encodedSequenceKeys[i] = csp::common::Encode::URI(inSequenceKeys[i]);
    }
    csp::services::ResponseHandlerPtr responseHandler = m_sequenceApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    std::vector<String> sequenceKeys = Convert(encodedSequenceKeys);
    static_cast<chs::SequenceApi*>(m_sequenceApi)
        ->sequencesKeysDelete({ sequenceKeys }, // Keys
            responseHandler, // ResponseHandler
            CancellationToken::Dummy() // CancellationToken)
        );
}

SequenceSystem::SequenceSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , m_sequenceApi(nullptr)
{
}

SequenceSystem::SequenceSystem(web::WebClient* webClient, multiplayer::NetworkEventBus& eventBus, csp::common::LogSystem& logSystem)
    : SystemBase(webClient, &eventBus, &logSystem)
{
    m_sequenceApi = new chs::SequenceApi(webClient);

    RegisterSystemCallback();
}

SequenceSystem::~SequenceSystem() { delete (m_sequenceApi); }

void SequenceSystem::SetSequenceChangedCallback(SequenceChangedCallbackHandler callback)
{
    m_sequenceChangedCallback = callback;
    RegisterSystemCallback();
}

void SequenceSystem::RegisterSystemCallback()
{
    if (!m_eventBusPtr)
    {
        CSP_LOG_ERROR_MSG("Error: Failed to register SequenceSystem. NetworkEventBus must be instantiated in the MultiplayerConnection first.");
        return;
    }

    if (!m_sequenceChangedCallback)
    {
        return;
    }

    m_eventBusPtr->ListenNetworkEvent(
        csp::multiplayer::NetworkEventRegistration("CSPInternal::SequenceSystem",
            csp::multiplayer::NetworkEventBus::StringFromNetworkEvent(csp::multiplayer::NetworkEventBus::NetworkEvent::SequenceChanged)),
        [this](const csp::common::NetworkEventData& networkEventData) { this->OnSequenceChangedEvent(networkEventData); });
}

void SequenceSystem::OnSequenceChangedEvent(const csp::common::NetworkEventData& networkEventData)
{
    // This event may either represent a default sequence or a hotspot sequence. Here we are only concerned with default sequences.
    const auto& sequenceEvent = static_cast<const csp::common::SequenceChangedNetworkEventData&>(networkEventData);

    if (sequenceEvent.SequenceType == csp::common::ESequenceType::Default && m_sequenceChangedCallback)
    {
        // We can cast directly, we're sure we're the correct type.
        m_sequenceChangedCallback(sequenceEvent);
    }
}

} // namespace csp::systems
