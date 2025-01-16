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

#include "CSP/Multiplayer/EventParameters.h"
#include "CallHelpers.h"
#include "Common/Convert.h"
#include "Common/Encode.h"
#include "Multiplayer/EventSerialisation.h"
#include "Services/AggregationService/Api.h"
#include "Systems/ResultHelpers.h"

using namespace csp;
using namespace csp::common;

namespace chs = csp::services::generated::aggregationservice;

namespace
{
std::shared_ptr<chs::SequenceDto> CreateSequenceDto(const String& SequenceKey, const String& ReferenceType, const String& ReferenceId,
    const Array<String>& Items, const csp::common::Map<csp::common::String, csp::common::String>& MetaData)
{
    auto SequenceInfo = std::make_shared<chs::SequenceDto>();
    SequenceInfo->SetKey(csp::common::Encode::URI(SequenceKey)); // We encode the sequence key in order to allow keys to include reserved characters.
    SequenceInfo->SetReferenceType(ReferenceType);
    SequenceInfo->SetReferenceId(ReferenceId);
    SequenceInfo->SetItems(Convert(Items));
    SequenceInfo->SetMetadata(Convert(MetaData));

    return SequenceInfo;
}

bool ValidateKey(const String& Key)
{
    std::string str = Key.c_str();
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
void SequenceSystem::CreateSequence(const String& SequenceKey, const String& ReferenceType, const String& ReferenceId, const Array<String>& Items,
    const csp::common::Map<csp::common::String, csp::common::String>& MetaData, SequenceResultCallback Callback)
{
    if (!ValidateKey(SequenceKey))
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::Error,
            "Cannot create Sequence. Key: %s contains invalid characters. Invalid characters are \"/\", \"#\", \"%%\"", SequenceKey.c_str());
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SequenceResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
        return;
    }

    const auto SequenceInfo = CreateSequenceDto(SequenceKey, ReferenceType, ReferenceId, Items, MetaData);

    csp::services::ResponseHandlerPtr ResponseHandler
        = SequenceAPI->CreateHandler<SequenceResultCallback, SequenceResult, void, chs::SequenceDto>(Callback, nullptr);

    static_cast<chs::SequenceApi*>(SequenceAPI)
        ->apiV1SequencesPut(std::nullopt, // NewKey
            SequenceInfo, // Dto
            ResponseHandler, // ResponseHandler
            CancellationToken::Dummy() // CancellationToken
        );
}

void SequenceSystem::UpdateSequence(const String& SequenceKey, const String& ReferenceType, const String& ReferenceId, const Array<String>& Items,
    const csp::common::Map<csp::common::String, csp::common::String>& MetaData, SequenceResultCallback Callback)
{
    if (!ValidateKey(SequenceKey))
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::Error,
            "Cannot create Sequence. Key: %s contains invalid characters. Invalid characters are \"/\", \"#\", \"%%\"", SequenceKey.c_str());
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SequenceResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
        return;
    }

    const auto SequenceInfo = CreateSequenceDto(SequenceKey, ReferenceType, ReferenceId, Items, MetaData);

    csp::services::ResponseHandlerPtr ResponseHandler
        = SequenceAPI->CreateHandler<SequenceResultCallback, SequenceResult, void, chs::SequenceDto>(Callback, nullptr);

    static_cast<chs::SequenceApi*>(SequenceAPI)
        ->apiV1SequencesPut(SequenceKey, // NewKey
            SequenceInfo, // Dto
            ResponseHandler, // ResponseHandler
            CancellationToken::Dummy() // CancellationToken
        );
}

void SequenceSystem::RenameSequence(const String& OldSequenceKey, const String& NewSequenceKey, SequenceResultCallback Callback)
{
    if (!ValidateKey(OldSequenceKey))
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::Error,
            "Cannot rename Sequence. Old Key: %s contains invalid characters. Invalid characters are \"/\", \"#\", \"%%\"", OldSequenceKey.c_str());
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SequenceResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
        return;
    }
    if (!ValidateKey(NewSequenceKey))
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::Error,
            "Cannot rename Sequence. New Key: %s contains invalid characters. Invalid characters are \"/\", \"#\", \"%%\"", NewSequenceKey.c_str());
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SequenceResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
        return;
    }

    auto CB = [Callback, NewSequenceKey, this](const SequenceResult& Result)
    {
        if (Result.GetResultCode() != csp::systems::EResultCode::Success)
        {
            Callback(Result);
            return;
        }

        csp::services::ResponseHandlerPtr ResponseHandler
            = SequenceAPI->CreateHandler<SequenceResultCallback, SequenceResult, void, chs::SequenceDto>(Callback, nullptr);

        const auto Sequence = Result.GetSequence();
        const auto SequenceInfo = CreateSequenceDto(Sequence.Key, Sequence.ReferenceType, Sequence.ReferenceId, Sequence.Items, Sequence.MetaData);

        static_cast<chs::SequenceApi*>(SequenceAPI)
            ->apiV1SequencesPut(csp::common::Encode::URI(NewSequenceKey), // NewKey
                SequenceInfo, // Dto
                ResponseHandler, // ResponseHandler
                CancellationToken::Dummy() // CancellationToken
            );
    };

    GetSequence(OldSequenceKey, CB);
}

void SequenceSystem::GetSequencesByCriteria(const Array<String>& InSequenceKeys, const Optional<String>& InKeyRegex,
    const Optional<String>& InReferenceType, const Array<String>& InReferenceIds,
    const csp::common::Map<csp::common::String, csp::common::String>& MetaData, SequencesResultCallback Callback)
{
    Array<String> EncodedSequenceKeys(InSequenceKeys.Size());
    for (size_t i = 0; i < InSequenceKeys.Size(); i++)
    {
        if (!ValidateKey(InSequenceKeys[i]))
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Error,
                "Cannot get Sequence. Key: %s contains invalid characters. Invalid characters are \"/\", \"#\", \"%%\"", InSequenceKeys[i].c_str());
            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SequencesResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
            return;
        }

        // We URI-encode sequence keys to support reserved characters.
        EncodedSequenceKeys[i] = csp::common::Encode::URI(InSequenceKeys[i]);
    }

    // Regexes may also include reserved characters which require encoding.
    Optional<String> Regex;
    if (InKeyRegex.HasValue())
    {
        Regex = csp::common::Encode::URI(*InKeyRegex);
    }

    if ((InReferenceType.HasValue() && InReferenceIds.IsEmpty()) || (!InReferenceIds.IsEmpty() && !InReferenceType.HasValue()))
    {
        CSP_LOG_ERROR_MSG("InReferenceType and InReferenceIds need to be used together");
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SequencesResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
        return;
    }

    std::optional<std::vector<String>> SequenceKeys = Convert(EncodedSequenceKeys);
    std::optional<String> KeyRegex = Convert(Regex);
    std::optional<String> ReferenceType = Convert(InReferenceType);
    std::optional<std::vector<String>> ReferenceIds = Convert(InReferenceIds);

    csp::services::ResponseHandlerPtr ResponseHandler
        = SequenceAPI->CreateHandler<SequencesResultCallback, SequencesResult, void, csp::services::DtoArray<chs::SequenceDto>>(Callback, nullptr);

    static_cast<chs::SequenceApi*>(SequenceAPI)
        ->apiV1SequencesGet(SequenceKeys, // Keys
            KeyRegex, // Regex
            ReferenceType, // ReferenceType
            ReferenceIds, // ReferenceIds
            std::nullopt, // Items
            std::nullopt, // MetaData
            std::nullopt, // Skip
            std::nullopt, // Limit
            ResponseHandler, // ResponseHandler
            CancellationToken::Dummy() // CancellationToken
        );
}

void SequenceSystem::GetAllSequencesContainingItems(
    const Array<String>& InItems, const Optional<String>& InReferenceType, const Array<String>& InReferenceIds, SequencesResultCallback Callback)
{
    std::optional<std::vector<String>> Items = Convert(InItems);
    std::optional<String> ReferenceType = Convert(InReferenceType);
    std::optional<std::vector<String>> ReferenceIds = Convert(InReferenceIds);

    csp::services::ResponseHandlerPtr ResponseHandler
        = SequenceAPI->CreateHandler<SequencesResultCallback, SequencesResult, void, csp::services::DtoArray<chs::SequenceDto>>(Callback, nullptr);

    static_cast<chs::SequenceApi*>(SequenceAPI)
        ->apiV1SequencesGet(std::nullopt, // Keys
            std::nullopt, // Regex
            ReferenceType, // ReferenceType
            ReferenceIds, // ReferenceIds
            Items, // Items
            std::nullopt, // MetaData
            std::nullopt, // Skip
            std::nullopt, // Limit
            ResponseHandler, // ResponseHandler
            CancellationToken::Dummy() // CancellationToken
        );
}

void SequenceSystem::GetSequence(const String& SequenceKey, SequenceResultCallback Callback)
{
    if (!ValidateKey(SequenceKey))
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::Error,
            "Cannot get Sequence. Key: %s contains invalid characters. Invalid characters are \"/\", \"#\", \"%%\"", SequenceKey.c_str());
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SequenceResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
        return;
    }
    csp::services::ResponseHandlerPtr ResponseHandler
        = SequenceAPI->CreateHandler<SequenceResultCallback, SequenceResult, void, chs::SequenceDto>(Callback, nullptr);

    static_cast<chs::SequenceApi*>(SequenceAPI)
        ->apiV1SequencesKeysKeyGet(csp::common::Encode::URI(SequenceKey, true), // Key
            ResponseHandler, // ResponseHandler
            CancellationToken::Dummy() // CancellationToken
        );
}

void SequenceSystem::DeleteSequences(const Array<String>& InSequenceKeys, NullResultCallback Callback)
{
    Array<String> EncodedSequenceKeys(InSequenceKeys.Size());
    for (size_t i = 0; i < InSequenceKeys.Size(); i++)
    {
        if (!ValidateKey(InSequenceKeys[i]))
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Error,
                "Cannot delete Sequence. Key: %s contains invalid characters. Invalid characters are \"/\", \"#\", \"%%\"",
                InSequenceKeys[i].c_str());
            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>(csp::systems::ERequestFailureReason::InvalidSequenceKey));
            return;
        }

        // We URI-encode sequence keys to support reserved characters.
        EncodedSequenceKeys[i] = csp::common::Encode::URI(InSequenceKeys[i]);
    }
    csp::services::ResponseHandlerPtr ResponseHandler = SequenceAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    std::vector<String> SequenceKeys = Convert(EncodedSequenceKeys);
    static_cast<chs::SequenceApi*>(SequenceAPI)
        ->apiV1SequencesKeysDelete(SequenceKeys, // Keys
            ResponseHandler, // ResponseHandler
            CancellationToken::Dummy() // CancellationToken)
        );
}

SequenceSystem::SequenceSystem()
    : SystemBase(nullptr, nullptr)
    , SequenceAPI(nullptr)
{
}

SequenceSystem::SequenceSystem(web::WebClient* InWebClient, multiplayer::EventBus* InEventBus)
    : SystemBase(InWebClient, InEventBus)
{
    SequenceAPI = CSP_NEW chs::SequenceApi(InWebClient);

    RegisterSystemCallback();
}

SequenceSystem::~SequenceSystem()
{
    CSP_DELETE(SequenceAPI);

    DeregisterSystemCallback();
}

void SequenceSystem::SetSequenceChangedCallback(SequenceChangedCallbackHandler Callback)
{
    SequenceChangedCallback = Callback;
    RegisterSystemCallback();
}

void SequenceSystem::RegisterSystemCallback()
{
    if (!SequenceChangedCallback)
    {
        return;
    }

    EventBusPtr->ListenNetworkEvent("SequenceChanged", this);
}

void SequenceSystem::DeregisterSystemCallback()
{
    if (EventBusPtr)
    {
        EventBusPtr->StopListenNetworkEvent("SequenceChanged");
    }
}

void SequenceSystem::OnEvent(const std::vector<signalr::value>& EventValues)
{
    csp::multiplayer::SequenceChangedEventDeserialiser SequenceDeserialiser;
    SequenceDeserialiser.Parse(EventValues);

    if (SequenceChangedCallback)
    {
        SequenceChangedCallback(SequenceDeserialiser.GetEventParams());
    }
}

} // namespace csp::systems
