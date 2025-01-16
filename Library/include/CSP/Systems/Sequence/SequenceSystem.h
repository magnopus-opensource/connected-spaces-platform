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

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/EventParameters.h"
#include "CSP/Systems/Sequence/Sequence.h"
#include "CSP/Systems/SystemBase.h"

namespace csp::web
{

class WebClient;

} // namespace csp::web

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

namespace csp::systems
{
/// @ingroup Sequence System
/// @brief Public facing system that allows the management of groupings of items in a space.
class CSP_API SequenceSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    friend void csp::memory::Delete<SequenceSystem>(SequenceSystem* Ptr);
    /** @endcond */
    CSP_END_IGNORE
public:
    // Hotspot:[SpaceId]:[GroupName]
    /// @brief Creates a new sequence. If a sequence already exists with this key, it will overwrite the current one.
    /// @note This call will fail (Reason InvalidSequenceKey) if the SequenceKey parameter contains invalid keys, such as spaces, '/' or '%'
    /// This call will fail if the user isn't a creator of the space.
    /// @param SequenceKey csp::common::String : The unique grouping name. Our naming convention is: Type:[SpaceId]:[GroupName]
    /// @param ReferenceType csp::common::String : The type of reference (GroupId etc.)
    /// @param ReferenceId csp::common::String : The id of the reference
    /// @param Items csp::common::Array<csp::common::String> : An ordered array of members
    /// @param MetaData csp::common::Map<csp::common::String, csp::common::String> : Any additional data relating to the Sequence
    /// @param Callback SequenceResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void CreateSequence(const csp::common::String& SequenceKey, const csp::common::String& ReferenceType,
        const csp::common::String& ReferenceId, const csp::common::Array<csp::common::String>& Items,
        const csp::common::Map<csp::common::String, csp::common::String>& MetaData, SequenceResultCallback Callback);

    /// @brief Updates an existing sequence. This call will fail if the user isn't a creator of the space.
    /// @note This call will fail if the SequenceKey parameter contains invalid keys, such as spaces, '/' or '%'
    /// @param SequenceKey csp::common::String : The unique grouping name. Our naming convention is: Type:[SpaceId]:[GroupName]
    /// @param ReferenceType csp::common::String : The type of reference (GroupId etc.)
    /// @param ReferenceId csp::common::String : The id of the reference
    /// @param Items csp::common::Array<csp::common::String> : An ordered array of members
    /// @param MetaData csp::common::Map<csp::common::String, csp::common::String> : Any additional data relating to the Sequence
    /// @param Callback SequenceResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void UpdateSequence(const csp::common::String& SequenceKey, const csp::common::String& ReferenceType,
        const csp::common::String& ReferenceId, const csp::common::Array<csp::common::String>& Items,
        const csp::common::Map<csp::common::String, csp::common::String>& MetaData, SequenceResultCallback Callback);

    /// @brief Renames a given sequence. This call will fail if the user isn't a creator of the space.
    /// @note This call will fail (Reason InvalidSequenceKey) if the OldSequenceKey, or NewSequenceKey parameters contains invalid keys, such as
    /// spaces, '/' or '%'
    /// @param OldSequenceKey csp::common::String : The current sequence key name
    /// @param NewSequenceKey csp::common::String : The new sequence key name
    /// @param Callback SequenceResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void RenameSequence(
        const csp::common::String& OldSequenceKey, const csp::common::String& NewSequenceKey, SequenceResultCallback Callback);

    /// @brief Finds sequences based on the given criteria
    /// @note This call will fail (Reason InvalidSequenceKey) if the SequenceKey parameter contains invalid keys, such as spaces, '/' or '%'
    /// @param SequenceKeys csp::common::Array<csp::common::String> : An array of sequence keys to search for
    /// @param SequenceKeys csp::common::Optional<csp::common::String> : An optional regex string for searching keys
    /// @param ReferenceType csp::common::String : The type of reference (GroupId etc.). Must be used with ReferenceIds
    /// @param ReferenceIds csp::common::Array<csp::common::String> : The ids of the reference. Must be used with ReferenceType
    /// @param MetaData csp::common::Map<csp::common::String, csp::common::String> : Any additional data relating to the Sequence
    /// @param Callback SequencesResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void GetSequencesByCriteria(const csp::common::Array<csp::common::String>& SequenceKeys,
        const csp::common::Optional<csp::common::String>& KeyRegex, const csp::common::Optional<csp::common::String>& ReferenceType,
        const csp::common::Array<csp::common::String>& ReferenceIds, const csp::common::Map<csp::common::String, csp::common::String>& MetaData,
        SequencesResultCallback Callback);

    /// @brief Finds all sequences that contain the given items
    /// @param Items csp::common::Array<csp::common::String> : An array of items which should be searched for
    /// @param ReferenceType csp::common::String : The type of reference (GroupId etc.). Must be used with ReferenceIds
    /// @param ReferenceIds csp::common::Array<csp::common::String> : The ids of the reference. Must be used with ReferenceType
    /// @param Callback SequencesResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void GetAllSequencesContainingItems(const csp::common::Array<csp::common::String>& Items,
        const csp::common::Optional<csp::common::String>& ReferenceType, const csp::common::Array<csp::common::String>& ReferenceIds,
        SequencesResultCallback Callback);

    /// @brief Gets a sequence by it's key
    /// @note This call will fail (Reason InvalidSequenceKey) if the SequenceKey parameter contains invalid keys, such as spaces, '/' or '%'
    /// @param SequenceKey csp::common::String : The unique grouping name
    /// @param Callback SequenceResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void GetSequence(const csp::common::String& SequenceKey, SequenceResultCallback Callback);

    /// @brief Deletes the given sequences. This call will fail if the user isn't a creator of the space
    /// @note This call will fail (Reason InvalidSequenceKey) if the SequenceKey parameter contains invalid keys, such as spaces, '/' or '%'
    /// @param SequenceKeys csp::common::Array<csp::common::String> : An array of sequence keys to delete
    /// @param Callback NullResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void DeleteSequences(const csp::common::Array<csp::common::String>& SequenceKeys, NullResultCallback Callback);

    // Callback to receive sequence changes, contains a SequenceChangedParams with the details.
    typedef std::function<void(const csp::multiplayer::SequenceChangedParams&)> SequenceChangedCallbackHandler;

    /// @brief Sets a callback for a sequence changed event.
    /// @param Callback SequenceChangedCallbackHandler: Callback to receive data for the sequence that has been changed.
    CSP_EVENT void SetSequenceChangedCallback(SequenceChangedCallbackHandler Callback);

    /// @brief Registers the system to listen for the named event.
    void RegisterSystemCallback() override;
    /// @brief Deregisters the system from listening for the named event.
    void DeregisterSystemCallback() override;
    /// @brief Deserialises the event values of the system.
    /// @param EventValues std::vector<signalr::value> : event values to deserialise
    CSP_NO_EXPORT void OnEvent(const std::vector<signalr::value>& EventValues) override;

private:
    SequenceSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    SequenceSystem(csp::web::WebClient* InWebClient, csp::multiplayer::EventBus* InEventBus);
    ~SequenceSystem();

    csp::services::ApiBase* SequenceAPI;

    SequenceChangedCallbackHandler SequenceChangedCallback;
};
} // namespace csp::systems
