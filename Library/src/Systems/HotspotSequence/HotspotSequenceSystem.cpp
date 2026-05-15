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

#include "CSP/Systems/HotspotSequence/HotspotSequenceSystem.h"

#include "CSP/Common/NetworkEventData.h"
#include "CSP/Systems/HotspotSequence/HotspotGroup.h"
#include "CSP/Systems/Sequence/SequenceSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "Debug/Logging.h"
#include "Multiplayer/NetworkEventSerialisation.h"
#include "Systems/ResultHelpers.h"

#include <regex>
#include <string>

namespace csp::systems
{

namespace
{
    csp::common::String CreateKey(const csp::common::String& key, const csp::common::String& spaceId) { return "Hotspots:" + spaceId + ":" + key; }

    csp::common::Optional<csp::common::String> DeconstructKey(const csp::common::String& key, const csp::common::String& spaceId)
    {
        const csp::common::String expectedPrefix = "Hotspots:" + spaceId + ":";

        if (key.Length() <= expectedPrefix.Length() || !key.StartsWith(expectedPrefix))
        {
            return nullptr;
        }

        return csp::common::String(key.c_str() + expectedPrefix.Length(), key.Length() - expectedPrefix.Length());
    }

    void DeleteSequences(const std::vector<systems::Sequence>& sequences, csp::systems::NullResultCallback callback)
    {
        systems::SequenceSystem* sequenceSystem = systems::SystemsManager::Get().GetSequenceSystem();

        // Remove necessary sequences
        common::Array<common::String> deletionKeys(sequences.size());

        for (size_t i = 0; i < sequences.size(); ++i)
        {
            deletionKeys[i] = sequences[i].Key;
        }

        auto deleteCallback = [callback](const csp::systems::NullResult& deleteResult)
        {
            if (deleteResult.GetResultCode() == systems::EResultCode::InProgress)
            {
                return;
            }

            callback(deleteResult);
        };

        sequenceSystem->DeleteSequences(deletionKeys, deleteCallback);
    }

    void UpdateSequences(
        const std::vector<systems::Sequence>& sequences, const csp::common::String& itemToRemove, csp::systems::NullResultCallback callback)
    {
        systems::SequenceSystem* sequenceSystem = systems::SystemsManager::Get().GetSequenceSystem();

        for (const auto& sequence : sequences)
        {
            // Remove key from items array
            common::Array<common::String> items = sequence.Items;
            auto itemsList = items.ToList();
            itemsList.RemoveItem(itemToRemove);
            items = itemsList.ToArray();

            auto updateCb = [callback](const systems::SequenceResult& result)
            {
                if (result.GetResultCode() == systems::EResultCode::InProgress)
                {
                    return;
                }

                callback(systems::NullResult(result.GetResultCode(), result.GetHttpResultCode()));
            };

            sequenceSystem->UpdateSequence(sequence.Key, sequence.ReferenceType, sequence.ReferenceId, items, sequence.MetaData, updateCb);
        }
    }

} // namespace

HotspotSequenceSystem::HotspotSequenceSystem(csp::systems::SequenceSystem* sequenceSystem, csp::systems::SpaceSystem* spaceSystem,
    csp::multiplayer::NetworkEventBus& eventBus, csp::common::LogSystem& logSystem)
    : SystemBase(&eventBus, &logSystem)
{
    this->m_sequenceSystem = sequenceSystem;
    this->m_spaceSystem = spaceSystem;

    RegisterSystemCallback();
}

void HotspotSequenceSystem::CreateHotspotGroup(
    const csp::common::String& groupName, const csp::common::Array<csp::common::String>& hotspotIds, HotspotGroupResultCallback callback)
{
    const auto spaceId = m_spaceSystem->GetCurrentSpace().Id;

    auto cb = [callback, spaceId](const SequenceResult& result)
    {
        if (result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (result.GetResultCode() == EResultCode::Failed)
        {
            HotspotGroupResult returnValue(result.GetResultCode(), result.GetHttpResultCode());
            callback(returnValue);

            return;
        }

        const auto data = result.GetSequence();
        HotspotGroup group;
        group.Items = data.Items;

        if (auto deconstructedKey = DeconstructKey(data.Key, spaceId); deconstructedKey.HasValue())
        {
            group.Name = *deconstructedKey;
        }
        else
        {
            CSP_LOG_ERROR_FORMAT("HotspotSequenceSystem::CreateHotspotGroup - Failed to deconstruct key: %s", data.Key.c_str());

            callback(MakeInvalid<HotspotGroupResult>());

            return;
        }

        HotspotGroupResult returnValue(group, result.GetResultCode(), result.GetHttpResultCode());
        // convert SequenceResult To HotspotGroupResultCallback
        callback(returnValue);
    };

    const auto key = CreateKey(groupName, spaceId);
    m_sequenceSystem->CreateSequence(key, "GroupId", spaceId, hotspotIds, {}, cb);
}

void HotspotSequenceSystem::RenameHotspotGroup(
    const csp::common::String& groupName, const csp::common::String& newGroupName, HotspotGroupResultCallback callback)
{
    const auto spaceId = m_spaceSystem->GetCurrentSpace().Id;

    const auto key = CreateKey(groupName, spaceId);
    const auto newKey = CreateKey(newGroupName, spaceId);

    auto cb = [callback, spaceId](const SequenceResult& result)
    {
        if (result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (result.GetResultCode() == EResultCode::Failed)
        {
            HotspotGroupResult returnValue(result.GetResultCode(), result.GetHttpResultCode());
            callback(returnValue);

            return;
        }

        const auto data = result.GetSequence();
        HotspotGroup group;
        group.Items = data.Items;

        if (auto deconstructedKey = DeconstructKey(data.Key, spaceId); deconstructedKey.HasValue())
        {
            group.Name = *deconstructedKey;
        }
        else
        {
            CSP_LOG_ERROR_FORMAT("HotspotSequenceSystem::RenameHotspotGroup - Failed to deconstruct key: %s", data.Key.c_str());

            callback(MakeInvalid<HotspotGroupResult>());

            return;
        }

        HotspotGroupResult returnValue(group, result.GetResultCode(), result.GetHttpResultCode());
        // convert SequenceResult To HotspotGroupResultCallback
        callback(returnValue);
    };

    m_sequenceSystem->RenameSequence(key, newKey, cb);
}

void HotspotSequenceSystem::UpdateHotspotGroup(
    const csp::common::String& groupName, const csp::common::Array<csp::common::String>& hotspotIds, HotspotGroupResultCallback callback)
{
    const auto spaceId = m_spaceSystem->GetCurrentSpace().Id;

    auto cb = [callback, spaceId](const SequenceResult& result)
    {
        if (result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (result.GetResultCode() == EResultCode::Failed)
        {
            HotspotGroupResult returnValue(result.GetResultCode(), result.GetHttpResultCode());
            callback(returnValue);

            return;
        }

        const auto data = result.GetSequence();
        HotspotGroup group;
        group.Items = data.Items;

        if (auto deconstructedKey = DeconstructKey(data.Key, spaceId); deconstructedKey.HasValue())
        {
            group.Name = *deconstructedKey;
        }
        else
        {
            CSP_LOG_ERROR_FORMAT("HotspotSequenceSystem::UpdateHotspotGroup - Failed to deconstruct key: %s", data.Key.c_str());

            callback(MakeInvalid<HotspotGroupResult>());

            return;
        }

        HotspotGroupResult returnValue(group, result.GetResultCode(), result.GetHttpResultCode());
        // convert SequenceResult To HotspotGroupResultCallback
        callback(returnValue);
    };

    const auto key = CreateKey(groupName, spaceId);
    m_sequenceSystem->UpdateSequence(key, "GroupId", spaceId, hotspotIds, {}, cb);
}

void HotspotSequenceSystem::GetHotspotGroup(const csp::common::String& groupName, HotspotGroupResultCallback callback)
{
    const auto spaceId = m_spaceSystem->GetCurrentSpace().Id;

    auto cb = [callback, spaceId](const SequenceResult& result)
    {
        if (result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (result.GetResultCode() == EResultCode::Failed)
        {
            HotspotGroupResult returnValue(result.GetResultCode(), result.GetHttpResultCode());
            callback(returnValue);

            return;
        }

        const auto data = result.GetSequence();
        HotspotGroup group;
        group.Items = data.Items;

        if (auto deconstructedKey = DeconstructKey(data.Key, spaceId); deconstructedKey.HasValue())
        {
            group.Name = *deconstructedKey;
        }
        else
        {
            CSP_LOG_ERROR_FORMAT("HotspotSequenceSystem::GetHotspotGroup - Failed to deconstruct key: %s", data.Key.c_str());

            callback(MakeInvalid<HotspotGroupResult>());

            return;
        }

        HotspotGroupResult returnValue(group, result.GetResultCode(), result.GetHttpResultCode());
        // convert SequenceResult To HotspotGroupResultCallback
        callback(returnValue);
    };
    const auto key = CreateKey(groupName, spaceId);
    m_sequenceSystem->GetSequence(key, cb);
}

void HotspotSequenceSystem::GetHotspotGroups(HotspotGroupsResultCallback callback)
{
    const auto spaceId = m_spaceSystem->GetCurrentSpace().Id;

    auto cb = [callback, spaceId](const SequencesResult& result)
    {
        if (result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (result.GetResultCode() == EResultCode::Failed)
        {
            HotspotGroupsResult returnValue(result.GetResultCode(), result.GetHttpResultCode());
            callback(returnValue);

            return;
        }

        const auto data = result.GetSequences();

        csp::common::Array<HotspotGroup> groups(data.Size());

        for (size_t i = 0; i < data.Size(); i++)
        {
            groups[i].Items = data[i].Items;

            if (auto deconstructedKey = DeconstructKey(data[i].Key, spaceId); deconstructedKey.HasValue())
            {
                groups[i].Name = *deconstructedKey;
            }
            else
            {
                CSP_LOG_ERROR_FORMAT("HotspotSequenceSystem::GetHotspotGroups - Failed to deconstruct key: %s", data[i].Key.c_str());
            }
        }

        HotspotGroupsResult returnValue(groups, result.GetResultCode(), result.GetHttpResultCode());
        // convert SequenceResult To HotspotGroupResultCallback
        callback(returnValue);
    };

    m_sequenceSystem->GetSequencesByCriteria({}, spaceId, "GroupId", { spaceId }, {}, cb);
}

void HotspotSequenceSystem::DeleteHotspotGroup(const csp::common::String& groupName, NullResultCallback callback)
{
    auto cb = [callback](const NullResult& result) { callback(result); };

    const auto spaceId = m_spaceSystem->GetCurrentSpace().Id;
    const auto key = CreateKey(groupName, spaceId);
    m_sequenceSystem->DeleteSequences({ key }, cb);
}

HotspotSequenceSystem::~HotspotSequenceSystem()
{
    m_spaceSystem = nullptr;
    m_sequenceSystem = nullptr;
}

void HotspotSequenceSystem::RemoveItemFromGroups(const csp::common::String& itemId, csp::systems::NullResultCallback /*Callback*/)
{
    // E.M: It's very easy to get the argument you need to pass into this method wrong.
    // The type provides no help, and you have to actually call GetUniqueComponentId on HotspotComponent
    // to get a `parentId:componentId` pattern.

    systems::SpaceSystem* mySpaceSystem = systems::SystemsManager::Get().GetSpaceSystem();
    // This uses multiple async calls, so ensure this variable exists within this function
    csp::common::String itemCopy = itemId;

    auto getSequencesCallback = [itemCopy](const systems::SequencesResult& sequencesResult)
    {
        if (sequencesResult.GetResultCode() == systems::EResultCode::InProgress)
        {
            return;
        }

        const auto& sequences = sequencesResult.GetSequences();

        std::vector<systems::Sequence> sequencesToDelete;
        std::vector<systems::Sequence> sequencesToUpdate;
        sequencesToDelete.reserve(sequences.Size());
        sequencesToUpdate.reserve(sequences.Size());

        for (size_t i = 0; i < sequences.Size(); ++i)
        {
            if (sequences[i].Items.Size() == 1)
            {
                // This is the only item in the sequence, so delete the sequence
                sequencesToDelete.push_back(sequences[i]);
            }
            else
            {
                // There are other items in this sequence, so only remove this item
                sequencesToUpdate.push_back(sequences[i]);
            }
        }

        auto cb = [](const systems::NullResult& /*Res*/) {

        };

        DeleteSequences(sequencesToDelete, cb);
        UpdateSequences(sequencesToUpdate, itemCopy, cb);
    };

    // Find all sequences containing this name
    m_sequenceSystem->GetAllSequencesContainingItems({ itemCopy }, "GroupId", { mySpaceSystem->GetCurrentSpace().Id }, getSequencesCallback);
}

HotspotSequenceSystem::HotspotSequenceSystem(csp::common::LogSystem& logSystem)
    : SystemBase(nullptr, nullptr, &logSystem)
{
    m_spaceSystem = nullptr;
    m_sequenceSystem = nullptr;
}

void HotspotSequenceSystem::SetHotspotSequenceChangedCallback(HotspotSequenceChangedCallbackHandler callback)
{
    m_hotspotSequenceChangedCallback = callback;
    RegisterSystemCallback();
}

void HotspotSequenceSystem::RegisterSystemCallback()
{
    if (!m_hotspotSequenceChangedCallback)
    {
        return;
    }

    m_eventBusPtr->ListenNetworkEvent(
        csp::multiplayer::NetworkEventRegistration("CSPInternal::HotspotSequenceSystem",
            csp::multiplayer::NetworkEventBus::StringFromNetworkEvent(csp::multiplayer::NetworkEventBus::NetworkEvent::SequenceChanged)),
        [this](const csp::common::NetworkEventData& networkEventData) { this->OnSequenceChangedEvent(networkEventData); });
}

void HotspotSequenceSystem::OnSequenceChangedEvent(const csp::common::NetworkEventData& networkEventData)
{
    // This event may either represent a default sequence or a hotspot sequence. Here we are only concerned with hotspot sequences.
    const auto& sequenceEvent = static_cast<const csp::common::SequenceChangedNetworkEventData&>(networkEventData);

    if (sequenceEvent.SequenceType == csp::common::ESequenceType::Hotspot && m_hotspotSequenceChangedCallback)
    {
        // We can cast directly, we're sure we're the correct type.
        m_hotspotSequenceChangedCallback(sequenceEvent);
    }
}

} // namespace csp::systems
