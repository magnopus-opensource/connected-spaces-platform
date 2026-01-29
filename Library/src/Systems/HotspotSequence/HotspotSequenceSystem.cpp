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
    csp::common::String CreateKey(const csp::common::String& Key, const csp::common::String& SpaceId) { return "Hotspots:" + SpaceId + ":" + Key; }

    csp::common::Optional<csp::common::String> DeconstructKey(const csp::common::String& Key, const csp::common::String& SpaceId)
    {
        const csp::common::String ExpectedPrefix = "Hotspots:" + SpaceId + ":";

        if (Key.Length() <= ExpectedPrefix.Length() || !Key.StartsWith(ExpectedPrefix))
        {
            return nullptr;
        }

        return csp::common::String(Key.c_str() + ExpectedPrefix.Length(), Key.Length() - ExpectedPrefix.Length());
    }

    void DeleteSequences(const std::vector<systems::Sequence>& Sequences, csp::systems::NullResultCallback Callback)
    {
        systems::SequenceSystem* SequenceSystem = systems::SystemsManager::Get().GetSequenceSystem();

        // Remove necessary sequences
        common::Array<common::String> DeletionKeys(Sequences.size());

        for (size_t i = 0; i < Sequences.size(); ++i)
        {
            DeletionKeys[i] = Sequences[i].Key;
        }

        auto DeleteCallback = [Callback](const csp::systems::NullResult& DeleteResult)
        {
            if (DeleteResult.GetResultCode() == systems::EResultCode::InProgress)
            {
                return;
            }

            Callback(DeleteResult);
        };

        SequenceSystem->DeleteSequences(DeletionKeys, DeleteCallback);
    }

    void UpdateSequences(
        const std::vector<systems::Sequence>& Sequences, const csp::common::String& ItemToRemove, csp::systems::NullResultCallback Callback)
    {
        systems::SequenceSystem* SequenceSystem = systems::SystemsManager::Get().GetSequenceSystem();

        for (const auto& Sequence : Sequences)
        {
            // Remove key from items array
            common::Array<common::String> Items = Sequence.Items;
            auto ItemsList = Items.ToList();
            ItemsList.RemoveItem(ItemToRemove);
            Items = ItemsList.ToArray();

            auto UpdateCB = [Callback](const systems::SequenceResult& Result)
            {
                if (Result.GetResultCode() == systems::EResultCode::InProgress)
                {
                    return;
                }

                Callback(systems::NullResult(Result.GetResultCode(), Result.GetHttpResultCode()));
            };

            SequenceSystem->UpdateSequence(Sequence.Key, Sequence.ReferenceType, Sequence.ReferenceId, Items, Sequence.MetaData, UpdateCB);
        }
    }

} // namespace

HotspotSequenceSystem::HotspotSequenceSystem(csp::systems::SequenceSystem* SequenceSystem, csp::systems::SpaceSystem* SpaceSystem,
    csp::multiplayer::NetworkEventBus& EventBus, csp::common::LogSystem& LogSystem)
    : SystemBase(&EventBus, &LogSystem)
{
    this->SequenceSystem = SequenceSystem;
    this->SpaceSystem = SpaceSystem;

    RegisterSystemCallback();
}

void HotspotSequenceSystem::CreateHotspotGroup(
    const csp::common::String& GroupName, const csp::common::Array<csp::common::String>& HotspotIds, HotspotGroupResultCallback Callback)
{
    const auto SpaceId = SpaceSystem->GetCurrentSpace().Id;

    auto CB = [Callback, SpaceId](const SequenceResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (Result.GetResultCode() == EResultCode::Failed)
        {
            HotspotGroupResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
            Callback(ReturnValue);

            return;
        }

        const auto Data = Result.GetSequence();
        HotspotGroup Group;
        Group.Items = Data.Items;

        if (auto DeconstructedKey = DeconstructKey(Data.Key, SpaceId); DeconstructedKey.HasValue())
        {
            Group.Name = *DeconstructedKey;
        }
        else
        {
            CSP_LOG_ERROR_FORMAT("HotspotSequenceSystem::CreateHotspotGroup - Failed to deconstruct key: %s", Data.Key.c_str());

            Callback(MakeInvalid<HotspotGroupResult>());

            return;
        }

        HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
        // convert SequenceResult To HotspotGroupResultCallback
        Callback(ReturnValue);
    };

    const auto Key = CreateKey(GroupName, SpaceId);
    SequenceSystem->CreateSequence(Key, "GroupId", SpaceId, HotspotIds, {}, CB);
}

void HotspotSequenceSystem::RenameHotspotGroup(
    const csp::common::String& GroupName, const csp::common::String& NewGroupName, HotspotGroupResultCallback Callback)
{
    const auto SpaceId = SpaceSystem->GetCurrentSpace().Id;

    const auto Key = CreateKey(GroupName, SpaceId);
    const auto NewKey = CreateKey(NewGroupName, SpaceId);

    auto CB = [Callback, SpaceId](const SequenceResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (Result.GetResultCode() == EResultCode::Failed)
        {
            HotspotGroupResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
            Callback(ReturnValue);

            return;
        }

        const auto Data = Result.GetSequence();
        HotspotGroup Group;
        Group.Items = Data.Items;

        if (auto DeconstructedKey = DeconstructKey(Data.Key, SpaceId); DeconstructedKey.HasValue())
        {
            Group.Name = *DeconstructedKey;
        }
        else
        {
            CSP_LOG_ERROR_FORMAT("HotspotSequenceSystem::RenameHotspotGroup - Failed to deconstruct key: %s", Data.Key.c_str());

            Callback(MakeInvalid<HotspotGroupResult>());

            return;
        }

        HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
        // convert SequenceResult To HotspotGroupResultCallback
        Callback(ReturnValue);
    };

    SequenceSystem->RenameSequence(Key, NewKey, CB);
}

void HotspotSequenceSystem::UpdateHotspotGroup(
    const csp::common::String& GroupName, const csp::common::Array<csp::common::String>& HotspotIds, HotspotGroupResultCallback Callback)
{
    const auto SpaceId = SpaceSystem->GetCurrentSpace().Id;

    auto CB = [Callback, SpaceId](const SequenceResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (Result.GetResultCode() == EResultCode::Failed)
        {
            HotspotGroupResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
            Callback(ReturnValue);

            return;
        }

        const auto Data = Result.GetSequence();
        HotspotGroup Group;
        Group.Items = Data.Items;

        if (auto DeconstructedKey = DeconstructKey(Data.Key, SpaceId); DeconstructedKey.HasValue())
        {
            Group.Name = *DeconstructedKey;
        }
        else
        {
            CSP_LOG_ERROR_FORMAT("HotspotSequenceSystem::UpdateHotspotGroup - Failed to deconstruct key: %s", Data.Key.c_str());

            Callback(MakeInvalid<HotspotGroupResult>());

            return;
        }

        HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
        // convert SequenceResult To HotspotGroupResultCallback
        Callback(ReturnValue);
    };

    const auto Key = CreateKey(GroupName, SpaceId);
    SequenceSystem->UpdateSequence(Key, "GroupId", SpaceId, HotspotIds, {}, CB);
}

void HotspotSequenceSystem::GetHotspotGroup(const csp::common::String& GroupName, HotspotGroupResultCallback Callback)
{
    const auto SpaceId = SpaceSystem->GetCurrentSpace().Id;

    auto CB = [Callback, SpaceId](const SequenceResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (Result.GetResultCode() == EResultCode::Failed)
        {
            HotspotGroupResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
            Callback(ReturnValue);

            return;
        }

        const auto Data = Result.GetSequence();
        HotspotGroup Group;
        Group.Items = Data.Items;

        if (auto DeconstructedKey = DeconstructKey(Data.Key, SpaceId); DeconstructedKey.HasValue())
        {
            Group.Name = *DeconstructedKey;
        }
        else
        {
            CSP_LOG_ERROR_FORMAT("HotspotSequenceSystem::GetHotspotGroup - Failed to deconstruct key: %s", Data.Key.c_str());

            Callback(MakeInvalid<HotspotGroupResult>());

            return;
        }

        HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
        // convert SequenceResult To HotspotGroupResultCallback
        Callback(ReturnValue);
    };
    const auto Key = CreateKey(GroupName, SpaceId);
    SequenceSystem->GetSequence(Key, CB);
}

void HotspotSequenceSystem::GetHotspotGroups(HotspotGroupsResultCallback Callback)
{
    const auto SpaceId = SpaceSystem->GetCurrentSpace().Id;

    auto CB = [Callback, SpaceId](const SequencesResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (Result.GetResultCode() == EResultCode::Failed)
        {
            HotspotGroupsResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
            Callback(ReturnValue);

            return;
        }

        const auto Data = Result.GetSequences();

        csp::common::Array<HotspotGroup> Groups(Data.Size());

        for (size_t i = 0; i < Data.Size(); i++)
        {
            Groups[i].Items = Data[i].Items;

            if (auto DeconstructedKey = DeconstructKey(Data[i].Key, SpaceId); DeconstructedKey.HasValue())
            {
                Groups[i].Name = *DeconstructedKey;
            }
            else
            {
                CSP_LOG_ERROR_FORMAT("HotspotSequenceSystem::GetHotspotGroups - Failed to deconstruct key: %s", Data[i].Key.c_str());
            }
        }

        HotspotGroupsResult ReturnValue(Groups, Result.GetResultCode(), Result.GetHttpResultCode());
        // convert SequenceResult To HotspotGroupResultCallback
        Callback(ReturnValue);
    };

    SequenceSystem->GetSequencesByCriteria({}, SpaceId, "GroupId", { SpaceId }, {}, CB);
}

void HotspotSequenceSystem::DeleteHotspotGroup(const csp::common::String& GroupName, NullResultCallback Callback)
{
    auto CB = [Callback](const NullResult& Result) { Callback(Result); };

    const auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
    const auto Key = CreateKey(GroupName, SpaceId);
    SequenceSystem->DeleteSequences({ Key }, CB);
}

HotspotSequenceSystem::~HotspotSequenceSystem()
{
    SpaceSystem = nullptr;
    SequenceSystem = nullptr;
}

void HotspotSequenceSystem::RemoveItemFromGroups(const csp::common::String& ItemID, csp::systems::NullResultCallback /*Callback*/)
{
    // E.M: It's very easy to get the argument you need to pass into this method wrong.
    // The type provides no help, and you have to actually call GetUniqueComponentId on HotspotComponent
    // to get a `parentId:componentId` pattern.

    systems::SpaceSystem* MySpaceSystem = systems::SystemsManager::Get().GetSpaceSystem();
    // This uses multiple async calls, so ensure this variable exists within this function
    csp::common::String ItemCopy = ItemID;

    auto GetSequencesCallback = [ItemCopy](const systems::SequencesResult& SequencesResult)
    {
        if (SequencesResult.GetResultCode() == systems::EResultCode::InProgress)
        {
            return;
        }

        const auto& Sequences = SequencesResult.GetSequences();

        std::vector<systems::Sequence> SequencesToDelete;
        std::vector<systems::Sequence> SequencesToUpdate;
        SequencesToDelete.reserve(Sequences.Size());
        SequencesToUpdate.reserve(Sequences.Size());

        for (size_t i = 0; i < Sequences.Size(); ++i)
        {
            if (Sequences[i].Items.Size() == 1)
            {
                // This is the only item in the sequence, so delete the sequence
                SequencesToDelete.push_back(Sequences[i]);
            }
            else
            {
                // There are other items in this sequence, so only remove this item
                SequencesToUpdate.push_back(Sequences[i]);
            }
        }

        auto CB = [](const systems::NullResult& /*Res*/) {

        };

        DeleteSequences(SequencesToDelete, CB);
        UpdateSequences(SequencesToUpdate, ItemCopy, CB);
    };

    // Find all sequences containing this name
    SequenceSystem->GetAllSequencesContainingItems({ ItemCopy }, "GroupId", { MySpaceSystem->GetCurrentSpace().Id }, GetSequencesCallback);
}

HotspotSequenceSystem::HotspotSequenceSystem(csp::common::LogSystem& LogSystem)
    : SystemBase(nullptr, nullptr, &LogSystem)
{
    SpaceSystem = nullptr;
    SequenceSystem = nullptr;
}

void HotspotSequenceSystem::SetHotspotSequenceChangedCallback(HotspotSequenceChangedCallbackHandler Callback)
{
    HotspotSequenceChangedCallback = Callback;
    RegisterSystemCallback();
}

void HotspotSequenceSystem::RegisterSystemCallback()
{
    if (!HotspotSequenceChangedCallback)
    {
        return;
    }

    EventBusPtr->ListenNetworkEvent(
        csp::multiplayer::NetworkEventRegistration("CSPInternal::HotspotSequenceSystem",
            csp::multiplayer::NetworkEventBus::StringFromNetworkEvent(csp::multiplayer::NetworkEventBus::NetworkEvent::SequenceChanged)),
        [this](const csp::common::NetworkEventData& NetworkEventData) { this->OnSequenceChangedEvent(NetworkEventData); });
}

void HotspotSequenceSystem::OnSequenceChangedEvent(const csp::common::NetworkEventData& NetworkEventData)
{
    // This event may either represent a default sequence or a hotspot sequence. Here we are only concerned with hotspot sequences.
    const auto& SequenceEvent = static_cast<const csp::common::SequenceChangedNetworkEventData&>(NetworkEventData);

    if (SequenceEvent.SequenceType == csp::common::ESequenceType::Hotspot && HotspotSequenceChangedCallback)
    {
        // We can cast directly, we're sure we're the correct type.
        HotspotSequenceChangedCallback(SequenceEvent);
    }
}

} // namespace csp::systems
