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

#include "CSP/Multiplayer/EventParameters.h"
#include "CSP/Systems/HotspotSequence/HotspotGroup.h"
#include "CSP/Systems/Sequence/SequenceSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "Debug/Logging.h"
#include "Multiplayer/EventSerialisation.h"

#include <regex>
#include <string>

namespace csp::systems
{

namespace
{
    csp::common::String CreateKey(const csp::common::String& Key, const csp::common::String& SpaceId) { return "Hotspots:" + SpaceId + ":" + Key; }

    void DeleteSequences(const std::vector<systems::Sequence>& Sequences, csp::systems::NullResultCallback Callback)
    {
        systems::SequenceSystem* SequenceSystem = systems::SystemsManager::Get().GetSequenceSystem();

        // Remove necessary sequences
        common::Array<common::String> DeletionKeys(Sequences.size());

        for (size_t i = 0; i < Sequences.size(); ++i)
        {
            DeletionKeys[i] = Sequences[i].Key;
        }

        auto DeleteCallback = [Callback, SequenceSystem](const csp::systems::NullResult& DeleteResult)
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

HotspotSequenceSystem::HotspotSequenceSystem(
    csp::systems::SequenceSystem* SequenceSystem, csp::systems::SpaceSystem* SpaceSystem, csp::multiplayer::EventBus* EventBus)
    : SystemBase(EventBus)
{
    this->SequenceSystem = SequenceSystem;
    this->SpaceSystem = SpaceSystem;

    RegisterSystemCallback();
}

void HotspotSequenceSystem::CreateHotspotGroup(
    const csp::common::String& GroupName, const csp::common::Array<csp::common::String>& HotspotIds, HotspotGroupResultCallback Callback)
{
    auto CB = [Callback](const SequenceResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            HotspotGroupResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
            // convert SequenceResult To HotspotGroupResultCallback
            Callback(ReturnValue);
            return;
        }

        auto Data = Result.GetSequence();
        HotspotGroup Group;
        Group.Items = Data.Items;
        Group.Name = Data.MetaData["Name"];

        HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
        // convert SequenceResult To HotspotGroupResultCallback
        Callback(ReturnValue);
    };

    auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
    auto Key = CreateKey(GroupName, SpaceId);
    csp::common::Map<csp::common::String, csp::common::String> MetaData;
    MetaData["Name"] = GroupName;
    SequenceSystem->CreateSequence(Key, "GroupId", SpaceId, HotspotIds, MetaData, CB);
}

void HotspotSequenceSystem::RenameHotspotGroup(
    const csp::common::String& GroupName, const csp::common::String& NewGroupName, HotspotGroupResultCallback Callback)
{
    auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
    auto Key = CreateKey(GroupName, SpaceId);
    auto NewKey = CreateKey(NewGroupName, SpaceId);
    auto SQ = this->SequenceSystem;

    auto CB = [Callback, SQ, NewKey, NewGroupName, SpaceId](const SequenceResult& Result)
    {
        if (Result.GetResultCode() != csp::systems::EResultCode::Success)
        {
            HotspotGroupResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
            // convert SequenceResult To HotspotGroupResultCallback
            Callback(ReturnValue);
            return;
        }

        auto UpdateCB = [Callback, SQ, NewKey](const SequenceResult& Result)
        {
            if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
            {
                HotspotGroupResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
                // convert SequenceResult To HotspotGroupResultCallback
                Callback(ReturnValue);
                return;
            }
            auto Data = Result.GetSequence();
            HotspotGroup Group;
            Group.Items = Data.Items;
            Group.Name = Data.MetaData["Name"];
            HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
            // convert SequenceResult To HotspotGroupResultCallback
            Callback(ReturnValue);
        };
        auto MetaData = Result.GetSequence().MetaData;
        MetaData["Name"] = NewGroupName;
        SQ->UpdateSequence(NewKey, "GroupId", SpaceId, Result.GetSequence().Items, MetaData, UpdateCB);
    };

    SequenceSystem->RenameSequence(Key, NewKey, CB);
}

void HotspotSequenceSystem::UpdateHotspotGroup(
    const csp::common::String& GroupName, const csp::common::Array<csp::common::String>& HotspotIds, HotspotGroupResultCallback Callback)
{
    auto CB = [Callback](const SequenceResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            HotspotGroupResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
            // convert SequenceResult To HotspotGroupResultCallback
            Callback(ReturnValue);
            return;
        }
        auto Data = Result.GetSequence();
        HotspotGroup Group;
        Group.Items = Data.Items;
        Group.Name = Data.MetaData["Name"];
        HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
        // convert SequenceResult To HotspotGroupResultCallback
        Callback(ReturnValue);
    };

    auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
    auto Key = CreateKey(GroupName, SpaceId);
    csp::common::Map<csp::common::String, csp::common::String> MetaData;
    MetaData["Name"] = GroupName;
    SequenceSystem->UpdateSequence(Key, "GroupId", SpaceId, HotspotIds, MetaData, CB);
}

void HotspotSequenceSystem::GetHotspotGroup(const csp::common::String& GroupName, HotspotGroupResultCallback Callback)
{
    auto CB = [Callback](const SequenceResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            HotspotGroupResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
            // convert SequenceResult To HotspotGroupResultCallback
            Callback(ReturnValue);
            return;
        }
        auto Data = Result.GetSequence();
        HotspotGroup Group;
        Group.Items = Data.Items;
        Group.Name = Data.MetaData["Name"];
        HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
        // convert SequenceResult To HotspotGroupResultCallback
        Callback(ReturnValue);
    };
    auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
    auto Key = CreateKey(GroupName, SpaceId);
    SequenceSystem->GetSequence(Key, CB);
}

void HotspotSequenceSystem::GetHotspotGroups(HotspotGroupsResultCallback Callback)
{
    auto CB = [Callback](const SequencesResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            HotspotGroupsResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
            // convert SequenceResult To HotspotGroupResultCallback
            Callback(ReturnValue);
            return;
        }
        auto Data = Result.GetSequences();

        csp::common::Array<HotspotGroup> Groups(Data.Size());

        for (size_t i = 0; i < Data.Size(); i++)
        {
            Groups[i].Items = Data[i].Items;
            Groups[i].Name = Data[i].MetaData["Name"];
        }
        HotspotGroupsResult ReturnValue(Groups, Result.GetResultCode(), Result.GetHttpResultCode());
        // convert SequenceResult To HotspotGroupResultCallback
        Callback(ReturnValue);
    };
    auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
    csp::common::Map<csp::common::String, csp::common::String> MetaData;
    SequenceSystem->GetSequencesByCriteria({}, SpaceId, "GroupId", { SpaceId }, MetaData, CB);
}

void HotspotSequenceSystem::DeleteHotspotGroup(const csp::common::String& GroupName, NullResultCallback Callback)
{
    auto CB = [Callback](const NullResult& Result) { Callback(Result); };

    auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
    auto Key = CreateKey(GroupName, SpaceId);
    SequenceSystem->DeleteSequences({ Key }, CB);
}
HotspotSequenceSystem::~HotspotSequenceSystem()
{
    SpaceSystem = nullptr;
    SequenceSystem = nullptr;

    DeregisterSystemCallback();
}

void HotspotSequenceSystem::RemoveItemFromGroups(const csp::common::String& ItemName, csp::systems::NullResultCallback Callback)
{
    systems::SpaceSystem* SpaceSystem = systems::SystemsManager::Get().GetSpaceSystem();
    // This uses multiple async calls, so ensure this variable exists within this function
    csp::common::String ItemCopy = ItemName;

    auto GetSequencesCallback = [this, Callback, ItemCopy](const systems::SequencesResult& SequencesResult)
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

        auto CB = [](const systems::NullResult& Res) {

        };

        DeleteSequences(SequencesToDelete, CB);
        UpdateSequences(SequencesToUpdate, ItemCopy, CB);
    };

    // Find all sequences containing this name
    SequenceSystem->GetAllSequencesContainingItems({ ItemCopy }, "GroupId", { SpaceSystem->GetCurrentSpace().Id }, GetSequencesCallback);
}

HotspotSequenceSystem::HotspotSequenceSystem()
    : SystemBase(nullptr, nullptr)
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

    EventBusPtr->ListenNetworkEvent("SequenceChanged", this);
}

void HotspotSequenceSystem::DeregisterSystemCallback()
{
    if (EventBusPtr)
    {
        EventBusPtr->StopListenNetworkEvent("SequenceChanged");
    }
}

void HotspotSequenceSystem::OnEvent(const std::vector<signalr::value>& EventValues)
{
    csp::multiplayer::SequenceChangedEventDeserialiser SequenceDeserialiser;
    SequenceDeserialiser.Parse(EventValues);

    // There are a variety of sequence types.
    // Other CSP callbacks may also need to fire if the sequence change relates to a particular sequence type.
    const csp::common::String Key = SequenceDeserialiser.GetEventParams().Key;
    const csp::common::String SequenceType = csp::multiplayer::GetSequenceKeyIndex(Key, 0);
    if (SequenceType == "Hotspots")
    {
        if (HotspotSequenceChangedCallback)
        {
            csp::multiplayer::SequenceHotspotChangedEventDeserialiser HotspotDeserialiser;
            HotspotDeserialiser.Parse(EventValues);
            HotspotSequenceChangedCallback(HotspotDeserialiser.GetEventParams());
        }
    }
}

} // namespace csp::systems
