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

#include "Multiplayer/NgxScript/NgxScriptSystem.h"

#include "CSP/CSPFoundation.h"
#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Common/NetworkEventData.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/NetworkEventBus.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Events/EventId.h"
#include "Events/EventListener.h"
#include "Events/EventSystem.h"
#include "Multiplayer/NgxScript/signals.h"

#include "quickjs.h"
#include "quickjspp.hpp"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <cmath>
#include <fmt/format.h>
#include <set>
#include <optional>
#include <vector>

namespace
{

constexpr const char* PREACT_SIGNALS_CORE_MODULE = "@preact/signals-core";
constexpr const char* CODECOMPONENT_JSON_RESULT_SLOT = "__cspCodeComponentJsonResult";
constexpr const char* CODECOMPONENT_BOOL_RESULT_SLOT = "__cspCodeComponentBoolResult";
constexpr const char* EMPTY_JSON_OBJECT_STRING = "{}";
constexpr const char* ASSET_BLOB_CHANGED_RECEIVER_ID = "CSPInternal::NgxScriptSystem";

const char* RealtimeEngineTypeToString(csp::common::IRealtimeEngine* RealtimeEngine)
{
    if (RealtimeEngine == nullptr)
    {
        return "Null";
    }

    switch (RealtimeEngine->GetRealtimeEngineType())
    {
    case csp::common::RealtimeEngineType::Online:
        return "Online";
    case csp::common::RealtimeEngineType::Offline:
        return "Offline";
    default:
        return "Unknown";
    }
}

std::string EscapeJSStringLiteral(const std::string& Input)
{
    std::string Escaped;
    Escaped.reserve(Input.size());

    for (const char Character : Input)
    {
        switch (Character)
        {
        case '\\':
            Escaped += "\\\\";
            break;
        case '\'':
            Escaped += "\\'";
            break;
        case '"':
            Escaped += "\\\"";
            break;
        case '\n':
            Escaped += "\\n";
            break;
        case '\r':
            Escaped += "\\r";
            break;
        case '\t':
            Escaped += "\\t";
            break;
        default:
            Escaped += Character;
            break;
        }
    }

    return Escaped;
}

std::string JoinArgs(const qjs::rest<std::string>& Args)
{
    std::string Output;

    for (size_t Index = 0; Index < Args.size(); ++Index)
    {
        Output += Args[Index];
        if ((Index + 1) < Args.size())
        {
            Output += " ";
        }
    }

    return Output;
}

struct EntityUpdateLockGuard
{
    explicit EntityUpdateLockGuard(csp::common::IRealtimeEngine* InEngine)
        : Engine(InEngine)
        , bLocked(false)
    {
        if (Engine != nullptr)
        {
            Engine->LockEntityUpdate();
            bLocked = true;
        }
    }

    ~EntityUpdateLockGuard()
    {
        if (bLocked && (Engine != nullptr))
        {
            Engine->UnlockEntityUpdate();
        }
    }

    csp::common::IRealtimeEngine* Engine;
    bool bLocked;
};

std::optional<uint64_t> TryParseEntityId(const std::string& EntityIdText)
{
    if (EntityIdText.empty())
    {
        return std::nullopt;
    }

    try
    {
        return std::stoull(EntityIdText);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::vector<csp::multiplayer::SpaceEntity*> CollectEntities(csp::common::IRealtimeEngine* RealtimeEngine)
{
    std::vector<csp::multiplayer::SpaceEntity*> Entities;
    if (RealtimeEngine == nullptr)
    {
        return Entities;
    }

    const size_t EntityCount = RealtimeEngine->GetNumEntities();
    Entities.reserve(EntityCount);
    for (size_t Index = 0; Index < EntityCount; ++Index)
    {
        auto* Entity = RealtimeEngine->GetEntityByIndex(Index);
        if (Entity != nullptr)
        {
            Entities.push_back(Entity);
        }
    }

    return Entities;
}

csp::multiplayer::SpaceEntity* FindEntityById(const std::vector<csp::multiplayer::SpaceEntity*>& Entities, uint64_t EntityId)
{
    for (auto* Entity : Entities)
    {
        if ((Entity != nullptr) && (Entity->GetId() == EntityId))
        {
            return Entity;
        }
    }

    return nullptr;
}

std::set<uint64_t> BuildAllEntityIdSet(const std::vector<csp::multiplayer::SpaceEntity*>& Entities)
{
    std::set<uint64_t> EntityIds;
    for (const auto* Entity : Entities)
    {
        if (Entity != nullptr)
        {
            EntityIds.insert(Entity->GetId());
        }
    }

    return EntityIds;
}

std::set<uint64_t> EvaluateQueryToEntityIds(const rapidjson::Value& Query, const std::vector<csp::multiplayer::SpaceEntity*>& Entities)
{
    if (!Query.IsObject())
    {
        return {};
    }

    if (!Query.HasMember("kind") || !Query["kind"].IsString())
    {
        return {};
    }

    const std::string Kind = Query["kind"].GetString();
    if (Kind == "id")
    {
        if (!Query.HasMember("id"))
        {
            return {};
        }

        uint64_t RequestedId = 0;
        if (Query["id"].IsUint64())
        {
            RequestedId = Query["id"].GetUint64();
        }
        else if (Query["id"].IsInt64() && (Query["id"].GetInt64() > 0))
        {
            RequestedId = static_cast<uint64_t>(Query["id"].GetInt64());
        }
        else if (Query["id"].IsString())
        {
            const auto ParsedId = TryParseEntityId(Query["id"].GetString());
            if (!ParsedId.has_value())
            {
                return {};
            }

            RequestedId = *ParsedId;
        }
        else
        {
            return {};
        }

        return (FindEntityById(Entities, RequestedId) != nullptr) ? std::set<uint64_t> { RequestedId } : std::set<uint64_t> {};
    }

    if (Kind == "name")
    {
        if (!Query.HasMember("name") || !Query["name"].IsString())
        {
            return {};
        }

        const std::string RequestedName = Query["name"].GetString();
        std::set<uint64_t> MatchingIds;
        for (const auto* Entity : Entities)
        {
            if ((Entity != nullptr) && (Entity->GetName().c_str() == RequestedName))
            {
                MatchingIds.insert(Entity->GetId());
            }
        }
        return MatchingIds;
    }

    if (Kind == "tag")
    {
        // Tags are not yet modeled on SpaceEntity, so this query is currently unresolved by design.
        return {};
    }

    if (Kind == "componentType")
    {
        if (!Query.HasMember("componentType") || !Query["componentType"].IsInt())
        {
            return {};
        }

        const int32_t RawComponentType = Query["componentType"].GetInt();
        if (RawComponentType <= 0)
        {
            return {};
        }

        const auto ComponentType = static_cast<csp::multiplayer::ComponentType>(RawComponentType);
        std::set<uint64_t> MatchingIds;
        for (auto* Entity : Entities)
        {
            if ((Entity != nullptr) && (Entity->FindFirstComponentOfType(ComponentType) != nullptr))
            {
                MatchingIds.insert(Entity->GetId());
            }
        }
        return MatchingIds;
    }

    if ((Kind == "and") || (Kind == "or"))
    {
        std::vector<const rapidjson::Value*> OperandValues;
        if (Query.HasMember("operands"))
        {
            const auto& Operands = Query["operands"];
            if (Operands.IsArray())
            {
                for (const auto& Operand : Operands.GetArray())
                {
                    OperandValues.push_back(&Operand);
                }
            }
            else if (Operands.IsObject())
            {
                for (const auto& Member : Operands.GetObject())
                {
                    OperandValues.push_back(&Member.value);
                }
            }
        }

        if (OperandValues.empty())
        {
            return {};
        }

        if (Kind == "and")
        {
            std::set<uint64_t> Intersection = EvaluateQueryToEntityIds(*OperandValues.front(), Entities);
            for (size_t OperandIndex = 1; OperandIndex < OperandValues.size(); ++OperandIndex)
            {
                const std::set<uint64_t> Next = EvaluateQueryToEntityIds(*OperandValues[OperandIndex], Entities);
                std::set<uint64_t> Result;
                std::set_intersection(
                    Intersection.begin(), Intersection.end(), Next.begin(), Next.end(), std::inserter(Result, Result.begin()));
                Intersection = std::move(Result);
                if (Intersection.empty())
                {
                    break;
                }
            }

            return Intersection;
        }

        std::set<uint64_t> Union;
        for (const auto* Operand : OperandValues)
        {
            const auto Next = EvaluateQueryToEntityIds(*Operand, Entities);
            Union.insert(Next.begin(), Next.end());
        }
        return Union;
    }

    if (Kind == "not")
    {
        if (!Query.HasMember("operand"))
        {
            return {};
        }

        const std::set<uint64_t> AllEntityIds = BuildAllEntityIdSet(Entities);
        const std::set<uint64_t> ExcludedIds = EvaluateQueryToEntityIds(Query["operand"], Entities);
        std::set<uint64_t> Result;
        std::set_difference(AllEntityIds.begin(), AllEntityIds.end(), ExcludedIds.begin(), ExcludedIds.end(),
            std::inserter(Result, Result.begin()));
        return Result;
    }

    return {};
}

std::optional<uint64_t> ResolveEntityIdFromQueryJson(
    const std::string& QueryJson, const std::vector<csp::multiplayer::SpaceEntity*>& Entities)
{
    rapidjson::Document QueryDocument;
    if (QueryDocument.Parse(QueryJson.c_str()).HasParseError() || !QueryDocument.IsObject())
    {
        return std::nullopt;
    }

    const std::set<uint64_t> MatchingIds = EvaluateQueryToEntityIds(QueryDocument, Entities);
    if (MatchingIds.empty())
    {
        return std::nullopt;
    }

    return *MatchingIds.begin();
}

std::string BuildEntitySnapshotJson(const csp::multiplayer::SpaceEntity& Entity)
{
    rapidjson::StringBuffer Buffer;
    rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);

    Writer.StartObject();
    Writer.Key("id");
    Writer.Uint64(Entity.GetId());
    Writer.Key("name");
    Writer.String(Entity.GetName().c_str());
    Writer.Key("thirdPartyRef");
    Writer.String(Entity.GetThirdPartyRef().c_str());

    Writer.Key("position");
    Writer.StartArray();
    const auto Position = Entity.GetPosition();
    Writer.Double(static_cast<double>(Position.X));
    Writer.Double(static_cast<double>(Position.Y));
    Writer.Double(static_cast<double>(Position.Z));
    Writer.EndArray();

    Writer.Key("rotation");
    Writer.StartArray();
    const auto Rotation = Entity.GetRotation();
    Writer.Double(static_cast<double>(Rotation.X));
    Writer.Double(static_cast<double>(Rotation.Y));
    Writer.Double(static_cast<double>(Rotation.Z));
    Writer.Double(static_cast<double>(Rotation.W));
    Writer.EndArray();

    Writer.Key("scale");
    Writer.StartArray();
    const auto Scale = Entity.GetScale();
    Writer.Double(static_cast<double>(Scale.X));
    Writer.Double(static_cast<double>(Scale.Y));
    Writer.Double(static_cast<double>(Scale.Z));
    Writer.EndArray();

    Writer.EndObject();
    return Buffer.GetString();
}

std::string BuildComponentSnapshotJson(const csp::multiplayer::ComponentBase& Component)
{
    rapidjson::StringBuffer Buffer;
    rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);

    Writer.StartObject();
    Writer.Key("componentId");
    Writer.Uint(Component.GetId());
    Writer.Key("componentType");
    Writer.Int(static_cast<int32_t>(Component.GetComponentType()));
    Writer.Key("componentName");
    Writer.String(Component.GetComponentName().c_str());
    Writer.EndObject();

    return Buffer.GetString();
}

csp::multiplayer::ComponentBase* FindComponentByTypeAndIndex(
    csp::multiplayer::SpaceEntity& Entity, csp::multiplayer::ComponentType ComponentType, int32_t Index)
{
    if (Index < 0)
    {
        return nullptr;
    }

    const auto* Components = Entity.GetComponents();
    if (Components == nullptr)
    {
        return nullptr;
    }

    int32_t CurrentIndex = 0;
    for (const auto& ComponentPair : *Components)
    {
        auto* Component = ComponentPair.second;
        if ((Component != nullptr) && (Component->GetComponentType() == ComponentType))
        {
            if (CurrentIndex == Index)
            {
                return Component;
            }

            ++CurrentIndex;
        }
    }

    return nullptr;
}

} // namespace

namespace csp::systems::ngxscript
{

csp::common::String StripRandomPartFromScriptPath(const csp::common::String& ScriptPath)
{
    constexpr const char* RANDOM_SUFFIX_SEPARATOR = "___";
    const std::string ScriptPathStd = ScriptPath.c_str();
    const size_t FoundIndex = ScriptPathStd.find(RANDOM_SUFFIX_SEPARATOR);

    if (FoundIndex != std::string::npos)
    {
        return csp::common::String(ScriptPathStd.substr(0, FoundIndex).c_str());
    }

    return ScriptPath;
}

csp::common::String BuildCanonicalAssetPath(
    const csp::systems::Asset& Asset, const csp::common::Map<csp::common::String, csp::systems::AssetCollection>& AssetCollectionMap)
{
    csp::common::String Path = StripRandomPartFromScriptPath(Asset.Name);
    csp::common::String CurrentCollectionId = Asset.AssetCollectionId;

    while (!CurrentCollectionId.IsEmpty())
    {
        if (!AssetCollectionMap.HasKey(CurrentCollectionId))
        {
            break;
        }

        const auto& CurrentCollection = AssetCollectionMap[CurrentCollectionId];
        Path = StripRandomPartFromScriptPath(CurrentCollection.Name) + "/" + Path;
        CurrentCollectionId = CurrentCollection.ParentId;
    }

    return "/" + Path;
}

} // namespace csp::systems::ngxscript

namespace csp::systems
{

class NgxScriptSystem::NgxScriptTickEventHandler final : public csp::events::EventListener
{
public:
    explicit NgxScriptTickEventHandler(NgxScriptSystem* InSystem)
        : System(InSystem)
    {
    }

    void OnEvent(const csp::events::Event& InEvent) override
    {
        if (InEvent.GetId() == csp::events::FOUNDATION_TICK_EVENT_ID)
        {
            System->DrainPendingJobs();
        }
    }

private:
    NgxScriptSystem* System;
};

NgxScriptSystem::NgxScriptSystem(csp::common::LogSystem& InLogSystem)
    : LogSystem(InLogSystem)
    , ActiveRealtimeEngine(nullptr)
    , ActiveSpaceId("")
    , Runtime(std::make_unique<qjs::Runtime>())
    , Context(nullptr)
    , SessionGeneration(0)
    , ContextGeneration(0)
    , ScriptModulesLoaded(false)
    , bAssetDetailBlobChangedListenerRegistered(false)
    , TickEventHandler(std::make_unique<NgxScriptTickEventHandler>(this))
{
    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, TickEventHandler.get());
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: System constructed and tick listener registered.");
}

NgxScriptSystem::~NgxScriptSystem()
{
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: System shutting down.");
    OnExitSpace();
    csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, TickEventHandler.get());
}

void NgxScriptSystem::OnEnterSpace(const csp::common::String& InSpaceId, csp::common::IRealtimeEngine* InRealtimeEngine)
{
    const std::string EnterMessage = fmt::format("NgxScript Trace: OnEnterSpace(spaceId='{}', engineType={}).", InSpaceId.c_str(),
        RealtimeEngineTypeToString(InRealtimeEngine));
    LogSystem.LogMsg(csp::common::LogLevel::Log, EnterMessage.c_str());

    ActiveSpaceId = InSpaceId;
    ActiveRealtimeEngine = InRealtimeEngine;
    SessionGeneration.fetch_add(1);
    ScriptModulesLoaded.store(false);

    {
        std::scoped_lock ModuleLock(ModuleSourcesMutex);
        LoadedModuleSources.clear();
        TrackedScriptAssetCollectionIds.clear();
    }

    RegisterAssetDetailBlobChangedListener();
    RebuildContext();
    LoadScriptModules();
}

void NgxScriptSystem::OnExitSpace()
{
    const std::string ExitMessage = fmt::format("NgxScript Trace: OnExitSpace(spaceId='{}').", ActiveSpaceId.c_str());
    LogSystem.LogMsg(csp::common::LogLevel::Log, ExitMessage.c_str());

    SessionGeneration.fetch_add(1);
    ScriptModulesLoaded.store(false);
    UnregisterAssetDetailBlobChangedListener();
    ActiveSpaceId = "";
    ActiveRealtimeEngine = nullptr;

    {
        std::scoped_lock ModuleLock(ModuleSourcesMutex);
        LoadedModuleSources.clear();
        TrackedScriptAssetCollectionIds.clear();
    }

    TeardownContext();
}

void NgxScriptSystem::RegisterAssetDetailBlobChangedListener()
{
    if (bAssetDetailBlobChangedListenerRegistered)
    {
        return;
    }

    auto* MultiplayerConnection = csp::systems::SystemsManager::Get().GetMultiplayerConnection();
    if (!MultiplayerConnection)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning,
            "NgxScript: Failed to register AssetDetailBlobChanged listener (MultiplayerConnection unavailable).");
        return;
    }

    auto* EventBus = &MultiplayerConnection->GetEventBus();
    const csp::common::String EventName
        = csp::multiplayer::NetworkEventBus::StringFromNetworkEvent(csp::multiplayer::NetworkEventBus::NetworkEvent::AssetDetailBlobChanged);

    EventBus->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(ASSET_BLOB_CHANGED_RECEIVER_ID, EventName),
        [this](const csp::common::NetworkEventData& NetworkEventData) { this->OnAssetDetailBlobChanged(NetworkEventData); });

    bAssetDetailBlobChangedListenerRegistered = true;
}

void NgxScriptSystem::UnregisterAssetDetailBlobChangedListener()
{
    if (!bAssetDetailBlobChangedListenerRegistered)
    {
        return;
    }

    if (auto* MultiplayerConnection = csp::systems::SystemsManager::Get().GetMultiplayerConnection())
    {
        auto* EventBus = &MultiplayerConnection->GetEventBus();
        const csp::common::String EventName = csp::multiplayer::NetworkEventBus::StringFromNetworkEvent(
            csp::multiplayer::NetworkEventBus::NetworkEvent::AssetDetailBlobChanged);
        EventBus->StopListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(ASSET_BLOB_CHANGED_RECEIVER_ID, EventName));
    }

    bAssetDetailBlobChangedListenerRegistered = false;
}

bool NgxScriptSystem::IsTrackedScriptAssetCollection(const csp::common::String& AssetCollectionId) const
{
    if (AssetCollectionId.IsEmpty())
    {
        return false;
    }

    std::scoped_lock ModuleLock(ModuleSourcesMutex);
    return TrackedScriptAssetCollectionIds.find(AssetCollectionId.c_str()) != TrackedScriptAssetCollectionIds.end();
}

void NgxScriptSystem::OnAssetDetailBlobChanged(const csp::common::NetworkEventData& NetworkEventData)
{
    if (ActiveSpaceId.IsEmpty())
    {
        return;
    }

    const auto& AssetBlobEvent = static_cast<const csp::common::AssetDetailBlobChangedNetworkEventData&>(NetworkEventData);
    if (AssetBlobEvent.AssetType != csp::systems::EAssetType::SCRIPT_LIBRARY)
    {
        return;
    }

    if (!IsTrackedScriptAssetCollection(AssetBlobEvent.AssetCollectionId))
    {
        return;
    }

    if ((AssetBlobEvent.ChangeType == csp::common::EAssetChangeType::Created)
        || (AssetBlobEvent.ChangeType == csp::common::EAssetChangeType::Updated))
    {
        ReloadScriptModule(AssetBlobEvent.AssetCollectionId, AssetBlobEvent.AssetId);
        return;
    }

    if (AssetBlobEvent.ChangeType == csp::common::EAssetChangeType::Deleted)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Log,
            "NgxScript Trace: Script library asset deleted; rebuilding context and reloading module map.");
        ScriptModulesLoaded.store(false);
        RebuildContext();
        LoadScriptModules();
    }
}

void NgxScriptSystem::RegisterStaticModuleSource(const csp::common::String& ModulePath, const csp::common::String& ModuleSource)
{
    if (ModulePath.IsEmpty())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: Cannot register static module source with empty module path.");
        return;
    }

    {
        std::scoped_lock ModuleLock(ModuleSourcesMutex);
        StaticModuleSources[ModulePath.c_str()] = ModuleSource.c_str();
    }

    const std::string RegisterMessage = fmt::format("NgxScript Trace: Registered static module source '{}'.", ModulePath.c_str());
    LogSystem.LogMsg(csp::common::LogLevel::Log, RegisterMessage.c_str());
}

void NgxScriptSystem::UnregisterStaticModuleSource(const csp::common::String& ModulePath)
{
    if (ModulePath.IsEmpty())
    {
        return;
    }

    {
        std::scoped_lock ModuleLock(ModuleSourcesMutex);
        StaticModuleSources.erase(ModulePath.c_str());
    }

    const std::string UnregisterMessage = fmt::format("NgxScript Trace: Unregistered static module source '{}'.", ModulePath.c_str());
    LogSystem.LogMsg(csp::common::LogLevel::Log, UnregisterMessage.c_str());
}

void NgxScriptSystem::ReloadScriptModule(const csp::common::String& AssetCollectionId, const csp::common::String& AssetId)
{
    const std::string ReloadRequestMessage = fmt::format(
        "NgxScript Trace: ReloadScriptModule(assetCollectionId='{}', assetId='{}').", AssetCollectionId.c_str(), AssetId.c_str());
    LogSystem.LogMsg(csp::common::LogLevel::Log, ReloadRequestMessage.c_str());

    if (AssetCollectionId.IsEmpty() || AssetId.IsEmpty())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: Cannot reload script with empty asset identifiers.");
        return;
    }

    if (ActiveSpaceId.IsEmpty())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: Cannot reload script while no space is active.");
        return;
    }

    if (!csp::CSPFoundation::GetIsInitialised())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: CSPFoundation is not initialised. Skipping module reload.");
        return;
    }

    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    if (!AssetSystem)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxScript: AssetSystem unavailable for script reload.");
        return;
    }

    const uint64_t Generation = SessionGeneration.load();
    FetchAssetCollectionMapForSpace(Generation,
        [this, Generation, AssetCollectionId, AssetId, AssetSystem](std::shared_ptr<AssetCollectionMap> AssetCollections)
        {
            if (!IsGenerationCurrent(Generation))
            {
                return;
            }

            AssetSystem->GetAssetById(AssetCollectionId, AssetId,
                [this, Generation, AssetCollections, AssetSystem](const csp::systems::AssetResult& AssetResult)
                {
                    if (!IsGenerationCurrent(Generation))
                    {
                        return;
                    }

                    if (AssetResult.GetResultCode() != csp::systems::EResultCode::Success)
                    {
                        LogSystem.LogMsg(
                            csp::common::LogLevel::Error, "NgxScript: Failed to find script asset to reload by id.");
                        return;
                    }

                    const csp::systems::Asset ScriptAsset = AssetResult.GetAsset();
                    AssetSystem->DownloadAssetData(ScriptAsset,
                        [this, Generation, AssetCollections, ScriptAsset](const csp::systems::AssetDataResult& DownloadResult)
                        {
                            if (!IsGenerationCurrent(Generation))
                            {
                                return;
                            }

                            if (DownloadResult.GetResultCode() == csp::systems::EResultCode::InProgress)
                            {
                                return;
                            }

                            if (DownloadResult.GetResultCode() != csp::systems::EResultCode::Success)
                            {
                                LogSystem.LogMsg(
                                    csp::common::LogLevel::Error, "NgxScript: Failed to download script module for reload.");
                                return;
                            }

                            const char* ModuleSourceData = static_cast<const char*>(DownloadResult.GetData());
                            const size_t ModuleSourceLength = DownloadResult.GetDataLength();
                            const std::string ModuleSource = (ModuleSourceData != nullptr)
                                ? std::string(ModuleSourceData, ModuleSourceLength)
                                : std::string();

                            const csp::common::String CanonicalPath
                                = ngxscript::BuildCanonicalAssetPath(ScriptAsset, *AssetCollections);
                            const std::string ReloadSuccessMessage
                                = fmt::format("NgxScript Trace: Reloaded module '{}'.", CanonicalPath.c_str());
                            LogSystem.LogMsg(csp::common::LogLevel::Log, ReloadSuccessMessage.c_str());

                            {
                                std::scoped_lock ModuleLock(ModuleSourcesMutex);
                                LoadedModuleSources[CanonicalPath.c_str()] = ModuleSource;
                            }

                            ScriptModulesLoaded.store(false);
                            // Rebuild context to clear module cache then refresh module source set for the active space.
                            RebuildContext();
                            LoadScriptModules();
                        });
                });
        });
}

#ifdef CSP_TESTS
bool NgxScriptSystem::HasActiveContextForTesting() const
{
    std::scoped_lock ContextLock(ContextMutex);
    return Context != nullptr;
}

bool NgxScriptSystem::EvaluateModuleScriptForTesting(const csp::common::String& ScriptText)
{
    return EvaluateModuleScript(ScriptText.c_str(), "<ngx-test>");
}

int32_t NgxScriptSystem::GetGlobalIntForTesting(const csp::common::String& Key, int32_t DefaultValue) const
{
    std::scoped_lock ContextLock(ContextMutex);
    if (!Context)
    {
        return DefaultValue;
    }

    try
    {
        const qjs::Value Value = Context->global()[Key.c_str()];
        return Value.as<int32_t>();
    }
    catch (...)
    {
        return DefaultValue;
    }
}

void NgxScriptSystem::SetLoadedModuleSourceForTesting(const csp::common::String& ModulePath, const csp::common::String& ModuleSource)
{
    std::scoped_lock ModuleLock(ModuleSourcesMutex);
    LoadedModuleSources[ModulePath.c_str()] = ModuleSource.c_str();
}

bool NgxScriptSystem::RunModuleForTesting(const csp::common::String& ModulePath)
{
    // Mirror reload flow so test calls observe freshly compiled module state.
    RebuildContext();
    return ExecuteModule(ModulePath);
}
#endif

void NgxScriptSystem::RebuildContext()
{
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: RebuildContext begin.");

    {
        std::scoped_lock ContextLock(ContextMutex);
        Context.reset();

        if (!Runtime)
        {
            Runtime = std::make_unique<qjs::Runtime>();
        }

        Context = std::make_unique<qjs::Context>(*Runtime);
    }

    InstallModuleLoader();
    InstallHostBindings();
    ContextGeneration.fetch_add(1);
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: RebuildContext complete.");
}

void NgxScriptSystem::TeardownContext()
{
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: TeardownContext.");
    std::scoped_lock ContextLock(ContextMutex);
    Context.reset();
}

void NgxScriptSystem::InstallModuleLoader()
{
    std::scoped_lock ContextLock(ContextMutex);
    if (Context)
    {
        Context->moduleLoader = [this](std::string_view ModuleName) -> qjs::Context::ModuleData
        {
            const std::string RequestedModule(ModuleName);

            if (RequestedModule == PREACT_SIGNALS_CORE_MODULE)
            {
                const std::string ResolveMessage
                    = fmt::format("NgxScript Trace: Resolved built-in module '{}'.", RequestedModule);
                LogSystem.LogMsg(csp::common::LogLevel::Verbose, ResolveMessage.c_str());
                return qjs::Context::ModuleData { RequestedModule, csp::systems::SignalsScriptCode.c_str(), std::nullopt };
            }

            {
                std::scoped_lock ModuleLock(ModuleSourcesMutex);
                auto StaticSourceIt = StaticModuleSources.find(RequestedModule);
                if (StaticSourceIt != StaticModuleSources.end())
                {
                    const std::string ResolveMessage
                        = fmt::format("NgxScript Trace: Resolved static module '{}'.", RequestedModule);
                    LogSystem.LogMsg(csp::common::LogLevel::Verbose, ResolveMessage.c_str());
                    return qjs::Context::ModuleData { RequestedModule, StaticSourceIt->second, std::nullopt };
                }

                auto SourceIt = LoadedModuleSources.find(RequestedModule);
                if (SourceIt != LoadedModuleSources.end())
                {
                    const std::string ResolveMessage
                        = fmt::format("NgxScript Trace: Resolved module '{}' from in-memory source map.", RequestedModule);
                    LogSystem.LogMsg(csp::common::LogLevel::Verbose, ResolveMessage.c_str());
                    return qjs::Context::ModuleData { RequestedModule, SourceIt->second, std::nullopt };
                }
            }

            const std::string MissingMessage
                = fmt::format("NgxScript: Missing module '{}' (module source may still be loading).", RequestedModule);
            LogSystem.LogMsg(csp::common::LogLevel::Warning, MissingMessage.c_str());

            const std::string ThrowingModule
                = "throw new Error('NgxScript module not found: " + EscapeJSStringLiteral(RequestedModule) + "');";
            return qjs::Context::ModuleData { RequestedModule, ThrowingModule, std::nullopt };
        };
        LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: Module loader installed.");
    }
}

void NgxScriptSystem::InstallHostBindings()
{
    std::scoped_lock ContextLock(ContextMutex);
    if (!Context)
    {
        return;
    }

    auto& CSPModule = Context->addModule("csp");
    CSPModule.function("__log", [this](qjs::rest<std::string> Args) { LogSystem.LogMsg(csp::common::LogLevel::Log, JoinArgs(Args).c_str()); });
    CSPModule.function(
        "__warn", [this](qjs::rest<std::string> Args) { LogSystem.LogMsg(csp::common::LogLevel::Warning, JoinArgs(Args).c_str()); });
    CSPModule.function(
        "__error", [this](qjs::rest<std::string> Args) { LogSystem.LogMsg(csp::common::LogLevel::Error, JoinArgs(Args).c_str()); });
    CSPModule.function("__resolveEntityQuery", [this](const std::string& QueryJson) -> std::string
    {
        if (ActiveRealtimeEngine == nullptr)
        {
            return "";
        }

        EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
        const auto Entities = CollectEntities(ActiveRealtimeEngine);
        const auto ResolvedEntityId = ResolveEntityIdFromQueryJson(QueryJson, Entities);
        return ResolvedEntityId.has_value() ? std::to_string(*ResolvedEntityId) : std::string();
    });
    CSPModule.function("__getEntitySnapshot", [this](const std::string& EntityIdText) -> std::string
    {
        if (ActiveRealtimeEngine == nullptr)
        {
            return "";
        }

        const auto EntityId = TryParseEntityId(EntityIdText);
        if (!EntityId.has_value())
        {
            return "";
        }

        EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
        const auto Entities = CollectEntities(ActiveRealtimeEngine);
        auto* Entity = FindEntityById(Entities, *EntityId);
        if (Entity == nullptr)
        {
            return "";
        }

        return BuildEntitySnapshotJson(*Entity);
    });
    CSPModule.function("__getComponentSnapshot", [this](const std::string& EntityIdText, int32_t ComponentTypeValue, int32_t ComponentIndex) -> std::string
    {
        if (ActiveRealtimeEngine == nullptr)
        {
            return "";
        }

        const auto EntityId = TryParseEntityId(EntityIdText);
        if (!EntityId.has_value())
        {
            return "";
        }

        if (ComponentTypeValue <= 0)
        {
            return "";
        }

        EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
        const auto Entities = CollectEntities(ActiveRealtimeEngine);
        auto* Entity = FindEntityById(Entities, *EntityId);
        if (Entity == nullptr)
        {
            return "";
        }

        auto* Component = FindComponentByTypeAndIndex(
            *Entity, static_cast<csp::multiplayer::ComponentType>(ComponentTypeValue), ComponentIndex);
        if (Component == nullptr)
        {
            return "";
        }

        return BuildComponentSnapshotJson(*Component);
    });
    CSPModule.function("__setEntityPosition", [this](const std::string& EntityIdText, double X, double Y, double Z) -> bool
    {
        if (ActiveRealtimeEngine == nullptr)
        {
            return false;
        }

        const auto EntityId = TryParseEntityId(EntityIdText);
        if (!EntityId.has_value())
        {
            return false;
        }

        EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
        const auto Entities = CollectEntities(ActiveRealtimeEngine);
        auto* Entity = FindEntityById(Entities, *EntityId);
        if (Entity == nullptr)
        {
            return false;
        }

        return Entity->SetPosition(csp::common::Vector3 { static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z) });
    });
    CSPModule.function("__setEntityRotation", [this](const std::string& EntityIdText, double X, double Y, double Z, double W) -> bool
    {
        if (ActiveRealtimeEngine == nullptr)
        {
            return false;
        }

        const auto EntityId = TryParseEntityId(EntityIdText);
        if (!EntityId.has_value())
        {
            return false;
        }

        EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
        const auto Entities = CollectEntities(ActiveRealtimeEngine);
        auto* Entity = FindEntityById(Entities, *EntityId);
        if (Entity == nullptr)
        {
            return false;
        }

        return Entity->SetRotation(
            csp::common::Vector4 { static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z), static_cast<float>(W) });
    });
    CSPModule.function("__setEntityScale", [this](const std::string& EntityIdText, double X, double Y, double Z) -> bool
    {
        if (ActiveRealtimeEngine == nullptr)
        {
            return false;
        }

        const auto EntityId = TryParseEntityId(EntityIdText);
        if (!EntityId.has_value())
        {
            return false;
        }

        EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
        const auto Entities = CollectEntities(ActiveRealtimeEngine);
        auto* Entity = FindEntityById(Entities, *EntityId);
        if (Entity == nullptr)
        {
            return false;
        }

        return Entity->SetScale(csp::common::Vector3 { static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z) });
    });
    CSPModule.function("__setEntityName", [this](const std::string& EntityIdText, const std::string& Name) -> bool
    {
        if (ActiveRealtimeEngine == nullptr)
        {
            return false;
        }

        const auto EntityId = TryParseEntityId(EntityIdText);
        if (!EntityId.has_value())
        {
            return false;
        }

        EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
        const auto Entities = CollectEntities(ActiveRealtimeEngine);
        auto* Entity = FindEntityById(Entities, *EntityId);
        if (Entity == nullptr)
        {
            return false;
        }

        return Entity->SetName(Name.c_str());
    });
    CSPModule.function("__setEntityThirdPartyRef", [this](const std::string& EntityIdText, const std::string& ThirdPartyRef) -> bool
    {
        if (ActiveRealtimeEngine == nullptr)
        {
            return false;
        }

        const auto EntityId = TryParseEntityId(EntityIdText);
        if (!EntityId.has_value())
        {
            return false;
        }

        EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
        const auto Entities = CollectEntities(ActiveRealtimeEngine);
        auto* Entity = FindEntityById(Entities, *EntityId);
        if (Entity == nullptr)
        {
            return false;
        }

        return Entity->SetThirdPartyRef(ThirdPartyRef.c_str());
    });

    static constexpr const char* HOST_BINDINGS_SCRIPT = R"(
import * as csp from "csp";
globalThis.csp = csp;
globalThis.console = {
    log: (...args) => csp.__log(...args.map((value) => String(value))),
    warn: (...args) => csp.__warn(...args.map((value) => String(value))),
    error: (...args) => csp.__error(...args.map((value) => String(value))),
};
)";

    const qjs::Value EvalResult = Context->eval(HOST_BINDINGS_SCRIPT, "<ngx-host-bindings>", JS_EVAL_TYPE_MODULE);
    if (EvalResult.isException())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxScript: Failed to install host bindings.");
        return;
    }

    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: Host bindings installed.");
}

void NgxScriptSystem::DrainPendingJobs()
{
    std::scoped_lock ContextLock(ContextMutex);
    if (!Runtime || !Context)
    {
        return;
    }

    while (Runtime->isJobPending())
    {
        Runtime->executePendingJob();
    }
}

void NgxScriptSystem::LoadScriptModules()
{
    const std::string LoadStartMessage = fmt::format("NgxScript Trace: LoadScriptModules(spaceId='{}').", ActiveSpaceId.c_str());
    LogSystem.LogMsg(csp::common::LogLevel::Log, LoadStartMessage.c_str());

    {
        std::scoped_lock ModuleLock(ModuleSourcesMutex);
        TrackedScriptAssetCollectionIds.clear();
    }

    if (ActiveSpaceId.IsEmpty())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript Trace: LoadScriptModules aborted (no active space).");
        ScriptModulesLoaded.store(true);
        return;
    }

    if (!csp::CSPFoundation::GetIsInitialised())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript Trace: LoadScriptModules aborted (CSPFoundation not initialised).");
        ScriptModulesLoaded.store(true);
        return;
    }

    const uint64_t Generation = SessionGeneration.load();
    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    if (!AssetSystem)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxScript: AssetSystem unavailable, cannot load script modules.");
        ScriptModulesLoaded.store(true);
        return;
    }

    FetchAssetCollectionMapForSpace(Generation,
        [this, Generation, AssetSystem](std::shared_ptr<AssetCollectionMap> AssetCollections)
        {
            if (!IsGenerationCurrent(Generation))
            {
                return;
            }

            std::unique_ptr<const csp::common::Array<csp::common::String>> AssetCollectionIds(AssetCollections->Keys());
            if (!AssetCollectionIds || AssetCollectionIds->IsEmpty())
            {
                LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: No asset collections found while loading scripts.");
                ScriptModulesLoaded.store(true);
                return;
            }

            {
                std::scoped_lock ModuleLock(ModuleSourcesMutex);
                TrackedScriptAssetCollectionIds.clear();
                for (size_t Index = 0; Index < AssetCollectionIds->Size(); ++Index)
                {
                    TrackedScriptAssetCollectionIds.insert((*AssetCollectionIds)[Index].c_str());
                }
            }

            auto BeginDownloads = [this, Generation, AssetSystem, AssetCollections](const std::vector<csp::systems::Asset>& ScriptAssets)
            {
                const size_t TotalAssets = ScriptAssets.size();
                if (TotalAssets == 0)
                {
                    LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: No JavaScript module assets found for active space.");
                    ScriptModulesLoaded.store(true);
                    return;
                }

                auto DownloadedSources = std::make_shared<ModuleSourceMap>();
                auto DownloadedCount = std::make_shared<size_t>(0);
                auto HasFailed = std::make_shared<bool>(false);

                for (const csp::systems::Asset& ScriptAsset : ScriptAssets)
                {
                    AssetSystem->DownloadAssetData(ScriptAsset,
                        [this, Generation, TotalAssets, ScriptAsset, AssetCollections, DownloadedSources, DownloadedCount, HasFailed](
                            const csp::systems::AssetDataResult& DownloadResult)
                        {
                            if (!IsGenerationCurrent(Generation))
                            {
                                return;
                            }

                            if (DownloadResult.GetResultCode() == csp::systems::EResultCode::InProgress)
                            {
                                return;
                            }

                            if (*HasFailed)
                            {
                                return;
                            }

                            if (DownloadResult.GetResultCode() != csp::systems::EResultCode::Success)
                            {
                                *HasFailed = true;
                                LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxScript: Failed to download a script module.");
                                ScriptModulesLoaded.store(true);
                                return;
                            }

                            const char* ModuleSourceData = static_cast<const char*>(DownloadResult.GetData());
                            const size_t ModuleSourceLength = DownloadResult.GetDataLength();
                            const std::string ModuleSource = (ModuleSourceData != nullptr)
                                ? std::string(ModuleSourceData, ModuleSourceLength)
                                : std::string();

                            const csp::common::String CanonicalPath = ngxscript::BuildCanonicalAssetPath(ScriptAsset, *AssetCollections);
                            (*DownloadedSources)[CanonicalPath.c_str()] = ModuleSource;
                            {
                                std::scoped_lock ModuleLock(ModuleSourcesMutex);
                                LoadedModuleSources[CanonicalPath.c_str()] = ModuleSource;
                            }
                            const std::string DownloadMessage = fmt::format("NgxScript Trace: Loaded module '{}'.", CanonicalPath.c_str());
                            LogSystem.LogMsg(csp::common::LogLevel::Verbose, DownloadMessage.c_str());

                            (*DownloadedCount)++;
                            if (*DownloadedCount < TotalAssets)
                            {
                                return;
                            }

                            size_t LoadedModuleCount = 0;
                            {
                                std::scoped_lock ModuleLock(ModuleSourcesMutex);
                                LoadedModuleSources = *DownloadedSources;
                                LoadedModuleCount = LoadedModuleSources.size();
                            }

                            const std::string LoadCompleteMessage
                                = fmt::format("NgxScript Trace: Module load complete ({} modules).", LoadedModuleCount);
                            LogSystem.LogMsg(csp::common::LogLevel::Log, LoadCompleteMessage.c_str());
                            ScriptModulesLoaded.store(true);
                        });
                }
            };

            AssetSystem->GetAssetsByCriteria(*AssetCollectionIds, nullptr, nullptr, csp::common::Array { csp::systems::EAssetType::SCRIPT_LIBRARY },
                [this, Generation, BeginDownloads](const csp::systems::AssetsResult& ScriptLibraryAssetsResult)
                {
                    if (!IsGenerationCurrent(Generation))
                    {
                        return;
                    }

                    if (ScriptLibraryAssetsResult.GetResultCode() != csp::systems::EResultCode::Success)
                    {
                        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: Failed to load script asset metadata.");
                        ScriptModulesLoaded.store(true);
                        return;
                    }

                    const auto ScriptLibraryAssetsArray = ScriptLibraryAssetsResult.GetAssets();
                    std::vector<csp::systems::Asset> ScriptLibraryAssets;
                    ScriptLibraryAssets.reserve(ScriptLibraryAssetsArray.Size());
                    for (size_t Index = 0; Index < ScriptLibraryAssetsArray.Size(); ++Index)
                    {
                        ScriptLibraryAssets.emplace_back(ScriptLibraryAssetsArray[Index]);
                    }

                    const std::string DiscoveredScriptLibrariesMessage = fmt::format(
                        "NgxScript Trace: Discovered {} script library asset(s) for active space.", ScriptLibraryAssets.size());
                    LogSystem.LogMsg(csp::common::LogLevel::Log, DiscoveredScriptLibrariesMessage.c_str());

                    if (!ScriptLibraryAssets.empty())
                    {
                        BeginDownloads(ScriptLibraryAssets);
                        return;
                    }

                    LogSystem.LogMsg(csp::common::LogLevel::Warning,
                        "NgxScript: No script library assets found for active space. Ensure scripts are uploaded as EAssetType::SCRIPT_LIBRARY.");
                    ScriptModulesLoaded.store(true);
                });
        });
}

void NgxScriptSystem::FetchAssetCollectionMapForSpace(
    uint64_t Generation, std::function<void(std::shared_ptr<AssetCollectionMap>)> Callback) const
{
    const std::string QueryCollectionsMessage
        = fmt::format("NgxScript Trace: Querying asset collections for spaceId='{}'.", ActiveSpaceId.c_str());
    LogSystem.LogMsg(csp::common::LogLevel::Log, QueryCollectionsMessage.c_str());

    if (ActiveSpaceId.IsEmpty())
    {
        return;
    }

    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    if (!AssetSystem)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxScript: AssetSystem unavailable while resolving asset collections.");
        return;
    }

    AssetSystem->FindAssetCollections(
        nullptr, nullptr, nullptr, nullptr, nullptr, csp::common::Array<csp::common::String> { ActiveSpaceId }, nullptr, nullptr,
        [this, Generation, Callback = std::move(Callback)](const csp::systems::AssetCollectionsResult& AssetCollectionsResult)
        {
            if (!IsGenerationCurrent(Generation))
            {
                return;
            }

            if (AssetCollectionsResult.GetResultCode() != csp::systems::EResultCode::Success)
            {
                LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: Failed to query asset collections for space.");
                return;
            }

            auto ResultMap = std::make_shared<AssetCollectionMap>();
            const auto Collections = AssetCollectionsResult.GetAssetCollections();
            for (size_t Index = 0; Index < Collections.Size(); ++Index)
            {
                (*ResultMap)[Collections[Index].Id] = Collections[Index];
            }

            const std::string CollectionsMessage
                = fmt::format("NgxScript Trace: Resolved {} asset collection(s).", ResultMap->Size());
            LogSystem.LogMsg(csp::common::LogLevel::Log, CollectionsMessage.c_str());
            Callback(ResultMap);
        });
}

bool NgxScriptSystem::IsGenerationCurrent(uint64_t Generation) const { return SessionGeneration.load() == Generation; }

bool NgxScriptSystem::EvaluateModuleScript(const std::string& ScriptText, const char* DebugName)
{
    std::scoped_lock ContextLock(ContextMutex);
    if (!Context)
    {
        return false;
    }

    const qjs::Value EvalResult = Context->eval(ScriptText, DebugName, JS_EVAL_TYPE_MODULE);
    if (!EvalResult.isException())
    {
        const std::string DebugNameString = (DebugName != nullptr) ? std::string(DebugName) : std::string();
        const bool bIsCodeComponentSnippet = (DebugNameString.rfind("<ngx-codecomponent-", 0) == 0);
        const bool bIsClientScriptTickSnippet = (DebugNameString == "<ngx-client-script-tick>");
        if (!bIsCodeComponentSnippet && !bIsClientScriptTickSnippet)
        {
            const std::string SuccessMessage = fmt::format("NgxScript Trace: JavaScript execution succeeded ({}).", DebugName);
            LogSystem.LogMsg(csp::common::LogLevel::Verbose, SuccessMessage.c_str());
        }
        return true;
    }

    std::string ErrorMessage = "NgxScript: JavaScript execution failed.";

    try
    {
        const qjs::Value ExceptionValue = Context->getException();
        ErrorMessage += " ";
        ErrorMessage += ExceptionValue.as<std::string>();
    }
    catch (...)
    {
    }

    LogSystem.LogMsg(csp::common::LogLevel::Error, ErrorMessage.c_str());
    return false;
}

bool NgxScriptSystem::HasModuleSource(const csp::common::String& ModulePath) const
{
    const std::string ModulePathStd = ModulePath.c_str();
    if (ModulePathStd.empty())
    {
        return false;
    }

    if (ModulePathStd == PREACT_SIGNALS_CORE_MODULE)
    {
        return true;
    }

    std::scoped_lock ModuleLock(ModuleSourcesMutex);
    const bool bFoundInLoaded = (LoadedModuleSources.find(ModulePathStd) != LoadedModuleSources.end());
    const bool bFoundInStatic = (StaticModuleSources.find(ModulePathStd) != StaticModuleSources.end());
    return bFoundInLoaded || bFoundInStatic;
}

bool NgxScriptSystem::EvaluateSnippet(const csp::common::String& ScriptText, const csp::common::String& DebugName)
{
    const char* DebugNameValue = DebugName.IsEmpty() ? "<ngx-snippet>" : DebugName.c_str();
    return EvaluateModuleScript(ScriptText.c_str(), DebugNameValue);
}

void NgxScriptSystem::PumpPendingJobs() { DrainPendingJobs(); }

bool NgxScriptSystem::AreScriptModulesLoaded() const { return ScriptModulesLoaded.load(); }

uint64_t NgxScriptSystem::GetContextGeneration() const { return ContextGeneration.load(); }

bool NgxScriptSystem::TickScriptRegistry(double TimestampMs)
{
    if (!std::isfinite(TimestampMs))
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: TickScriptRegistry received non-finite timestamp.");
        return false;
    }

    const std::string TickSnippet = "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.tick === 'function') {\n"
        "    globalThis.scriptRegistry.tick(" + fmt::format("{:.6f}", TimestampMs) + ");\n" + "}\n";

    const bool bSuccess = EvaluateSnippet(TickSnippet.c_str(), "<ngx-client-script-tick>");
    PumpPendingJobs();
    return bSuccess;
}

csp::common::String NgxScriptSystem::SyncCodeComponentSchema(const csp::common::String& EntityId)
{
    const std::string SyncSchemaRequestMessage
        = fmt::format("NgxScript Trace: SyncCodeComponentSchema(entityId='{}') called.", EntityId.c_str());
    LogSystem.LogMsg(csp::common::LogLevel::Verbose, SyncSchemaRequestMessage.c_str());

    if (EntityId.IsEmpty())
    {
        return EMPTY_JSON_OBJECT_STRING;
    }

    if (!ScriptModulesLoaded.load())
    {
        const std::string DeferredMessage = fmt::format(
            "NgxScript Trace: SyncCodeComponentSchema(entityId='{}') deferred while script modules are loading.", EntityId.c_str());
        LogSystem.LogMsg(csp::common::LogLevel::Verbose, DeferredMessage.c_str());
        return EMPTY_JSON_OBJECT_STRING;
    }

    const std::string EntityIdEscaped = EscapeJSStringLiteral(EntityId.c_str());
    const std::string Snippet = "globalThis." + std::string(CODECOMPONENT_JSON_RESULT_SLOT) + " = '{}';\n"
        "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.syncCodeComponentSchema === 'function') {\n"
        "    const __cspResult = globalThis.scriptRegistry.syncCodeComponentSchema('"
        + EntityIdEscaped
        + "');\n"
          "    const __cspSerializable = (__cspResult === undefined || __cspResult === null) ? {} : __cspResult;\n"
          "    try {\n"
          "        globalThis."
        + std::string(CODECOMPONENT_JSON_RESULT_SLOT)
        + " = JSON.stringify(__cspSerializable);\n"
          "    } catch (_error) {\n"
          "        globalThis."
        + std::string(CODECOMPONENT_JSON_RESULT_SLOT)
        + " = '{}';\n"
          "    }\n"
          "}\n";

    auto ReadJsonResultString = [this]() -> std::string
    {
        std::scoped_lock ContextLock(ContextMutex);
        if (!Context)
        {
            return EMPTY_JSON_OBJECT_STRING;
        }

        try
        {
            const qjs::Value JsonResult = Context->global()[CODECOMPONENT_JSON_RESULT_SLOT];
            return JsonResult.as<std::string>();
        }
        catch (...)
        {
            return EMPTY_JSON_OBJECT_STRING;
        }
    };

    if (!EvaluateSnippet(Snippet.c_str(), "<ngx-codecomponent-sync-schema-bridge>"))
    {
        PumpPendingJobs();
        return EMPTY_JSON_OBJECT_STRING;
    }

    PumpPendingJobs();
    std::string JsonResultString = ReadJsonResultString();

    // If module import completed during pending jobs, one immediate retry returns the resolved schema payload
    // rather than requiring a second user click.
    if (JsonResultString == EMPTY_JSON_OBJECT_STRING)
    {
        if (EvaluateSnippet(Snippet.c_str(), "<ngx-codecomponent-sync-schema-bridge-retry>"))
        {
            PumpPendingJobs();
            JsonResultString = ReadJsonResultString();
        }
        else
        {
            PumpPendingJobs();
        }
    }

    const std::string SyncSchemaResultMessage = fmt::format(
        "NgxScript Trace: SyncCodeComponentSchema(entityId='{}') resultLength={}.", EntityId.c_str(), JsonResultString.size());
    LogSystem.LogMsg(csp::common::LogLevel::Verbose, SyncSchemaResultMessage.c_str());
    return csp::common::String(JsonResultString.c_str());
}

bool NgxScriptSystem::AddCodeComponent(const csp::common::String& EntityId, const csp::common::String& PayloadJson)
{
    const std::string AddRequestMessage = fmt::format(
        "NgxScript Trace: AddCodeComponent(entityId='{}', payloadLength={}) called.", EntityId.c_str(), PayloadJson.Length());
    LogSystem.LogMsg(csp::common::LogLevel::Verbose, AddRequestMessage.c_str());

    if (EntityId.IsEmpty())
    {
        return false;
    }

    if (!ScriptModulesLoaded.load())
    {
        const std::string DeferredMessage = fmt::format(
            "NgxScript Trace: AddCodeComponent(entityId='{}') deferred until script modules finish loading.", EntityId.c_str());
        LogSystem.LogMsg(csp::common::LogLevel::Verbose, DeferredMessage.c_str());
        return true;
    }

    const std::string EntityIdEscaped = EscapeJSStringLiteral(EntityId.c_str());
    const std::string PayloadJsonEscaped = EscapeJSStringLiteral(PayloadJson.c_str());
    const std::string Snippet = "globalThis." + std::string(CODECOMPONENT_BOOL_RESULT_SLOT) + " = false;\n"
        "let __cspPayload = {};\n"
        "try {\n"
        "    __cspPayload = JSON.parse('" + PayloadJsonEscaped + "');\n"
        "} catch (_error) {\n"
        "    __cspPayload = {};\n"
        "}\n"
        "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.addCodeComponent === 'function') {\n"
        "    globalThis."
        + std::string(CODECOMPONENT_BOOL_RESULT_SLOT)
        + " = !!globalThis.scriptRegistry.addCodeComponent('"
        + EntityIdEscaped + "', __cspPayload);\n"
                           "}\n";

    if (!EvaluateSnippet(Snippet.c_str(), "<ngx-codecomponent-add-bridge>"))
    {
        PumpPendingJobs();
        return false;
    }

    PumpPendingJobs();

    std::scoped_lock ContextLock(ContextMutex);
    if (!Context)
    {
        return false;
    }

    try
    {
        const qjs::Value BoolResult = Context->global()[CODECOMPONENT_BOOL_RESULT_SLOT];
        const bool bResult = BoolResult.as<bool>();
        const std::string AddResultMessage
            = fmt::format("NgxScript Trace: AddCodeComponent(entityId='{}') -> {}.", EntityId.c_str(), bResult);
        LogSystem.LogMsg(csp::common::LogLevel::Verbose, AddResultMessage.c_str());
        return bResult;
    }
    catch (...)
    {
        return false;
    }
}

csp::common::String NgxScriptSystem::SyncCodeComponentAttributes(
    const csp::common::String& EntityId, const csp::common::String& AttributesJson)
{
    if (EntityId.IsEmpty())
    {
        return EMPTY_JSON_OBJECT_STRING;
    }

    if (!ScriptModulesLoaded.load())
    {
        const std::string DeferredMessage = fmt::format(
            "NgxScript Trace: SyncCodeComponentAttributes(entityId='{}') deferred while script modules are loading.", EntityId.c_str());
        LogSystem.LogMsg(csp::common::LogLevel::Verbose, DeferredMessage.c_str());
        return EMPTY_JSON_OBJECT_STRING;
    }

    const std::string EntityIdEscaped = EscapeJSStringLiteral(EntityId.c_str());
    const std::string AttributesJsonEscaped = EscapeJSStringLiteral(AttributesJson.c_str());
    const std::string Snippet = "globalThis." + std::string(CODECOMPONENT_JSON_RESULT_SLOT) + " = '{}';\n"
        "let __cspAttributes = {};\n"
        "try {\n"
        "    __cspAttributes = JSON.parse('" + AttributesJsonEscaped + "');\n"
        "} catch (_error) {\n"
        "    __cspAttributes = {};\n"
        "}\n"
        "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.syncCodeComponentAttributes === 'function') {\n"
        "    const __cspResult = globalThis.scriptRegistry.syncCodeComponentAttributes('" + EntityIdEscaped + "', __cspAttributes);\n"
        "    const __cspSerializable = (__cspResult === undefined || __cspResult === null) ? {} : __cspResult;\n"
        "    try {\n"
        "        globalThis."
        + std::string(CODECOMPONENT_JSON_RESULT_SLOT)
        + " = JSON.stringify(__cspSerializable);\n"
          "    } catch (_error) {\n"
          "        globalThis."
        + std::string(CODECOMPONENT_JSON_RESULT_SLOT)
        + " = '{}';\n"
          "    }\n"
          "}\n";

    auto ReadJsonResultString = [this]() -> std::string
    {
        std::scoped_lock ContextLock(ContextMutex);
        if (!Context)
        {
            return EMPTY_JSON_OBJECT_STRING;
        }

        try
        {
            const qjs::Value JsonResult = Context->global()[CODECOMPONENT_JSON_RESULT_SLOT];
            return JsonResult.as<std::string>();
        }
        catch (...)
        {
            return EMPTY_JSON_OBJECT_STRING;
        }
    };

    if (!EvaluateSnippet(Snippet.c_str(), "<ngx-codecomponent-sync-attributes-bridge>"))
    {
        PumpPendingJobs();
        return EMPTY_JSON_OBJECT_STRING;
    }

    PumpPendingJobs();
    std::string JsonResultString = ReadJsonResultString();

    if (JsonResultString == EMPTY_JSON_OBJECT_STRING)
    {
        if (EvaluateSnippet(Snippet.c_str(), "<ngx-codecomponent-sync-attributes-bridge-retry>"))
        {
            PumpPendingJobs();
            JsonResultString = ReadJsonResultString();
        }
        else
        {
            PumpPendingJobs();
        }
    }

    return csp::common::String(JsonResultString.c_str());
}

bool NgxScriptSystem::UpdateAttributeForEntity(
    const csp::common::String& EntityId, const csp::common::String& Key, const csp::common::String& ValueJson)
{
    if (EntityId.IsEmpty() || Key.IsEmpty())
    {
        return false;
    }

    if (!ScriptModulesLoaded.load())
    {
        const std::string DeferredMessage = fmt::format(
            "NgxScript Trace: UpdateAttributeForEntity(entityId='{}', key='{}') deferred while script modules are loading.",
            EntityId.c_str(),
            Key.c_str());
        LogSystem.LogMsg(csp::common::LogLevel::Verbose, DeferredMessage.c_str());
        return true;
    }

    const std::string EntityIdEscaped = EscapeJSStringLiteral(EntityId.c_str());
    const std::string KeyEscaped = EscapeJSStringLiteral(Key.c_str());
    const std::string ValueJsonEscaped = EscapeJSStringLiteral(ValueJson.c_str());
    const std::string Snippet = "globalThis." + std::string(CODECOMPONENT_BOOL_RESULT_SLOT) + " = false;\n"
        "let __cspValue = null;\n"
        "try {\n"
        "    __cspValue = JSON.parse('" + ValueJsonEscaped + "');\n"
        "} catch (_error) {\n"
        "    __cspValue = null;\n"
        "}\n"
        "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.updateAttributeForEntity === 'function') {\n"
        "    globalThis."
        + std::string(CODECOMPONENT_BOOL_RESULT_SLOT)
        + " = !!globalThis.scriptRegistry.updateAttributeForEntity('"
        + EntityIdEscaped + "', '" + KeyEscaped + "', __cspValue);\n"
                                                  "}\n";

    if (!EvaluateSnippet(Snippet.c_str(), "<ngx-codecomponent-update-attribute-bridge>"))
    {
        PumpPendingJobs();
        return false;
    }

    PumpPendingJobs();

    std::scoped_lock ContextLock(ContextMutex);
    if (!Context)
    {
        return false;
    }

    try
    {
        const qjs::Value BoolResult = Context->global()[CODECOMPONENT_BOOL_RESULT_SLOT];
        return BoolResult.as<bool>();
    }
    catch (...)
    {
        return false;
    }
}

bool NgxScriptSystem::RemoveCodeComponent(const csp::common::String& EntityId)
{
    if (EntityId.IsEmpty())
    {
        return false;
    }

    if (!ScriptModulesLoaded.load())
    {
        const std::string DeferredMessage = fmt::format(
            "NgxScript Trace: RemoveCodeComponent(entityId='{}') deferred while script modules are loading.", EntityId.c_str());
        LogSystem.LogMsg(csp::common::LogLevel::Verbose, DeferredMessage.c_str());
        return true;
    }

    const std::string EntityIdEscaped = EscapeJSStringLiteral(EntityId.c_str());
    const std::string Snippet = "globalThis." + std::string(CODECOMPONENT_BOOL_RESULT_SLOT) + " = false;\n"
        "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.removeCodeComponent === 'function') {\n"
        "    globalThis."
        + std::string(CODECOMPONENT_BOOL_RESULT_SLOT)
        + " = !!globalThis.scriptRegistry.removeCodeComponent('"
        + EntityIdEscaped + "');\n"
                           "}\n";

    if (!EvaluateSnippet(Snippet.c_str(), "<ngx-codecomponent-remove-bridge>"))
    {
        PumpPendingJobs();
        return false;
    }

    PumpPendingJobs();

    std::scoped_lock ContextLock(ContextMutex);
    if (!Context)
    {
        return false;
    }

    try
    {
        const qjs::Value BoolResult = Context->global()[CODECOMPONENT_BOOL_RESULT_SLOT];
        return BoolResult.as<bool>();
    }
    catch (...)
    {
        return false;
    }
}

bool NgxScriptSystem::ExecuteModule(const csp::common::String& ModulePath)
{
    const std::string ModulePathStd = ModulePath.c_str();
    const std::string RunModuleMessage = fmt::format("NgxScript Trace: Executing module '{}'.", ModulePathStd);
    LogSystem.LogMsg(csp::common::LogLevel::Log, RunModuleMessage.c_str());

    {
        std::scoped_lock ModuleLock(ModuleSourcesMutex);
        const bool bFoundInLoaded = (LoadedModuleSources.find(ModulePathStd) != LoadedModuleSources.end());
        const bool bFoundInStatic = (StaticModuleSources.find(ModulePathStd) != StaticModuleSources.end());
        const bool bFoundInBuiltIn = (ModulePathStd == PREACT_SIGNALS_CORE_MODULE);
        if (!bFoundInLoaded && !bFoundInStatic && !bFoundInBuiltIn)
        {
            const std::string WarningMessage = fmt::format(
                "NgxScript: Module '{}' not found in loaded or static module maps.", ModulePathStd);
            LogSystem.LogMsg(csp::common::LogLevel::Warning, WarningMessage.c_str());
            return false;
        }
    }

    const std::string Script = "import '" + EscapeJSStringLiteral(ModulePathStd) + "';";

    const bool bSuccess = EvaluateModuleScript(Script, "<ngx-module>");

    const std::string CompletedMessage = fmt::format(
        "NgxScript Trace: Module '{}' {}.", ModulePathStd, bSuccess ? "completed" : "failed");
    LogSystem.LogMsg(csp::common::LogLevel::Log, CompletedMessage.c_str());
    return bSuccess;
}

} // namespace csp::systems
