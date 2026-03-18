/*
 * Copyright 2026 Magnopus LLC

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

#include "CSP/Systems/Assets/RuntimeMaterialSystem.h"

#include "CSP/CSPFoundation.h"
#include "CSP/Common/NetworkEventData.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/NetworkEventBus.h"
#include "CSP/Systems/Assets/AlphaVideoMaterial.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Assets/GLTFMaterial.h"
#include "CSP/Systems/SystemsManager.h"

#include <fmt/format.h>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace
{

struct PendingChangedNotification
{
    csp::common::String MaterialId;
    csp::common::String MaterialPath;
    uint64_t EntityId = 0;
    int32_t ComponentType = -1;
    int32_t ComponentIndex = -1;
};

struct SourceEntry
{
    std::unique_ptr<csp::systems::Material> Source;
};

struct BindingEntry
{
    csp::common::String Handle;
    csp::common::String MaterialId;
    csp::common::String MaterialPath;
    uint64_t EntityId = 0;
    int32_t ComponentType = 0;
    int32_t ComponentIndex = 0;
    std::unique_ptr<csp::systems::Material> Live;
};

constexpr const char* ASSET_BLOB_CHANGED_RECEIVER_ID = "CSPInternal::RuntimeMaterialSystem";

std::unique_ptr<csp::systems::Material> CloneMaterial(const csp::systems::Material& Material)
{
    if (Material.GetShaderType() == csp::systems::EShaderType::Standard)
    {
        const auto* GLTF = static_cast<const csp::systems::GLTFMaterial*>(&Material);
        return std::make_unique<csp::systems::GLTFMaterial>(*GLTF);
    }

    if (Material.GetShaderType() == csp::systems::EShaderType::AlphaVideo)
    {
        const auto* AlphaVideo = static_cast<const csp::systems::AlphaVideoMaterial*>(&Material);
        return std::make_unique<csp::systems::AlphaVideoMaterial>(*AlphaVideo);
    }

    return nullptr;
}

std::string MakeBindingHandle(uint64_t EntityId, int32_t ComponentType, int32_t ComponentIndex, const csp::common::String& MaterialPath)
{
    return fmt::format("binding:{}:{}:{}:{}", EntityId, ComponentType, ComponentIndex, MaterialPath.c_str());
}

csp::systems::RuntimeMaterialState MakeState(csp::systems::RuntimeMaterialStatus Status, const csp::common::String& Handle,
    const csp::common::String& MaterialId, const csp::common::String& MaterialPath, uint64_t EntityId, int32_t ComponentType,
    int32_t ComponentIndex, csp::systems::Material* MaterialRef)
{
    csp::systems::RuntimeMaterialState State;
    State.Status = Status;
    State.Handle = Handle;
    State.MaterialId = MaterialId;
    State.MaterialPath = MaterialPath;
    State.EntityId = EntityId;
    State.ComponentType = ComponentType;
    State.ComponentIndex = ComponentIndex;
    State.MaterialRef = MaterialRef;
    return State;
}

bool ApplyPatch(csp::systems::Material& Material, const csp::systems::RuntimeMaterialPatch& Patch)
{
    if (Material.GetShaderType() == csp::systems::EShaderType::Standard)
    {
        auto* GLTF = static_cast<csp::systems::GLTFMaterial*>(&Material);
        if (Patch.HasBaseColorFactor)
        {
            GLTF->SetBaseColorFactor(Patch.BaseColorFactor);
        }
        if (Patch.HasMetallicFactor)
        {
            GLTF->SetMetallicFactor(Patch.MetallicFactor);
        }
        if (Patch.HasRoughnessFactor)
        {
            GLTF->SetRoughnessFactor(Patch.RoughnessFactor);
        }
        if (Patch.HasEmissiveFactor)
        {
            GLTF->SetEmissiveFactor(Patch.EmissiveFactor);
        }
        if (Patch.HasEmissiveStrength)
        {
            GLTF->SetEmissiveStrength(Patch.EmissiveStrength);
        }
        if (Patch.HasAlphaCutoff)
        {
            GLTF->SetAlphaCutoff(Patch.AlphaCutoff);
        }
        if (Patch.HasDoubleSided)
        {
            GLTF->SetDoubleSided(Patch.DoubleSided);
        }

        return Patch.HasBaseColorFactor || Patch.HasMetallicFactor || Patch.HasRoughnessFactor || Patch.HasEmissiveFactor
            || Patch.HasEmissiveStrength || Patch.HasAlphaCutoff || Patch.HasDoubleSided;
    }

    if (Material.GetShaderType() == csp::systems::EShaderType::AlphaVideo)
    {
        auto* AlphaVideo = static_cast<csp::systems::AlphaVideoMaterial*>(&Material);
        if (Patch.HasTint)
        {
            AlphaVideo->SetTint(Patch.Tint);
        }
        if (Patch.HasAlphaFactor)
        {
            AlphaVideo->SetAlphaFactor(Patch.AlphaFactor);
        }
        if (Patch.HasEmissiveIntensity)
        {
            AlphaVideo->SetEmissiveIntensity(Patch.EmissiveIntensity);
        }
        if (Patch.HasFresnelFactor)
        {
            AlphaVideo->SetFresnelFactor(Patch.FresnelFactor);
        }
        if (Patch.HasAlphaMask)
        {
            AlphaVideo->SetAlphaMask(Patch.AlphaMask);
        }
        if (Patch.HasDoubleSided)
        {
            AlphaVideo->SetDoubleSided(Patch.DoubleSided);
        }
        if (Patch.HasIsEmissive)
        {
            AlphaVideo->SetIsEmissive(Patch.IsEmissive);
        }
        if (Patch.HasBlendMode)
        {
            AlphaVideo->SetBlendMode(static_cast<csp::systems::EBlendMode>(Patch.BlendMode));
        }
        if (Patch.HasReadAlphaFromChannel)
        {
            AlphaVideo->SetReadAlphaFromChannel(static_cast<csp::systems::EColorChannel>(Patch.ReadAlphaFromChannel));
        }

        return Patch.HasTint || Patch.HasAlphaFactor || Patch.HasEmissiveIntensity || Patch.HasFresnelFactor || Patch.HasAlphaMask
            || Patch.HasDoubleSided || Patch.HasIsEmissive || Patch.HasBlendMode || Patch.HasReadAlphaFromChannel;
    }

    return false;
}

} // namespace

namespace csp::systems
{

class RuntimeMaterialSystem::Impl
{
public:
    explicit Impl(csp::common::LogSystem& InLogSystem)
        : LogSystem(InLogSystem)
    {
    }

    csp::common::LogSystem& LogSystem;
    csp::common::String ActiveSpaceId;
    uint64_t Generation = 0;
    bool bListenerRegistered = false;
    bool bLoadingAll = false;
    std::unordered_map<std::string, SourceEntry> Sources;
    std::unordered_map<std::string, std::unique_ptr<Material>> GlobalLive;
    std::unordered_map<std::string, BindingEntry> Bindings;
    RuntimeMaterialChangedCallbackHandler ChangedCallback;
    std::mutex Mutex;
};

RuntimeMaterialSystem::RuntimeMaterialSystem(csp::common::LogSystem& InLogSystem)
    : ImplData(std::make_unique<Impl>(InLogSystem))
{
}

RuntimeMaterialSystem::~RuntimeMaterialSystem()
{
    OnExitSpace();
}

void RuntimeMaterialSystem::OnEnterSpace(const csp::common::String& InSpaceId)
{
    auto& State = *ImplData;
    std::scoped_lock Lock(State.Mutex);
    State.ActiveSpaceId = InSpaceId;
    ++State.Generation;
    State.Sources.clear();
    State.GlobalLive.clear();
    State.Bindings.clear();
    State.bLoadingAll = false;
    RegisterAssetDetailBlobChangedListener();
}

void RuntimeMaterialSystem::OnExitSpace()
{
    auto& State = *ImplData;
    {
        std::scoped_lock Lock(State.Mutex);
        State.ActiveSpaceId = "";
        ++State.Generation;
        State.Sources.clear();
        State.GlobalLive.clear();
        State.Bindings.clear();
        State.bLoadingAll = false;
    }

    UnregisterAssetDetailBlobChangedListener();
}

void RuntimeMaterialSystem::RegisterAssetDetailBlobChangedListener()
{
    auto& State = *ImplData;
    if (State.bListenerRegistered)
    {
        return;
    }

    auto* MultiplayerConnection = csp::systems::SystemsManager::Get().GetMultiplayerConnection();
    if (MultiplayerConnection == nullptr)
    {
        return;
    }

    auto* EventBus = &MultiplayerConnection->GetEventBus();
    const csp::common::String EventName
        = csp::multiplayer::NetworkEventBus::StringFromNetworkEvent(csp::multiplayer::NetworkEventBus::NetworkEvent::AssetDetailBlobChanged);
    EventBus->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(ASSET_BLOB_CHANGED_RECEIVER_ID, EventName),
        [this](const csp::common::NetworkEventData& NetworkEventData) { this->OnAssetDetailBlobChanged(NetworkEventData); });
    State.bListenerRegistered = true;
}

void RuntimeMaterialSystem::UnregisterAssetDetailBlobChangedListener()
{
    auto& State = *ImplData;
    if (!State.bListenerRegistered)
    {
        return;
    }

    if (auto* MultiplayerConnection = csp::systems::SystemsManager::Get().GetMultiplayerConnection())
    {
        auto* EventBus = &MultiplayerConnection->GetEventBus();
        const csp::common::String EventName
            = csp::multiplayer::NetworkEventBus::StringFromNetworkEvent(csp::multiplayer::NetworkEventBus::NetworkEvent::AssetDetailBlobChanged);
        EventBus->StopListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(ASSET_BLOB_CHANGED_RECEIVER_ID, EventName));
    }

    State.bListenerRegistered = false;
}

void RuntimeMaterialSystem::OnAssetDetailBlobChanged(const csp::common::NetworkEventData& NetworkEventData)
{
    const auto& AssetBlobEvent = static_cast<const csp::common::AssetDetailBlobChangedNetworkEventData&>(NetworkEventData);
    if (AssetBlobEvent.AssetType != csp::systems::EAssetType::MATERIAL)
    {
        return;
    }

    auto& State = *ImplData;
    bool bShouldRefresh = false;
    {
        std::scoped_lock Lock(State.Mutex);

        if (AssetBlobEvent.ChangeType == csp::common::EAssetChangeType::Deleted)
        {
            State.Sources.erase(AssetBlobEvent.AssetId.c_str());
            State.GlobalLive.erase(AssetBlobEvent.AssetId.c_str());

            for (auto It = State.Bindings.begin(); It != State.Bindings.end();)
            {
                if (It->second.MaterialId == AssetBlobEvent.AssetId)
                {
                    It = State.Bindings.erase(It);
                }
                else
                {
                    ++It;
                }
            }

            return;
        }

        auto SourceIt = State.Sources.find(AssetBlobEvent.AssetId.c_str());
        if (SourceIt != State.Sources.end())
        {
            SourceIt->second.Source.reset();
        }

        bShouldRefresh = !State.ActiveSpaceId.IsEmpty() && csp::CSPFoundation::GetIsInitialised();
    }

    if (bShouldRefresh)
    {
        RefreshAll();
    }
}

void RuntimeMaterialSystem::RefreshAll()
{
    auto& State = *ImplData;
    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    if (AssetSystem == nullptr)
    {
        return;
    }

    const uint64_t Generation = State.Generation;
    const csp::common::String SpaceId = State.ActiveSpaceId;
    State.bLoadingAll = true;

    AssetSystem->GetMaterials(SpaceId, [this, Generation](const csp::systems::MaterialsResult& Result)
        {
            auto& LocalState = *ImplData;
            RuntimeMaterialChangedCallbackHandler Callback;
            std::vector<PendingChangedNotification> Notifications;

            {
                std::scoped_lock Lock(LocalState.Mutex);
                if (LocalState.Generation != Generation)
                {
                    return;
                }

                LocalState.bLoadingAll = false;
                if (Result.GetResultCode() != csp::systems::EResultCode::Success || Result.GetMaterials() == nullptr)
                {
                    return;
                }

                Callback = LocalState.ChangedCallback;

                const auto* Materials = Result.GetMaterials();
                for (size_t Index = 0; Index < Materials->Size(); ++Index)
                {
                    const auto* Material = (*Materials)[Index];
                    if (Material == nullptr)
                    {
                        continue;
                    }

                    auto Clone = CloneMaterial(*Material);
                    if (!Clone)
                    {
                        continue;
                    }

                    auto& Entry = LocalState.Sources[Material->GetMaterialId().c_str()];
                    Entry.Source = std::move(Clone);

                    auto GlobalIt = LocalState.GlobalLive.find(Material->GetMaterialId().c_str());
                    if (GlobalIt != LocalState.GlobalLive.end())
                    {
                        GlobalIt->second = CloneMaterial(*Entry.Source);
                        if (GlobalIt->second)
                        {
                            Notifications.push_back({ Material->GetMaterialId(), "", 0, -1, -1 });
                        }
                    }

                    for (auto& [Handle, Binding] : LocalState.Bindings)
                    {
                        if (Binding.MaterialId == Material->GetMaterialId())
                        {
                            Binding.Live = CloneMaterial(*Entry.Source);
                            if (Binding.Live)
                            {
                                Notifications.push_back(
                                    { Binding.MaterialId, Binding.MaterialPath, Binding.EntityId, Binding.ComponentType, Binding.ComponentIndex });
                            }
                        }
                    }
                }
            }

            if (!Callback)
            {
                return;
            }

            for (const auto& Notification : Notifications)
            {
                Callback(Notification.MaterialId, Notification.EntityId, Notification.ComponentType, Notification.ComponentIndex, Notification.MaterialPath);
            }
        });
}

Material* RuntimeMaterialSystem::Get(const csp::common::String& MaterialId)
{
    return Resolve(MaterialId).MaterialRef;
}

Material* RuntimeMaterialSystem::GetForBinding(uint64_t EntityId, csp::multiplayer::ComponentType ComponentType, int32_t ComponentIndex,
    const csp::common::String& MaterialPath, const csp::common::String& MaterialId)
{
    return ResolveForBinding(EntityId, ComponentType, ComponentIndex, MaterialPath, MaterialId).MaterialRef;
}

void RuntimeMaterialSystem::SetChangedCallback(RuntimeMaterialChangedCallbackHandler Callback)
{
    auto& State = *ImplData;
    std::scoped_lock Lock(State.Mutex);
    State.ChangedCallback = std::move(Callback);
}

RuntimeMaterialState RuntimeMaterialSystem::Resolve(const csp::common::String& MaterialId)
{
    auto& State = *ImplData;
    std::scoped_lock Lock(State.Mutex);

    if (MaterialId.IsEmpty())
    {
        return MakeState(RuntimeMaterialStatus::Missing, "", "", "", 0, 0, 0, nullptr);
    }

    auto GlobalIt = State.GlobalLive.find(MaterialId.c_str());
    if (GlobalIt != State.GlobalLive.end() && GlobalIt->second)
    {
        return MakeState(RuntimeMaterialStatus::Resolved, MaterialId, MaterialId, "", 0, 0, 0, GlobalIt->second.get());
    }

    auto SourceIt = State.Sources.find(MaterialId.c_str());
    if (SourceIt != State.Sources.end() && SourceIt->second.Source)
    {
        State.GlobalLive[MaterialId.c_str()] = CloneMaterial(*SourceIt->second.Source);
        return MakeState(RuntimeMaterialStatus::Resolved, MaterialId, MaterialId, "", 0, 0, 0, State.GlobalLive[MaterialId.c_str()].get());
    }

    if (!State.bLoadingAll && !State.ActiveSpaceId.IsEmpty() && csp::CSPFoundation::GetIsInitialised())
    {
        RefreshAll();
    }

    return MakeState(RuntimeMaterialStatus::Loading, MaterialId, MaterialId, "", 0, 0, 0, nullptr);
}

RuntimeMaterialState RuntimeMaterialSystem::ResolveForBinding(uint64_t EntityId, csp::multiplayer::ComponentType ComponentType, int32_t ComponentIndex,
    const csp::common::String& MaterialPath, const csp::common::String& MaterialId)
{
    auto& State = *ImplData;
    std::scoped_lock Lock(State.Mutex);

    if (MaterialId.IsEmpty() || MaterialPath.IsEmpty())
    {
        return MakeState(RuntimeMaterialStatus::Missing, "", MaterialId, MaterialPath, EntityId, static_cast<int32_t>(ComponentType), ComponentIndex, nullptr);
    }

    const csp::common::String Handle = MakeBindingHandle(EntityId, static_cast<int32_t>(ComponentType), ComponentIndex, MaterialPath).c_str();
    auto BindingIt = State.Bindings.find(Handle.c_str());
    if (BindingIt != State.Bindings.end()
        && BindingIt->second.MaterialId == MaterialId
        && BindingIt->second.MaterialPath == MaterialPath
        && BindingIt->second.Live)
    {
        return MakeState(RuntimeMaterialStatus::Resolved, BindingIt->second.Handle, BindingIt->second.MaterialId, BindingIt->second.MaterialPath,
            BindingIt->second.EntityId, BindingIt->second.ComponentType, BindingIt->second.ComponentIndex, BindingIt->second.Live.get());
    }

    if (BindingIt != State.Bindings.end()
        && (BindingIt->second.MaterialId != MaterialId || BindingIt->second.MaterialPath != MaterialPath))
    {
        BindingIt->second.MaterialId = MaterialId;
        BindingIt->second.MaterialPath = MaterialPath;
        BindingIt->second.EntityId = EntityId;
        BindingIt->second.ComponentType = static_cast<int32_t>(ComponentType);
        BindingIt->second.ComponentIndex = ComponentIndex;
        BindingIt->second.Live.reset();
    }

    auto SourceIt = State.Sources.find(MaterialId.c_str());
    if (SourceIt != State.Sources.end() && SourceIt->second.Source)
    {
        BindingEntry Entry;
        Entry.Handle = Handle;
        Entry.MaterialId = MaterialId;
        Entry.MaterialPath = MaterialPath;
        Entry.EntityId = EntityId;
        Entry.ComponentType = static_cast<int32_t>(ComponentType);
        Entry.ComponentIndex = ComponentIndex;

        auto GlobalIt = State.GlobalLive.find(MaterialId.c_str());
        Entry.Live = (GlobalIt != State.GlobalLive.end() && GlobalIt->second) ? CloneMaterial(*GlobalIt->second) : CloneMaterial(*SourceIt->second.Source);

        State.Bindings[Handle.c_str()] = std::move(Entry);
        const auto& Stored = State.Bindings[Handle.c_str()];
        return MakeState(RuntimeMaterialStatus::Resolved, Stored.Handle, Stored.MaterialId, Stored.MaterialPath, Stored.EntityId, Stored.ComponentType,
            Stored.ComponentIndex, Stored.Live.get());
    }

    if (BindingIt == State.Bindings.end())
    {
        BindingEntry Entry;
        Entry.Handle = Handle;
        Entry.MaterialId = MaterialId;
        Entry.MaterialPath = MaterialPath;
        Entry.EntityId = EntityId;
        Entry.ComponentType = static_cast<int32_t>(ComponentType);
        Entry.ComponentIndex = ComponentIndex;
        State.Bindings[Handle.c_str()] = std::move(Entry);
    }

    if (!State.bLoadingAll && !State.ActiveSpaceId.IsEmpty() && csp::CSPFoundation::GetIsInitialised())
    {
        RefreshAll();
    }

    return MakeState(RuntimeMaterialStatus::Loading, Handle, MaterialId, MaterialPath, EntityId, static_cast<int32_t>(ComponentType), ComponentIndex, nullptr);
}

bool RuntimeMaterialSystem::PatchHandle(const csp::common::String& Handle, const RuntimeMaterialPatch& Patch)
{
    if (Handle.IsEmpty())
    {
        return false;
    }

    auto& State = *ImplData;
    RuntimeMaterialChangedCallbackHandler Callback;
    csp::common::String MaterialId;
    csp::common::String MaterialPath;
    uint64_t EntityId = 0;
    int32_t ComponentType = -1;
    int32_t ComponentIndex = -1;
    bool bPatched = false;

    {
        std::scoped_lock Lock(State.Mutex);

        if (auto BindingIt = State.Bindings.find(Handle.c_str()); BindingIt != State.Bindings.end() && BindingIt->second.Live)
        {
            bPatched = ApplyPatch(*BindingIt->second.Live, Patch);
            if (bPatched)
            {
                Callback = State.ChangedCallback;
                MaterialId = BindingIt->second.MaterialId;
                MaterialPath = BindingIt->second.MaterialPath;
                EntityId = BindingIt->second.EntityId;
                ComponentType = BindingIt->second.ComponentType;
                ComponentIndex = BindingIt->second.ComponentIndex;
            }
        }
        else
        {
            auto GlobalIt = State.GlobalLive.find(Handle.c_str());
            if (GlobalIt == State.GlobalLive.end() || !GlobalIt->second)
            {
                return false;
            }

            bPatched = ApplyPatch(*GlobalIt->second, Patch);
            if (bPatched)
            {
                Callback = State.ChangedCallback;
                MaterialId = Handle;
            }
        }
    }

    if (bPatched && Callback)
    {
        Callback(MaterialId, EntityId, ComponentType, ComponentIndex, MaterialPath);
    }

    return bPatched;
}

bool RuntimeMaterialSystem::ResetHandle(const csp::common::String& Handle)
{
    auto& State = *ImplData;
    RuntimeMaterialChangedCallbackHandler Callback;
    csp::common::String MaterialId;
    csp::common::String MaterialPath;
    uint64_t EntityId = 0;
    int32_t ComponentType = -1;
    int32_t ComponentIndex = -1;
    bool bReset = false;

    {
        std::scoped_lock Lock(State.Mutex);

        if (auto BindingIt = State.Bindings.find(Handle.c_str()); BindingIt != State.Bindings.end())
        {
            auto SourceIt = State.Sources.find(BindingIt->second.MaterialId.c_str());
            if (SourceIt == State.Sources.end() || !SourceIt->second.Source)
            {
                return false;
            }

            BindingIt->second.Live = CloneMaterial(*SourceIt->second.Source);
            bReset = BindingIt->second.Live != nullptr;
            if (bReset)
            {
                Callback = State.ChangedCallback;
                MaterialId = BindingIt->second.MaterialId;
                MaterialPath = BindingIt->second.MaterialPath;
                EntityId = BindingIt->second.EntityId;
                ComponentType = BindingIt->second.ComponentType;
                ComponentIndex = BindingIt->second.ComponentIndex;
            }
        }
        else
        {
            auto SourceIt = State.Sources.find(Handle.c_str());
            if (SourceIt == State.Sources.end() || !SourceIt->second.Source)
            {
                return false;
            }

            State.GlobalLive[Handle.c_str()] = CloneMaterial(*SourceIt->second.Source);
            bReset = State.GlobalLive[Handle.c_str()] != nullptr;
            if (bReset)
            {
                Callback = State.ChangedCallback;
                MaterialId = Handle;
            }
        }
    }

    if (bReset && Callback)
    {
        Callback(MaterialId, EntityId, ComponentType, ComponentIndex, MaterialPath);
    }

    return bReset;
}

bool RuntimeMaterialSystem::SaveHandle(const csp::common::String& Handle)
{
    auto& State = *ImplData;
    std::scoped_lock Lock(State.Mutex);

    Material* MaterialToSave = nullptr;
    csp::common::String MaterialId;

    if (auto BindingIt = State.Bindings.find(Handle.c_str()); BindingIt != State.Bindings.end())
    {
        MaterialToSave = BindingIt->second.Live.get();
        MaterialId = BindingIt->second.MaterialId;
    }
    else if (auto GlobalIt = State.GlobalLive.find(Handle.c_str()); GlobalIt != State.GlobalLive.end())
    {
        MaterialToSave = GlobalIt->second.get();
        MaterialId = Handle;
    }

    if (MaterialToSave == nullptr || MaterialId.IsEmpty())
    {
        return false;
    }

    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    if (AssetSystem == nullptr)
    {
        return false;
    }

    AssetSystem->UpdateMaterial(*MaterialToSave, nullptr);
    auto SourceIt = State.Sources.find(MaterialId.c_str());
    if (SourceIt != State.Sources.end())
    {
        SourceIt->second.Source = CloneMaterial(*MaterialToSave);
    }

    return true;
}

#ifdef CSP_TESTS
void RuntimeMaterialSystem::SetGLTFForTesting(
    const csp::common::String& MaterialId, const csp::common::String&, const csp::systems::GLTFMaterial& Material)
{
    auto& State = *ImplData;
    std::scoped_lock Lock(State.Mutex);
    State.Sources[MaterialId.c_str()].Source = std::make_unique<csp::systems::GLTFMaterial>(Material);
}

void RuntimeMaterialSystem::SetAlphaVideoForTesting(
    const csp::common::String& MaterialId, const csp::common::String&, const csp::systems::AlphaVideoMaterial& Material)
{
    auto& State = *ImplData;
    std::scoped_lock Lock(State.Mutex);
    State.Sources[MaterialId.c_str()].Source = std::make_unique<csp::systems::AlphaVideoMaterial>(Material);
}
#endif

} // namespace csp::systems
