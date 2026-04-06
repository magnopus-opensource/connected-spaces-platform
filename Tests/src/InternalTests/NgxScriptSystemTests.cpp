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

#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Common/Interfaces/IJSScriptRunner.h"
#include "CSP/Common/SharedConstants.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/Components/CodeAttribute.h"
#include "CSP/Multiplayer/Components/CodeSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AlphaVideoMaterial.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/Assets/GLTFMaterial.h"
#include "CSP/Systems/Assets/RuntimeMaterialSystem.h"
#include "CSP/Systems/Assets/TextureInfo.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "Events/EventId.h"
#include "Events/EventSystem.h"
#include "Multiplayer/EntityQueryUtils.h"
#include "Multiplayer/NgxScript/NgxCodeComponentRuntime.h"
#include "Multiplayer/NgxScript/NgxScriptSystem.h"
#include "Multiplayer/Script/RuntimeMaterialScriptInterface.h"
#include "Multiplayer/Script/RuntimeMaterialTextureScriptInterface.h"
#include "TestHelpers.h"

#include <rapidjson/document.h>

#include <algorithm>
#include <atomic>
#include <string>
#include <vector>

namespace
{

class TestRealtimeEngine final : public csp::common::IRealtimeEngine
{
public:
    explicit TestRealtimeEngine(csp::common::RealtimeEngineType InType)
        : Type(InType)
    {
    }

    csp::common::RealtimeEngineType GetRealtimeEngineType() const override { return Type; }

private:
    csp::common::RealtimeEngineType Type;
};

class TestRealtimeEngineWithEntities final : public csp::common::IRealtimeEngine
{
public:
    explicit TestRealtimeEngineWithEntities(csp::common::RealtimeEngineType InType)
        : Type(InType)
        , bEntityLockHeld(false)
        , LockCount(0)
        , UnlockCount(0)
    {
    }

    csp::common::RealtimeEngineType GetRealtimeEngineType() const override { return Type; }

    size_t GetNumEntities() const override { return Entities.size(); }

    csp::multiplayer::SpaceEntity* GetEntityByIndex(size_t EntityIndex) override
    {
        if (EntityIndex >= Entities.size())
        {
            return nullptr;
        }
        return Entities[EntityIndex];
    }

    csp::multiplayer::SpaceEntity* FindSpaceEntityById(uint64_t EntityId) override
    {
        for (auto* Entity : Entities)
        {
            if (Entity != nullptr && Entity->GetId() == EntityId)
            {
                return Entity;
            }
        }

        return nullptr;
    }

    bool AddEntityToSelectedEntities(csp::multiplayer::SpaceEntity* Entity) override
    {
        if (std::find(SelectedEntities.begin(), SelectedEntities.end(), Entity) != SelectedEntities.end())
        {
            return false;
        }

        SelectedEntities.push_back(Entity);
        return true;
    }

    bool RemoveEntityFromSelectedEntities(csp::multiplayer::SpaceEntity* Entity) override
    {
        const auto SelectedEntityIt = std::find(SelectedEntities.begin(), SelectedEntities.end(), Entity);
        if (SelectedEntityIt == SelectedEntities.end())
        {
            return false;
        }

        SelectedEntities.erase(SelectedEntityIt);
        return true;
    }

    csp::multiplayer::ModifiableStatus IsEntityModifiable(const csp::multiplayer::SpaceEntity*) const override
    {
        return csp::multiplayer::ModifiableStatus::Modifiable;
    }

    void ResolveEntityHierarchy(csp::multiplayer::SpaceEntity* Entity) override
    {
        if (Entity != nullptr)
        {
            Entity->ResolveParentChildRelationship();
        }
    }

    void LockEntityUpdate() override
    {
        bEntityLockHeld = true;
        ++LockCount;
    }

    bool TryLockEntityUpdate() override
    {
        if (bEntityLockHeld)
        {
            return false;
        }

        LockEntityUpdate();
        return true;
    }

    void UnlockEntityUpdate() override
    {
        if (bEntityLockHeld)
        {
            bEntityLockHeld = false;
            ++UnlockCount;
        }
    }

    void AddEntity(csp::multiplayer::SpaceEntity* Entity) { Entities.push_back(Entity); }

    void RemoveEntity(csp::multiplayer::SpaceEntity* Entity)
    {
        Entities.erase(std::remove(Entities.begin(), Entities.end(), Entity), Entities.end());
    }

    int32_t GetLockCount() const { return LockCount; }
    int32_t GetUnlockCount() const { return UnlockCount; }

private:
    csp::common::RealtimeEngineType Type;
    bool bEntityLockHeld;
    int32_t LockCount;
    int32_t UnlockCount;
    std::vector<csp::multiplayer::SpaceEntity*> Entities;
    std::vector<csp::multiplayer::SpaceEntity*> SelectedEntities;
};

class TestScriptRunner final : public csp::common::IJSScriptRunner
{
public:
    void RegisterScriptBinding(csp::common::IScriptBinding*) override { }
    void UnregisterScriptBinding(csp::common::IScriptBinding*) override { }
};

csp::multiplayer::CodeAttribute::EntityQueryValueType BuildNameEntityQuery(const csp::common::String& Name)
{
    csp::multiplayer::CodeAttribute::EntityQueryValueType Query;
    Query["kind"] = "name";
    Query["name"] = Name;
    return Query;
}

csp::multiplayer::CodeAttribute::ImageAssetValueType BuildImageAssetValue(
    const csp::common::String& AssetCollectionId, const csp::common::String& ImageAssetId)
{
    csp::multiplayer::CodeAttribute::ImageAssetValueType Value;
    Value["assetCollectionId"] = AssetCollectionId;
    Value["imageAssetId"] = ImageAssetId;
    return Value;
}

const csp::multiplayer::NgxScriptAttribute* FindScriptAttributeByName(
    const csp::common::List<csp::multiplayer::NgxScriptAttribute>& Attributes, const char* Name)
{
    for (size_t Index = 0; Index < Attributes.Size(); ++Index)
    {
        if (Attributes[Index].Name == Name)
        {
            return &Attributes[Index];
        }
    }

    return nullptr;
}

std::string BuildTagQueryJson(const char* Tag)
{
    return std::string("{\"kind\":\"tag\",\"tag\":\"") + Tag + "\"}";
}

std::string BuildAndTagQueryJson(const char* LeftTag, const char* RightTag)
{
    return std::string("{\"kind\":\"and\",\"operands\":[{\"kind\":\"tag\",\"tag\":\"") + LeftTag + "\"},{\"kind\":\"tag\",\"tag\":\"" + RightTag
        + "\"}]}";
}

std::string BuildNotTagQueryJson(const char* Tag)
{
    return std::string("{\"kind\":\"not\",\"operand\":{\"kind\":\"tag\",\"tag\":\"") + Tag + "\"}}";
}

void ProcessTickEvent()
{
    auto& EventSystem = csp::events::EventSystem::Get();
    csp::events::Event* TickEvent = EventSystem.AllocateEvent(csp::events::FOUNDATION_TICK_EVENT_ID);
    EventSystem.EnqueueEvent(TickEvent);
    EventSystem.ProcessEvents();
}

void ProcessScriptTick(csp::systems::NgxScriptSystem& NgxScriptSystem, double /*TimestampMs*/)
{
    EXPECT_TRUE(NgxScriptSystem.FlushPendingCodeComponentUI());
}

void ProcessAnimationFrameTick(csp::systems::NgxScriptSystem& NgxScriptSystem, double TimestampMs)
{
    EXPECT_TRUE(NgxScriptSystem.TickAnimationFrame(TimestampMs));
}

rapidjson::Document ParseJson(const std::string& Json)
{
    rapidjson::Document Document;
    Document.Parse(Json.c_str());
    return Document;
}

const rapidjson::Value* FindDrawableById(const rapidjson::Document& Document, const char* Id)
{
    if (!Document.IsArray())
    {
        return nullptr;
    }

    for (rapidjson::SizeType Index = 0; Index < Document.Size(); ++Index)
    {
        const rapidjson::Value& Entry = Document[Index];
        if (Entry.IsObject() && Entry.HasMember("id") && Entry["id"].IsString() && std::string(Entry["id"].GetString()) == Id)
        {
            return &Entry;
        }
    }

    return nullptr;
}

const rapidjson::Value* FindUpdateDrawableByType(const rapidjson::Document& Document, const char* Op, const char* Type)
{
    if (!Document.IsArray())
    {
        return nullptr;
    }

    for (rapidjson::SizeType Index = 0; Index < Document.Size(); ++Index)
    {
        const rapidjson::Value& Entry = Document[Index];
        if (!Entry.IsObject() || !Entry.HasMember("op") || !Entry["op"].IsString())
        {
            continue;
        }

        if (std::string(Entry["op"].GetString()) != Op || !Entry.HasMember("drawable") || !Entry["drawable"].IsObject())
        {
            continue;
        }

        const rapidjson::Value& Drawable = Entry["drawable"];
        if (Drawable.HasMember("type") && Drawable["type"].IsString() && std::string(Drawable["type"].GetString()) == Type)
        {
            return &Drawable;
        }
    }

    return nullptr;
}

void RegisterTrackingRegistryModule(csp::systems::NgxScriptSystem& NgxScriptSystem)
{
    NgxScriptSystem.RegisterStaticModuleSource("/scripts/engine/registry.js", R"(
export function createScriptRegistry() {
    const entries = new Map();
    globalThis.__ngxAddCalls = 0;
    globalThis.__ngxSchemaSyncCalls = 0;
    globalThis.__ngxSyncAttributesCalls = 0;
    globalThis.__ngxUpdateCalls = 0;
    globalThis.__ngxRemoveCalls = 0;
    globalThis.__ngxTickCalls = 0;
    globalThis.__ngxTickMonotonic = 1;
    globalThis.__ngxTickLast = -1;

    return {
        syncCodeComponentSchema(_entityId) {
            globalThis.__ngxSchemaSyncCalls += 1;
        },
        addCodeComponent(entityId, payload = {}) {
            globalThis.__ngxAddCalls += 1;
            entries.set(entityId, { attributes: { ...(payload.attributes || {}) } });
        },
        syncCodeComponentAttributes(entityId, attributes = {}) {
            globalThis.__ngxSyncAttributesCalls += 1;
            const entry = entries.get(entityId);
            if (!entry) {
                return;
            }

            entry.attributes = { ...attributes };
        },
        updateAttributeForEntity(entityId, key, value) {
            globalThis.__ngxUpdateCalls += 1;
            const entry = entries.get(entityId);
            if (!entry) {
                return;
            }

            entry.attributes[key] = value;
        },
        removeCodeComponent(entityId) {
            globalThis.__ngxRemoveCalls += 1;
            entries.delete(entityId);
        },
        tick(timestampMs) {
            globalThis.__ngxTickCalls += 1;
            if (globalThis.__ngxTickLast > timestampMs) {
                globalThis.__ngxTickMonotonic = 0;
            }
            globalThis.__ngxTickLast = timestampMs;
        },
        destroy() {
            globalThis.__ngxRegistryDestroyed = 1;
        }
    };
}
)");
}

void RunRuntimeSyncSmokeScenario(csp::common::RealtimeEngineType EngineType)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(EngineType);

    RegisterTrackingRegistryModule(NgxScriptSystem);
    NgxScriptSystem.OnEnterSpace("code-sync-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("code-sync-space", &Engine);

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    EXPECT_NE(CodeComponent, nullptr);
    ASSERT_NE(CodeComponent, nullptr);

    Entity.SetUpdateCallback([](csp::multiplayer::SpaceEntity*, csp::multiplayer::SpaceEntityUpdateFlags, csp::common::Array<csp::multiplayer::ComponentUpdateInfo>&)
    {
    });
    EXPECT_TRUE(static_cast<bool>(Entity.GetEntityUpdateCallback()));

    CodeComponent->SetScriptAssetPath("/scripts/modules/test-module.js");
    CodeComponent->SetAttribute("score", csp::multiplayer::CodeAttribute::FromInteger(1));
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxAddCalls", -1), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxSchemaSyncCalls", -1), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxSyncAttributesCalls", -1), 1);
    EXPECT_GE(NgxScriptSystem.GetGlobalIntForTesting("__ngxTickCalls", -1), 1);

    ProcessScriptTick(NgxScriptSystem, 10.0);
    EXPECT_GE(NgxScriptSystem.GetGlobalIntForTesting("__ngxTickCalls", -1), 2);

    CodeComponent->SetAttribute("score", csp::multiplayer::CodeAttribute::FromInteger(2));
    ProcessTickEvent();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxUpdateCalls", -1), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxAddCalls", -1), 1);
    ProcessScriptTick(NgxScriptSystem, 20.0);
    EXPECT_GE(NgxScriptSystem.GetGlobalIntForTesting("__ngxTickCalls", -1), 4);

    CodeComponent->RemoveAttribute("score");
    ProcessTickEvent();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxAddCalls", -1), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRemoveCalls", -1), 0);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxSyncAttributesCalls", -1), 2);

    Engine.RemoveEntity(&Entity);
    ProcessTickEvent();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRemoveCalls", -1), 1);
    ProcessScriptTick(NgxScriptSystem, 30.0);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxTickMonotonic", -1), 1);

    EXPECT_EQ(Engine.GetLockCount(), Engine.GetUnlockCount());
    EXPECT_TRUE(static_cast<bool>(Entity.GetEntityUpdateCallback()));

    ASSERT_TRUE(NgxScriptSystem.EvaluateModuleScriptForTesting("globalThis.scriptRegistry = {};"));
    ProcessTickEvent();
    ProcessScriptTick(NgxScriptSystem, 40.0);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxTickMonotonic", -1), 1);

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
}

} // namespace

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, OfflineEnterExitBootstrapsAndTearsDown)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    TestRealtimeEngine OfflineEngine(csp::common::RealtimeEngineType::Offline);

    EXPECT_FALSE(NgxScriptSystem.HasActiveContextForTesting());

    NgxScriptSystem.OnEnterSpace("offline-test-space", &OfflineEngine);
    EXPECT_TRUE(NgxScriptSystem.HasActiveContextForTesting());

    NgxScriptSystem.OnExitSpace();
    EXPECT_FALSE(NgxScriptSystem.HasActiveContextForTesting());
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, OnlineEnterExitBootstrapsAndTearsDown)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    TestRealtimeEngine OnlineEngine(csp::common::RealtimeEngineType::Online);

    EXPECT_FALSE(NgxScriptSystem.HasActiveContextForTesting());

    NgxScriptSystem.OnEnterSpace("online-test-space", &OnlineEngine);
    EXPECT_TRUE(NgxScriptSystem.HasActiveContextForTesting());

    NgxScriptSystem.OnExitSpace();
    EXPECT_FALSE(NgxScriptSystem.HasActiveContextForTesting());
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, TickEventDrainsPendingJobs)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    TestRealtimeEngine OfflineEngine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("tick-test-space", &OfflineEngine);

    ASSERT_TRUE(NgxScriptSystem.EvaluateModuleScriptForTesting(
        "globalThis.__ngxTickValue = 1; Promise.resolve().then(() => { globalThis.__ngxTickValue = 2; });"));
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxTickValue", -1), 1);

    ProcessTickEvent();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxTickValue", -1), 2);

    NgxScriptSystem.OnExitSpace();
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, RequestAnimationFrameUsesAnimationFrameTickNotFoundationTick)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    TestRealtimeEngine OfflineEngine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("raf-hook-space", &OfflineEngine);

    ASSERT_TRUE(NgxScriptSystem.EvaluateModuleScriptForTesting(R"(
globalThis.__ngxRafCallbacks = 0;
globalThis.__ngxRafCancelled = 0;
globalThis.__ngxRafFirstTimestampRounded = -1;
globalThis.__ngxRafSecondTimestampRounded = -1;

const cancelledHandle = requestAnimationFrame(() => {
    globalThis.__ngxRafCancelled = 1;
});
cancelAnimationFrame(cancelledHandle);

requestAnimationFrame((timestampMs) => {
    globalThis.__ngxRafCallbacks += 1;
    globalThis.__ngxRafFirstTimestampRounded = Math.round(timestampMs);

    requestAnimationFrame((nextTimestampMs) => {
        globalThis.__ngxRafCallbacks += 1;
        globalThis.__ngxRafSecondTimestampRounded = Math.round(nextTimestampMs);
    });
});
)"));

    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRafCallbacks", -1), 0);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRafCancelled", -1), 0);

    ProcessTickEvent();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRafCallbacks", -1), 0);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRafCancelled", -1), 0);

    ProcessAnimationFrameTick(NgxScriptSystem, 1000.0);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRafCallbacks", -1), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRafFirstTimestampRounded", -1), 1000);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRafSecondTimestampRounded", -1), -1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRafCancelled", -1), 0);

    ProcessTickEvent();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRafCallbacks", -1), 1);

    ProcessAnimationFrameTick(NgxScriptSystem, 1016.0);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRafCallbacks", -1), 2);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRafSecondTimestampRounded", -1), 1016);
    EXPECT_GE(NgxScriptSystem.GetGlobalIntForTesting("__ngxRafSecondTimestampRounded", -1),
        NgxScriptSystem.GetGlobalIntForTesting("__ngxRafFirstTimestampRounded", -1));
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRafCancelled", -1), 0);

    NgxScriptSystem.OnExitSpace();
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, EnterSpaceDoesNotExecuteUserScriptsByDefault)
{
    std::atomic<int32_t> ScriptExecutionLogCount = 0;
    csp::common::LogSystem LogSystem;
    LogSystem.SetLogCallback([&ScriptExecutionLogCount](csp::common::LogLevel, const csp::common::String& Message)
    {
        if (std::string(Message.c_str()).find("NgxScript Trace: Executing module") != std::string::npos)
        {
            ++ScriptExecutionLogCount;
        }
    });

    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    TestRealtimeEngine OfflineEngine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("hello-space-a", &OfflineEngine);
    NgxScriptSystem.OnExitSpace();
    NgxScriptSystem.OnEnterSpace("hello-space-b", &OfflineEngine);
    NgxScriptSystem.OnExitSpace();

    EXPECT_EQ(ScriptExecutionLogCount.load(), 0);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CanonicalPathMappingIsStable)
{
    csp::systems::Asset ScriptAsset {};
    ScriptAsset.Name = "main.js___AF12";
    ScriptAsset.AssetCollectionId = "collection-child";

    csp::systems::AssetCollection RootCollection {};
    RootCollection.Id = "collection-root";
    RootCollection.Name = "scripts___B100";
    RootCollection.ParentId = "";

    csp::systems::AssetCollection ChildCollection {};
    ChildCollection.Id = "collection-child";
    ChildCollection.Name = "engine___C200";
    ChildCollection.ParentId = "collection-root";

    csp::common::Map<csp::common::String, csp::systems::AssetCollection> CollectionMap {};
    CollectionMap[RootCollection.Id] = RootCollection;
    CollectionMap[ChildCollection.Id] = ChildCollection;

    EXPECT_EQ(csp::systems::ngxscript::StripRandomPartFromScriptPath("main.js___AF12"), "main.js");
    EXPECT_EQ(csp::systems::ngxscript::BuildCanonicalAssetPath(ScriptAsset, CollectionMap), "/scripts/engine/main.js");
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, ModuleImportResolvesFromInMemoryMap)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/lib/value.js", "export const value = 41;");
    NgxScriptSystem.SetLoadedModuleSourceForTesting(
        "/scripts/engine/main.js", "import { value } from '/scripts/lib/value.js'; globalThis.__ngxModuleValue = value + 1;");

    ASSERT_TRUE(NgxScriptSystem.RunModuleForTesting("/scripts/engine/main.js"));
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxModuleValue", -1), 42);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, BuiltInSignalsModuleImportResolvesAndExecutes)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/engine/signals-main.js",
        "import { signal } from '@preact/signals-core'; "
        "const value = signal(3); "
        "globalThis.__ngxSignalInitial = value.value; "
        "value.value = 7; "
        "globalThis.__ngxSignalUpdated = value.value;");

    ASSERT_TRUE(NgxScriptSystem.RunModuleForTesting("/scripts/engine/signals-main.js"));
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxSignalInitial", -1), 3);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxSignalUpdated", -1), 7);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, ExecuteModuleRunsBuiltInSignalsModule)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);

    ASSERT_TRUE(NgxScriptSystem.RunModuleForTesting("@preact/signals-core"));
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, ExecuteModuleRunsLoadedModuleSource)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/engine/custom-module.js", "globalThis.__ngxCustomModuleLoaded = 1;");

    ASSERT_TRUE(NgxScriptSystem.RunModuleForTesting("/scripts/engine/custom-module.js"));
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxCustomModuleLoaded", -1), 1);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, ExecuteModuleRunsStaticModuleSource)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);

    NgxScriptSystem.RegisterStaticModuleSource("/__test/static-module.js", "globalThis.__ngxStaticModuleLoaded = 17;");

    ASSERT_TRUE(NgxScriptSystem.RunModuleForTesting("/__test/static-module.js"));
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxStaticModuleLoaded", -1), 17);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentBridgeMethodsReturnExpectedJsonAndBoolValues)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    TestRealtimeEngine OfflineEngine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("bridge-methods-space", &OfflineEngine);

    ASSERT_TRUE(NgxScriptSystem.EvaluateModuleScriptForTesting(R"(
globalThis.scriptRegistry = {
    syncCodeComponentSchema(entityId) {
        return {
            speed: 1,
            enabled: true,
            target: { kind: 'id', id: entityId },
        };
    },
    addCodeComponent(entityId, payload = {}) {
        return entityId === '200'
            && payload.scriptAssetPath === '/scripts/modules/test.js'
            && payload.attributes
            && payload.attributes.speed === 3;
    },
    syncCodeComponentAttributes(entityId, attributes = {}) {
        return {
            ...attributes,
            echoedEntityId: entityId,
        };
    },
    updateAttributeForEntity(entityId, key, value) {
        return entityId === '200' && key === 'speed' && value === 6;
    },
    removeCodeComponent(entityId) {
        return entityId === '200';
    },
};
)"));

    const std::string SchemaJson = NgxScriptSystem.SyncCodeComponentSchema("200").c_str();
    EXPECT_NE(SchemaJson.find("\"speed\":1"), std::string::npos);
    EXPECT_NE(SchemaJson.find("\"enabled\":true"), std::string::npos);
    EXPECT_NE(SchemaJson.find("\"kind\":\"id\""), std::string::npos);

    EXPECT_TRUE(NgxScriptSystem.AddCodeComponent("200", R"({"scriptAssetPath":"/scripts/modules/test.js","attributes":{"speed":3}})"));

    const std::string SyncedAttributesJson
        = NgxScriptSystem.SyncCodeComponentAttributes("200", R"({"speed":3,"target":{"kind":"id","id":"123"}})").c_str();
    EXPECT_NE(SyncedAttributesJson.find("\"echoedEntityId\":\"200\""), std::string::npos);
    EXPECT_NE(SyncedAttributesJson.find("\"speed\":3"), std::string::npos);
    EXPECT_NE(SyncedAttributesJson.find("\"kind\":\"id\""), std::string::npos);

    EXPECT_TRUE(NgxScriptSystem.UpdateAttributeForEntity("200", "speed", "6"));
    EXPECT_TRUE(NgxScriptSystem.RemoveCodeComponent("200"));

    NgxScriptSystem.OnExitSpace();
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentBridgeMethodsFallbackWhenRegistryUnavailableOrEntityIdInvalid)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    TestRealtimeEngine OfflineEngine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("bridge-fallback-space", &OfflineEngine);
    ASSERT_TRUE(NgxScriptSystem.EvaluateModuleScriptForTesting("delete globalThis.scriptRegistry;"));

    EXPECT_EQ(std::string(NgxScriptSystem.SyncCodeComponentSchema("123").c_str()), "{}");
    EXPECT_EQ(std::string(NgxScriptSystem.SyncCodeComponentAttributes("123", "{\"score\":1}").c_str()), "{}");
    EXPECT_FALSE(NgxScriptSystem.AddCodeComponent("123", "{\"scriptAssetPath\":\"/scripts/test.js\",\"attributes\":{}}"));
    EXPECT_FALSE(NgxScriptSystem.UpdateAttributeForEntity("123", "score", "1"));
    EXPECT_FALSE(NgxScriptSystem.RemoveCodeComponent("123"));

    EXPECT_EQ(std::string(NgxScriptSystem.SyncCodeComponentSchema("").c_str()), "{}");
    EXPECT_EQ(std::string(NgxScriptSystem.SyncCodeComponentAttributes("", "{\"score\":1}").c_str()), "{}");
    EXPECT_FALSE(NgxScriptSystem.AddCodeComponent("", "{\"scriptAssetPath\":\"/scripts/test.js\",\"attributes\":{}}"));
    EXPECT_FALSE(NgxScriptSystem.UpdateAttributeForEntity("", "score", "1"));
    EXPECT_FALSE(NgxScriptSystem.UpdateAttributeForEntity("123", "", "1"));
    EXPECT_FALSE(NgxScriptSystem.RemoveCodeComponent(""));

    NgxScriptSystem.OnExitSpace();
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeBootstrapsAndTearsDownRegistryGlobal)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngine OfflineEngine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("codecomponent-runtime-test-space", &OfflineEngine);
    NgxCodeComponentRuntime.OnEnterSpace("codecomponent-runtime-test-space", &OfflineEngine);
    ProcessTickEvent();

    ASSERT_TRUE(NgxScriptSystem.EvaluateModuleScriptForTesting(
        "globalThis.__ngxRegistryPresent = (typeof globalThis.scriptRegistry !== 'undefined') ? 1 : 0;"));
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRegistryPresent", -1), 1);

    NgxCodeComponentRuntime.OnExitSpace();

    ASSERT_TRUE(NgxScriptSystem.EvaluateModuleScriptForTesting(
        "globalThis.__ngxRegistryRemoved = (typeof globalThis.scriptRegistry === 'undefined') ? 1 : 0;"));
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRegistryRemoved", -1), 1);

    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeSupportsDirectScriptAndRunsTeardownOnRemoval)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.RegisterStaticModuleSource("/scripts/modules/direct-teardown.js", R"(
export function script({ entityId }) {
    globalThis.__ngxDirectInitCount = (globalThis.__ngxDirectInitCount || 0) + 1;
    globalThis.__ngxLastDirectEntityId = entityId;
    return () => {
        globalThis.__ngxDirectTeardownCount = (globalThis.__ngxDirectTeardownCount || 0) + 1;
        globalThis.__ngxLastDirectTeardownEntityId = entityId;
    };
}
)");

    NgxScriptSystem.OnEnterSpace("direct-script-removal-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("direct-script-removal-space", &Engine);

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/direct-teardown.js");
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    ProcessTickEvent();

    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxDirectInitCount", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxDirectTeardownCount", 0), 0);

    Engine.RemoveEntity(&Entity);
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();

    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxDirectTeardownCount", 0), 1);

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeRunsDirectScriptTeardownOnExitSpace)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.RegisterStaticModuleSource("/scripts/modules/direct-exit-teardown.js", R"(
export function script() {
    globalThis.__ngxExitInitCount = (globalThis.__ngxExitInitCount || 0) + 1;
    return () => {
        globalThis.__ngxExitTeardownCount = (globalThis.__ngxExitTeardownCount || 0) + 1;
    };
}
)");

    NgxScriptSystem.OnEnterSpace("direct-script-exit-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("direct-script-exit-space", &Engine);

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/direct-exit-teardown.js");
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    ProcessTickEvent();

    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxExitInitCount", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxExitTeardownCount", 0), 0);

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.PumpPendingJobs();

    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxExitTeardownCount", 0), 1);

    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeSpaceComponentAttributeRoundtrip)
{
    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);

    CodeComponent->SetScriptAssetPath("/scripts/modules/hello.js");
    EXPECT_EQ(CodeComponent->GetScriptAssetPath(), "/scripts/modules/hello.js");

    CodeComponent->SetCodeScopeType(csp::multiplayer::CodeScopeType::Local);
    EXPECT_EQ(CodeComponent->GetCodeScopeType(), csp::multiplayer::CodeScopeType::Local);

    CodeComponent->SetAttribute("enabled", csp::multiplayer::CodeAttribute::FromBoolean(true));
    CodeComponent->SetAttribute("count", csp::multiplayer::CodeAttribute::FromInteger(42));
    CodeComponent->SetAttribute("ratio", csp::multiplayer::CodeAttribute::FromFloat(1.5f));
    CodeComponent->SetAttribute("label", csp::multiplayer::CodeAttribute::FromString("hello"));
    CodeComponent->SetAttribute("target", csp::multiplayer::CodeAttribute::FromEntityQuery(BuildNameEntityQuery("target")));

    csp::multiplayer::CodeAttribute EnabledAttribute;
    csp::multiplayer::CodeAttribute CountAttribute;
    csp::multiplayer::CodeAttribute RatioAttribute;
    csp::multiplayer::CodeAttribute LabelAttribute;
    csp::multiplayer::CodeAttribute TargetAttribute;

    EXPECT_TRUE(CodeComponent->GetAttribute("enabled", EnabledAttribute));
    EXPECT_TRUE(CodeComponent->GetAttribute("count", CountAttribute));
    EXPECT_TRUE(CodeComponent->GetAttribute("ratio", RatioAttribute));
    EXPECT_TRUE(CodeComponent->GetAttribute("label", LabelAttribute));
    EXPECT_TRUE(CodeComponent->GetAttribute("target", TargetAttribute));

    EXPECT_EQ(EnabledAttribute.Type, csp::multiplayer::CodePropertyType::Boolean);
    EXPECT_TRUE(EnabledAttribute.BooleanValue);
    EXPECT_EQ(CountAttribute.Type, csp::multiplayer::CodePropertyType::Integer);
    EXPECT_EQ(CountAttribute.IntegerValue, 42);
    EXPECT_EQ(RatioAttribute.Type, csp::multiplayer::CodePropertyType::Float);
    EXPECT_FLOAT_EQ(RatioAttribute.FloatValue, 1.5f);
    EXPECT_EQ(LabelAttribute.Type, csp::multiplayer::CodePropertyType::String);
    EXPECT_EQ(LabelAttribute.StringValue, "hello");
    EXPECT_EQ(TargetAttribute.Type, csp::multiplayer::CodePropertyType::EntityQuery);
    EXPECT_EQ(TargetAttribute.GetEntityQueryValue()["kind"].GetString(), "name");
    EXPECT_EQ(TargetAttribute.GetEntityQueryValue()["name"].GetString(), "target");

    EXPECT_TRUE(CodeComponent->HasAttribute("label"));
    CodeComponent->RemoveAttribute("label");
    EXPECT_FALSE(CodeComponent->HasAttribute("label"));

    CodeComponent->ClearAttributes();
    EXPECT_EQ(CodeComponent->GetAttributeKeys().Size(), 0);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeAttributeEntityQueryRoundtripSerializesAndDeserializes)
{
    const auto Query = BuildNameEntityQuery("npc-a");
    const csp::multiplayer::CodeAttribute SourceAttribute = csp::multiplayer::CodeAttribute::FromEntityQuery(Query);

    EXPECT_EQ(SourceAttribute.Type, csp::multiplayer::CodePropertyType::EntityQuery);

    csp::multiplayer::CodeAttribute ParsedAttribute;
    EXPECT_TRUE(csp::multiplayer::CodeAttribute::TryFromReplicatedValue(SourceAttribute.ToReplicatedValue(), ParsedAttribute));
    EXPECT_EQ(ParsedAttribute.Type, csp::multiplayer::CodePropertyType::EntityQuery);
    EXPECT_EQ(ParsedAttribute.GetEntityQueryValue()["kind"].GetString(), "name");
    EXPECT_EQ(ParsedAttribute.GetEntityQueryValue()["name"].GetString(), "npc-a");
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeAttributeEntityQueryRejectsInvalidPayload)
{
    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> InvalidEntityQuery;
    InvalidEntityQuery["kind"] = "id";
    InvalidEntityQuery["id"] = "invalid-id";

    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> InvalidSerializedAttribute;
    InvalidSerializedAttribute["type"] = static_cast<int64_t>(csp::multiplayer::CodePropertyType::EntityQuery);
    InvalidSerializedAttribute["entityQueryValue"] = InvalidEntityQuery;

    csp::multiplayer::CodeAttribute ParsedAttribute;
    EXPECT_FALSE(csp::multiplayer::CodeAttribute::TryFromReplicatedValue(
        csp::common::ReplicatedValue(InvalidSerializedAttribute), ParsedAttribute));
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeAttributeImageAssetRoundtripSerializesAndDeserializes)
{
    const auto ImageAsset = BuildImageAssetValue("museum-assets", "hero");
    const csp::multiplayer::CodeAttribute SourceAttribute = csp::multiplayer::CodeAttribute::FromImageAsset(ImageAsset);

    EXPECT_EQ(SourceAttribute.Type, csp::multiplayer::CodePropertyType::ImageAsset);

    csp::multiplayer::CodeAttribute ParsedAttribute;
    EXPECT_TRUE(csp::multiplayer::CodeAttribute::TryFromReplicatedValue(SourceAttribute.ToReplicatedValue(), ParsedAttribute));
    EXPECT_EQ(ParsedAttribute.Type, csp::multiplayer::CodePropertyType::ImageAsset);
    EXPECT_EQ(ParsedAttribute.GetImageAssetValue()["assetCollectionId"].GetString(), "museum-assets");
    EXPECT_EQ(ParsedAttribute.GetImageAssetValue()["imageAssetId"].GetString(), "hero");
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeAttributeImageAssetRejectsInvalidPayload)
{
    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> InvalidImageAsset;
    InvalidImageAsset["assetCollectionId"] = "museum-assets";
    InvalidImageAsset["assetId"] = "hero";

    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> InvalidSerializedAttribute;
    InvalidSerializedAttribute["type"] = static_cast<int64_t>(csp::multiplayer::CodePropertyType::ImageAsset);
    InvalidSerializedAttribute["imageAssetValue"] = InvalidImageAsset;

    csp::multiplayer::CodeAttribute ParsedAttribute;
    EXPECT_FALSE(csp::multiplayer::CodeAttribute::TryFromReplicatedValue(
        csp::common::ReplicatedValue(InvalidSerializedAttribute), ParsedAttribute));
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeSpaceComponentScriptAttributesExposeImageAssetDefaults)
{
    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);

    CodeComponent->SetSchema(R"({
        "poster": { "type": "imageAsset", "default": { "assetCollectionId": "museum-assets", "imageAssetId": "hero" } },
        "thumbnail": { "type": "imageAsset" }
    })");

    const auto Attributes = CodeComponent->GetScriptAttributes();
    const auto* Poster = FindScriptAttributeByName(Attributes, "poster");
    const auto* Thumbnail = FindScriptAttributeByName(Attributes, "thumbnail");

    ASSERT_NE(Poster, nullptr);
    ASSERT_NE(Thumbnail, nullptr);
    EXPECT_EQ(Poster->Type, csp::multiplayer::ScriptAttributeType::ImageAsset);
    EXPECT_EQ(Poster->Value.GetReplicatedValueType(), csp::common::ReplicatedValueType::StringMap);
    EXPECT_EQ(Poster->Value.GetStringMap()["assetCollectionId"].GetString(), "museum-assets");
    EXPECT_EQ(Poster->Value.GetStringMap()["imageAssetId"].GetString(), "hero");

    EXPECT_EQ(Thumbnail->Type, csp::multiplayer::ScriptAttributeType::ImageAsset);
    EXPECT_EQ(Thumbnail->Value.GetReplicatedValueType(), csp::common::ReplicatedValueType::StringMap);
    EXPECT_EQ(Thumbnail->Value.GetStringMap()["assetCollectionId"].GetString(), "");
    EXPECT_EQ(Thumbnail->Value.GetStringMap()["imageAssetId"].GetString(), "");
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, RuntimeMaterialSystemTexturePatchSupportsAllSlotsAndChangedCallback)
{
    csp::common::LogSystem LogSystem;
    csp::systems::RuntimeMaterialSystem RuntimeMaterialSystem(LogSystem);

    csp::systems::GLTFMaterial Material("CRT", "materials", "crt-material");
    Material.SetBaseColorTexture(csp::systems::TextureInfo("collection-a", "base"));
    Material.SetMetallicRoughnessTexture(csp::systems::TextureInfo("collection-a", "metallic"));
    Material.SetNormalTexture(csp::systems::TextureInfo("collection-a", "normal"));
    Material.SetOcclusionTexture(csp::systems::TextureInfo("collection-a", "occlusion"));
    Material.SetEmissiveTexture(csp::systems::TextureInfo("collection-a", "emissive"));
    RuntimeMaterialSystem.SetGLTFForTesting("crt-material", "materials", Material);

    int32_t ChangedCount = 0;
    csp::common::String LastMaterialId;
    RuntimeMaterialSystem.SetChangedCallback(
        [&ChangedCount, &LastMaterialId](const csp::common::String& MaterialId, uint64_t, int32_t, int32_t, const csp::common::String&)
        {
            ++ChangedCount;
            LastMaterialId = MaterialId;
        });

    const auto State = RuntimeMaterialSystem.Resolve("crt-material");
    ASSERT_EQ(State.Status, csp::systems::RuntimeMaterialStatus::Resolved);

    struct SlotUpdate
    {
        csp::systems::RuntimeMaterialTextureSlot Slot;
        const char* AssetId;
    };

    const SlotUpdate Updates[] = {
        { csp::systems::RuntimeMaterialTextureSlot::BaseColor, "base-updated" },
        { csp::systems::RuntimeMaterialTextureSlot::MetallicRoughness, "metallic-updated" },
        { csp::systems::RuntimeMaterialTextureSlot::Normal, "normal-updated" },
        { csp::systems::RuntimeMaterialTextureSlot::Occlusion, "occlusion-updated" },
        { csp::systems::RuntimeMaterialTextureSlot::Emissive, "emissive-updated" },
    };

    for (const auto& Update : Updates)
    {
        csp::systems::RuntimeMaterialTexturePatch Patch;
        Patch.HasIsSet = true;
        Patch.IsSet = true;
        Patch.HasSourceType = true;
        Patch.SourceType = static_cast<int32_t>(csp::systems::ETextureResourceType::ImageAsset);
        Patch.HasAssetCollectionId = true;
        Patch.AssetCollectionId = "collection-b";
        Patch.HasAssetId = true;
        Patch.AssetId = Update.AssetId;
        Patch.HasUVOffset = true;
        Patch.UVOffset = csp::common::Vector2(0.25f, 0.5f);
        Patch.HasUVScale = true;
        Patch.UVScale = csp::common::Vector2(1.5f, 2.0f);
        Patch.HasUVRotation = true;
        Patch.UVRotation = 0.75f;
        Patch.HasTexCoord = true;
        Patch.TexCoord = 2;
        EXPECT_TRUE(RuntimeMaterialSystem.PatchTextureHandle(State.Handle, Update.Slot, Patch));
    }

    auto* LiveMaterial = static_cast<csp::systems::GLTFMaterial*>(RuntimeMaterialSystem.Get("crt-material"));
    ASSERT_NE(LiveMaterial, nullptr);
    EXPECT_EQ(LiveMaterial->GetBaseColorTexture().GetAssetId(), "base-updated");
    EXPECT_EQ(LiveMaterial->GetMetallicRoughnessTexture().GetAssetId(), "metallic-updated");
    EXPECT_EQ(LiveMaterial->GetNormalTexture().GetAssetId(), "normal-updated");
    EXPECT_EQ(LiveMaterial->GetOcclusionTexture().GetAssetId(), "occlusion-updated");
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetAssetId(), "emissive-updated");
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetUVOffset(), csp::common::Vector2(0.25f, 0.5f));
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetUVScale(), csp::common::Vector2(1.5f, 2.0f));
    EXPECT_FLOAT_EQ(LiveMaterial->GetEmissiveTexture().GetUVRotation(), 0.75f);
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetTexCoord(), 2);
    EXPECT_EQ(ChangedCount, static_cast<int32_t>(sizeof(Updates) / sizeof(Updates[0])));
    EXPECT_EQ(LastMaterialId, "crt-material");

    EXPECT_TRUE(RuntimeMaterialSystem.ResetHandle(State.Handle));
    LiveMaterial = static_cast<csp::systems::GLTFMaterial*>(RuntimeMaterialSystem.Get("crt-material"));
    ASSERT_NE(LiveMaterial, nullptr);
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetAssetId(), "emissive");
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetUVOffset(), csp::common::Vector2::Zero());
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetUVScale(), csp::common::Vector2::One());
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, RuntimeMaterialTextureScriptInterfacePatchesBindingTexturesAndResets)
{
    csp::common::LogSystem LogSystem;
    csp::systems::RuntimeMaterialSystem RuntimeMaterialSystem(LogSystem);

    csp::systems::GLTFMaterial Material("CRT", "materials", "crt-binding");
    csp::systems::TextureInfo EmissiveTexture("collection-a", "emissive-a");
    EmissiveTexture.SetUVOffset(csp::common::Vector2(0.1f, 0.2f));
    Material.SetEmissiveTexture(EmissiveTexture);
    RuntimeMaterialSystem.SetGLTFForTesting("crt-binding", "materials", Material);

    csp::multiplayer::RuntimeMaterialScriptInterface MaterialInterface(
        &RuntimeMaterialSystem, 1001, csp::multiplayer::ComponentType::StaticModel, 0, "Screen", "crt-binding");

    auto EmissiveTextureInterface = MaterialInterface.GetEmissiveTexture();
    ASSERT_NE(EmissiveTextureInterface, nullptr);
    EXPECT_EQ(MaterialInterface.GetColorTexture(), nullptr);
    EXPECT_EQ(EmissiveTextureInterface->GetAssetCollectionId(), "collection-a");
    EXPECT_EQ(EmissiveTextureInterface->GetAssetId(), "emissive-a");
    const auto InitialUVOffset = EmissiveTextureInterface->GetUVOffset();
    ASSERT_EQ(InitialUVOffset.size(), 2u);
    EXPECT_FLOAT_EQ(InitialUVOffset[0], 0.1f);
    EXPECT_FLOAT_EQ(InitialUVOffset[1], 0.2f);

    EXPECT_TRUE(EmissiveTextureInterface->SetAssetSource("collection-b", "emissive-b"));
    EmissiveTextureInterface->SetUVOffset({ 0.3f, 0.4f });
    EmissiveTextureInterface->SetUVScale({ 1.25f, 0.75f });
    EmissiveTextureInterface->SetUVRotation(1.25f);
    EmissiveTextureInterface->SetTexCoord(3);
    EmissiveTextureInterface->SetStereoVideoType(1);
    EmissiveTextureInterface->SetIsStereoFlipped(true);

    auto BindingState = RuntimeMaterialSystem.ResolveForBinding(1001, csp::multiplayer::ComponentType::StaticModel, 0, "Screen", "crt-binding");
    auto* LiveMaterial = static_cast<csp::systems::GLTFMaterial*>(BindingState.MaterialRef);
    ASSERT_NE(LiveMaterial, nullptr);
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetAssetCollectionId(), "collection-b");
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetAssetId(), "emissive-b");
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetUVOffset(), csp::common::Vector2(0.3f, 0.4f));
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetUVScale(), csp::common::Vector2(1.25f, 0.75f));
    EXPECT_FLOAT_EQ(LiveMaterial->GetEmissiveTexture().GetUVRotation(), 1.25f);
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetTexCoord(), 3);
    EXPECT_EQ(static_cast<int32_t>(LiveMaterial->GetEmissiveTexture().GetStereoVideoType()), 1);
    EXPECT_TRUE(LiveMaterial->GetEmissiveTexture().GetIsStereoFlipped());

    EXPECT_TRUE(EmissiveTextureInterface->SetComponentSource("1001-45"));
    BindingState = RuntimeMaterialSystem.ResolveForBinding(1001, csp::multiplayer::ComponentType::StaticModel, 0, "Screen", "crt-binding");
    LiveMaterial = static_cast<csp::systems::GLTFMaterial*>(BindingState.MaterialRef);
    ASSERT_NE(LiveMaterial, nullptr);
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetEntityComponentId(), "1001-45");
    EXPECT_EQ(static_cast<int32_t>(LiveMaterial->GetEmissiveTexture().GetSourceType()),
        static_cast<int32_t>(csp::systems::ETextureResourceType::Component));

    EXPECT_TRUE(EmissiveTextureInterface->Clear());
    EXPECT_FALSE(EmissiveTextureInterface->GetIsSet());

    EXPECT_TRUE(MaterialInterface.Reset());
    BindingState = RuntimeMaterialSystem.ResolveForBinding(1001, csp::multiplayer::ComponentType::StaticModel, 0, "Screen", "crt-binding");
    LiveMaterial = static_cast<csp::systems::GLTFMaterial*>(BindingState.MaterialRef);
    ASSERT_NE(LiveMaterial, nullptr);
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetAssetCollectionId(), "collection-a");
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetAssetId(), "emissive-a");
    EXPECT_EQ(LiveMaterial->GetEmissiveTexture().GetUVOffset(), csp::common::Vector2(0.1f, 0.2f));
    EXPECT_TRUE(LiveMaterial->GetEmissiveTexture().IsSet());
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, RuntimeMaterialTextureScriptInterfaceExposesAlphaVideoColorTextureOnly)
{
    csp::common::LogSystem LogSystem;
    csp::systems::RuntimeMaterialSystem RuntimeMaterialSystem(LogSystem);

    csp::systems::AlphaVideoMaterial Material("Billboard", "materials", "alpha-video");
    Material.SetColorTexture(csp::systems::TextureInfo("collection-a", "frame-a"));
    RuntimeMaterialSystem.SetAlphaVideoForTesting("alpha-video", "materials", Material);

    csp::multiplayer::RuntimeMaterialScriptInterface MaterialInterface(&RuntimeMaterialSystem, "alpha-video");
    EXPECT_EQ(MaterialInterface.GetEmissiveTexture(), nullptr);

    auto ColorTextureInterface = MaterialInterface.GetColorTexture();
    ASSERT_NE(ColorTextureInterface, nullptr);
    EXPECT_EQ(ColorTextureInterface->GetAssetCollectionId(), "collection-a");
    EXPECT_EQ(ColorTextureInterface->GetAssetId(), "frame-a");

    EXPECT_TRUE(ColorTextureInterface->SetComponentSource("alpha-component"));
    auto* LiveMaterial = static_cast<csp::systems::AlphaVideoMaterial*>(RuntimeMaterialSystem.Get("alpha-video"));
    ASSERT_NE(LiveMaterial, nullptr);
    EXPECT_EQ(LiveMaterial->GetColorTexture().GetEntityComponentId(), "alpha-component");
    EXPECT_EQ(static_cast<int32_t>(LiveMaterial->GetColorTexture().GetSourceType()),
        static_cast<int32_t>(csp::systems::ETextureResourceType::Component));
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, RuntimeMaterialTextureScriptInterfaceCachesPerSlotOnRuntimeMaterial)
{
    csp::common::LogSystem LogSystem;
    csp::systems::RuntimeMaterialSystem RuntimeMaterialSystem(LogSystem);

    csp::systems::GLTFMaterial Material;
    Material.SetName("CRT Cache Test");
    Material.SetMaterialId("crt-cache");
    RuntimeMaterialSystem.SetGLTFForTesting("crt-cache", "materials", Material);

    csp::multiplayer::RuntimeMaterialScriptInterface MaterialInterface(&RuntimeMaterialSystem, "crt-cache");

    auto FirstEmissiveTexture = MaterialInterface.GetEmissiveTexture();
    auto SecondEmissiveTexture = MaterialInterface.GetEmissiveTexture();
    auto FirstBaseColorTexture = MaterialInterface.GetBaseColorTexture();
    auto SecondBaseColorTexture = MaterialInterface.GetBaseColorTexture();

    ASSERT_NE(FirstEmissiveTexture, nullptr);
    ASSERT_NE(FirstBaseColorTexture, nullptr);
    EXPECT_EQ(FirstEmissiveTexture.get(), SecondEmissiveTexture.get());
    EXPECT_EQ(FirstBaseColorTexture.get(), SecondBaseColorTexture.get());
    EXPECT_NE(FirstEmissiveTexture.get(), FirstBaseColorTexture.get());
    EXPECT_EQ(MaterialInterface.GetColorTexture(), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, SpaceEntityTagsNormalizeAndDeduplicate)
{
    csp::multiplayer::SpaceEntity Entity;

    csp::common::Array<csp::common::String> Tags(4);
    Tags[0] = " Coin ";
    Tags[1] = "collectable";
    Tags[2] = "coin";
    Tags[3] = "COLLECTABLE";

    EXPECT_TRUE(Entity.SetTags(Tags));
    EXPECT_TRUE(Entity.HasTag("coin"));
    EXPECT_TRUE(Entity.HasTag("COLLECTABLE"));
    EXPECT_FALSE(Entity.HasTag("missing"));

    const auto EntityTags = Entity.GetTags();
    ASSERT_EQ(EntityTags.Size(), 2);
    EXPECT_EQ(EntityTags[0], "coin");
    EXPECT_EQ(EntityTags[1], "collectable");

    EXPECT_TRUE(Entity.AddTag(" Rare "));
    EXPECT_TRUE(Entity.HasTag("rare"));
    EXPECT_TRUE(Entity.RemoveTag("RARE"));
    EXPECT_FALSE(Entity.HasTag("rare"));
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, EntityQueryResolutionSupportsTagsAndBooleanComposition)
{
    csp::common::LogSystem LogSystem;
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);
    TestScriptRunner ScriptRunner;

    csp::multiplayer::SpaceEntity CoinCollectable(
        &Engine, ScriptRunner, &LogSystem, csp::multiplayer::SpaceEntityType::Object, 1001, "coin-a", csp::multiplayer::SpaceTransform {},
        1, csp::common::Optional<uint64_t> {}, true, true);
    csp::multiplayer::SpaceEntity CoinOnly(
        &Engine, ScriptRunner, &LogSystem, csp::multiplayer::SpaceEntityType::Object, 1002, "coin-b", csp::multiplayer::SpaceTransform {},
        1, csp::common::Optional<uint64_t> {}, true, true);
    csp::multiplayer::SpaceEntity Decorative(
        &Engine, ScriptRunner, &LogSystem, csp::multiplayer::SpaceEntityType::Object, 1003, "decor", csp::multiplayer::SpaceTransform {},
        1, csp::common::Optional<uint64_t> {}, true, true);

    csp::common::Array<csp::common::String> CoinCollectableTags(2);
    CoinCollectableTags[0] = "coin";
    CoinCollectableTags[1] = "collectable";
    EXPECT_TRUE(CoinCollectable.SetTags(CoinCollectableTags));

    csp::common::Array<csp::common::String> CoinOnlyTags(1);
    CoinOnlyTags[0] = "coin";
    EXPECT_TRUE(CoinOnly.SetTags(CoinOnlyTags));

    csp::common::Array<csp::common::String> DecorativeTags(1);
    DecorativeTags[0] = "decor";
    EXPECT_TRUE(Decorative.SetTags(DecorativeTags));

    std::vector<csp::multiplayer::SpaceEntity*> Entities { &CoinCollectable, &CoinOnly, &Decorative };

    EXPECT_EQ(csp::multiplayer::ResolveEntityIdsFromQueryJson(BuildTagQueryJson("CoIn"), Entities), (std::set<uint64_t> { 1001, 1002 }));
    EXPECT_EQ(csp::multiplayer::ResolveEntityIdsFromQueryJson(BuildAndTagQueryJson("coin", "collectable"), Entities), (std::set<uint64_t> { 1001 }));
    EXPECT_EQ(csp::multiplayer::ResolveEntityIdsFromQueryJson(BuildNotTagQueryJson("coin"), Entities), (std::set<uint64_t> { 1003 }));
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, SingleEntityQueryResolutionContractRemainsUnchangedForTags)
{
    csp::common::LogSystem LogSystem;
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);
    TestScriptRunner ScriptRunner;

    csp::multiplayer::SpaceEntity FirstMatch(
        &Engine, ScriptRunner, &LogSystem, csp::multiplayer::SpaceEntityType::Object, 2001, "first", csp::multiplayer::SpaceTransform {},
        1, csp::common::Optional<uint64_t> {}, true, true);
    csp::multiplayer::SpaceEntity SecondMatch(
        &Engine, ScriptRunner, &LogSystem, csp::multiplayer::SpaceEntityType::Object, 2002, "second", csp::multiplayer::SpaceTransform {},
        1, csp::common::Optional<uint64_t> {}, true, true);

    csp::common::Array<csp::common::String> Tags(1);
    Tags[0] = "collectable";
    EXPECT_TRUE(FirstMatch.SetTags(Tags));
    EXPECT_TRUE(SecondMatch.SetTags(Tags));

    std::vector<csp::multiplayer::SpaceEntity*> Entities { &FirstMatch, &SecondMatch };

    const auto ResolvedId = csp::multiplayer::ResolveEntityIdFromQueryJson(BuildTagQueryJson("collectable"), Entities);
    ASSERT_TRUE(ResolvedId.has_value());
    EXPECT_EQ(*ResolvedId, 2001);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, RegistryBootstrapUsesAssetModuleWhenAvailable)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngine OfflineEngine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.RegisterStaticModuleSource("/scripts/engine/registry.js", R"(
export function createScriptRegistry() {
    globalThis.__ngxAssetRegistryBootstrapUsed = 1;
    return {
        addCodeComponent() {},
        updateAttributeForEntity() {},
        removeCodeComponent() {},
        tick() {},
        destroy() {},
    };
}
)");

    NgxScriptSystem.OnEnterSpace("asset-registry-space", &OfflineEngine);
    NgxCodeComponentRuntime.OnEnterSpace("asset-registry-space", &OfflineEngine);
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();

    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxAssetRegistryBootstrapUsed", 0), 1);

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, RegistryBootstrapFallsBackToBuiltInWhenAssetMissing)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngine OfflineEngine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("fallback-registry-space", &OfflineEngine);
    NgxCodeComponentRuntime.OnEnterSpace("fallback-registry-space", &OfflineEngine);
    ProcessTickEvent();

    ASSERT_TRUE(NgxScriptSystem.EvaluateModuleScriptForTesting(
        "globalThis.__ngxFallbackRegistryPresent = (typeof globalThis.scriptRegistry !== 'undefined') ? 1 : 0;"));
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxFallbackRegistryPresent", -1), 1);

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeAutoSyncWorksOffline)
{
    RunRuntimeSyncSmokeScenario(csp::common::RealtimeEngineType::Offline);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeAutoSyncWorksOnline)
{
    RunRuntimeSyncSmokeScenario(csp::common::RealtimeEngineType::Online);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentFallbackSchemaSyncAppliesDefaultsAndRejectsInvalidTypes)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("schema-sync-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("schema-sync-space", &Engine);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/schema-test.js", R"(
import { useEffect } from '/__csp/internal/codecomponent/registry.js';

export const schema = {
    speed: { type: 'float', default: 1.5 },
    enabled: { type: 'boolean', default: true },
    label: { type: 'string', default: 'player' },
};

export function script() {
    return ({ attributes }) => {
        useEffect(() => {
            globalThis.__ngxSchemaSpeedScaled = Math.round(attributes.speed.value * 100);
            globalThis.__ngxSchemaEnabled = attributes.enabled.value ? 1 : 0;
            globalThis.__ngxSchemaLabelLength = attributes.label.value.length;
        });
    };
}
)");

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/schema-test.js");
    CodeComponent->SetAttribute("speed", csp::multiplayer::CodeAttribute::FromString("3.25"));
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();

    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxSchemaSpeedScaled", -1), 150);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxSchemaEnabled", -1), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxSchemaLabelLength", -1), 6);

    Engine.RemoveEntity(&Entity);
    ProcessTickEvent();
    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentFallbackSchemaSupportsImageAssetDefaultsAndIncrementalValidation)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("image-asset-schema-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("image-asset-schema-space", &Engine);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/image-asset-schema-test.js", R"(
import { useEffect } from '/__csp/internal/codecomponent/registry.js';

export const schema = {
    poster: { type: 'imageAsset', default: { assetCollectionId: 'museum-assets', imageAssetId: 'hero' } },
};

export function script() {
    return ({ attributes }) => {
        useEffect(() => {
            const poster = attributes.poster.value || {};
            const signature = `${poster.assetCollectionId || ''}:${poster.imageAssetId || ''}`;
            globalThis.__ngxImageAssetObservedCount = (globalThis.__ngxImageAssetObservedCount || 0) + 1;
            globalThis.__ngxImageAssetIsDefault = signature === 'museum-assets:hero' ? 1 : 0;
            globalThis.__ngxImageAssetIsGallery = signature === 'gallery-assets:screen-a' ? 1 : 0;
        });
    };
}
)");

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/image-asset-schema-test.js");
    CodeComponent->SetAttribute("poster", csp::multiplayer::CodeAttribute::FromString("not-an-image-asset"));
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();

    const std::string EntityId = std::to_string(Entity.GetId());
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxImageAssetIsDefault", 0), 1);
    EXPECT_GE(NgxScriptSystem.GetGlobalIntForTesting("__ngxImageAssetObservedCount", 0), 1);

    EXPECT_TRUE(NgxScriptSystem.UpdateAttributeForEntity(
        EntityId.c_str(), "poster", R"({"assetCollectionId":"gallery-assets","imageAssetId":"screen-a"})"));
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxImageAssetIsGallery", 0), 1);

    EXPECT_TRUE(NgxScriptSystem.UpdateAttributeForEntity(EntityId.c_str(), "poster", R"({"assetCollectionId":"broken-assets"})"));
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxImageAssetIsGallery", 0), 1);

    Engine.RemoveEntity(&Entity);
    ProcessTickEvent();
    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeSyncsNativeImageAssetAttributesIntoJS)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("image-asset-native-sync-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("image-asset-native-sync-space", &Engine);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/image-asset-native-sync-test.js", R"(
import { useEffect } from '/__csp/internal/codecomponent/registry.js';

export const schema = {
    poster: { type: 'imageAsset' },
};

export function script() {
    return ({ attributes }) => {
        useEffect(() => {
            const poster = attributes.poster.value || {};
            globalThis.__ngxNativeImageAssetCollectionMatches = poster.assetCollectionId === 'gallery-assets' ? 1 : 0;
            globalThis.__ngxNativeImageAssetIdMatches = poster.imageAssetId === 'screen-b' ? 1 : 0;
        });
    };
}
)");

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/image-asset-native-sync-test.js");
    CodeComponent->SetAttribute("poster", csp::multiplayer::CodeAttribute::FromImageAsset(BuildImageAssetValue("gallery-assets", "screen-b")));
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();

    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxNativeImageAssetCollectionMatches", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxNativeImageAssetIdMatches", 0), 1);

    Engine.RemoveEntity(&Entity);
    ProcessTickEvent();
    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentEntityReferenceResolvesWritesAndHandlesDeletion)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("entity-ref-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("entity-ref-space", &Engine);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/entity-ref-test.js", R"(
import { useEffect } from '/__csp/internal/codecomponent/registry.js';

export const schema = {
    target: { type: 'entity' },
};

export function script() {
    return ({ attributes }) => {
        useEffect(() => {
            const ref = attributes.target.value;
            globalThis.__ngxEntityRefInitResolved = ref.status === 'resolved' ? 1 : 0;
            globalThis.__ngxEntityRefInitHasId = ref.id !== null ? 1 : 0;
            const lightComponent = ref.getLightComponent();
            globalThis.__ngxEntityRefLightResolved = lightComponent.status === 'resolved' ? 1 : 0;

            globalThis.__ngxEntityRefTickStatusResolved = ref.status === 'resolved' ? 1 : 0;
            globalThis.__ngxEntityRefTickStatusDeleted = ref.status === 'deleted' ? 1 : 0;

            if (!globalThis.__ngxEntityRefDidWrite) {
                globalThis.__ngxEntityRefDidWrite = ref.setPosition([7, 8, 9]) ? 1 : 0;
            }
        });
    };
}
)");

    csp::multiplayer::SpaceEntity TargetEntity;
    EXPECT_TRUE(TargetEntity.SetName("target-entity"));
    EXPECT_NE(TargetEntity.AddComponent(csp::multiplayer::ComponentType::Light), nullptr);
    Engine.AddEntity(&TargetEntity);

    csp::multiplayer::SpaceEntity ScriptEntity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(ScriptEntity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/entity-ref-test.js");
    CodeComponent->SetAttribute("target", csp::multiplayer::CodeAttribute::FromEntityQuery(BuildNameEntityQuery("target-entity")));
    Engine.AddEntity(&ScriptEntity);

    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();

    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxEntityRefInitResolved", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxEntityRefInitHasId", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxEntityRefLightResolved", 0), 1);

    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxEntityRefTickStatusResolved", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxEntityRefDidWrite", 0), 1);
    EXPECT_EQ(TargetEntity.GetPosition(), csp::common::Vector3(7.0f, 8.0f, 9.0f));

    Engine.RemoveEntity(&TargetEntity);
    Engine.RemoveEntity(&ScriptEntity);
    ProcessTickEvent();

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentFallbackLifecycleIsStableAndIncremental)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("lifecycle-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("lifecycle-space", &Engine);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/lifecycle-test.js", R"(
import { useEffect } from '/__csp/internal/codecomponent/registry.js';

export function script() {
    return ({ attributes }) => {
        globalThis.__ngxLifecycleInitCalls = (globalThis.__ngxLifecycleInitCalls || 0) + 1;
        globalThis.__ngxLifecycleLastScore = attributes.score.value;

        useEffect(() => {
            globalThis.__ngxLifecycleUpdateCalls = (globalThis.__ngxLifecycleUpdateCalls || 0) + 1;
            globalThis.__ngxLifecycleLastScore = attributes.score.value;
        });
    };
}
)");

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/lifecycle-test.js");
    CodeComponent->SetAttribute("score", csp::multiplayer::CodeAttribute::FromInteger(1));
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();

    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxLifecycleInitCalls", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxLifecycleUpdateCalls", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxLifecycleLastScore", -1), 1);

    CodeComponent->SetAttribute("score", csp::multiplayer::CodeAttribute::FromInteger(2));
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();

    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxLifecycleInitCalls", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxLifecycleUpdateCalls", 0), 2);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxLifecycleLastScore", -1), 2);

    Engine.RemoveEntity(&Entity);
    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, ScriptWithoutSchemaStillExecutesThroughFallbackRegistry)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("no-schema-script-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("no-schema-script-space", &Engine);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/no-schema-test.js", R"(
import { useEffect } from '/__csp/internal/codecomponent/registry.js';

export function script() {
    return ({ attributes }) => {
        globalThis.__ngxNoSchemaInitCalls = (globalThis.__ngxNoSchemaInitCalls || 0) + 1;
        globalThis.__ngxNoSchemaValue = attributes.value.value;

        useEffect(() => {
            globalThis.__ngxNoSchemaValue = attributes.value.value;
        });
    };
}
)");

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/no-schema-test.js");
    CodeComponent->SetAttribute("value", csp::multiplayer::CodeAttribute::FromInteger(42));
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();

    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxNoSchemaInitCalls", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxNoSchemaValue", -1), 42);

    Engine.RemoveEntity(&Entity);
    ProcessTickEvent();
    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, LocalCodeComponentOnlyRunsWhenSelectedInEditMode)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);
    TestScriptRunner ScriptRunner;

    NgxScriptSystem.OnEnterSpace("selection-edit-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("selection-edit-space", &Engine);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/selection-test.js", R"(
import { useEffect } from '/__csp/internal/codecomponent/registry.js';

export function script() {
    return () => {
        globalThis.__ngxSelectionInitCalls = (globalThis.__ngxSelectionInitCalls || 0) + 1;
        useEffect(() => {
            globalThis.__ngxSelectionEffectCalls = (globalThis.__ngxSelectionEffectCalls || 0) + 1;
        });
    };
}
)");

    csp::multiplayer::SpaceEntity Entity(
        &Engine, ScriptRunner, &LogSystem, csp::multiplayer::SpaceEntityType::Object, 3001, "selection-target", csp::multiplayer::SpaceTransform {},
        csp::common::LocalClientID, csp::common::Optional<uint64_t> {}, true, true);
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/selection-test.js");
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxSelectionInitCalls", 0), 0);

    EXPECT_TRUE(Entity.Select());
    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxSelectionInitCalls", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxSelectionEffectCalls", 0), 1);

    EXPECT_TRUE(Entity.Deselect());
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();

    EXPECT_TRUE(Entity.Select());
    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxSelectionInitCalls", 0), 2);

    Engine.RemoveEntity(&Entity);
    ProcessTickEvent();
    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, EditorCodeComponentOnlyRunsInEditAndLocalRunsInPlay)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("runtime-mode-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("runtime-mode-space", &Engine);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/runtime-local.js", R"(
export function script() {
    return () => {
        globalThis.__ngxRuntimeLocalInitCalls = (globalThis.__ngxRuntimeLocalInitCalls || 0) + 1;
    };
}
)");

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/runtime-editor.js", R"(
export function script() {
    return () => {
        globalThis.__ngxRuntimeEditorInitCalls = (globalThis.__ngxRuntimeEditorInitCalls || 0) + 1;
    };
}
)");

    csp::multiplayer::SpaceEntity LocalEntity;
    auto* LocalCodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(LocalEntity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(LocalCodeComponent, nullptr);
    LocalCodeComponent->SetScriptAssetPath("/scripts/modules/runtime-local.js");

    csp::multiplayer::SpaceEntity EditorEntity;
    auto* EditorCodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(EditorEntity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(EditorCodeComponent, nullptr);
    EditorCodeComponent->SetScriptAssetPath("/scripts/modules/runtime-editor.js");
    EditorCodeComponent->SetCodeScopeType(csp::multiplayer::CodeScopeType::Editor);

    Engine.AddEntity(&LocalEntity);
    Engine.AddEntity(&EditorEntity);

    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRuntimeLocalInitCalls", 0), 0);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRuntimeEditorInitCalls", 0), 1);

    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRuntimeLocalInitCalls", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRuntimeEditorInitCalls", 0), 1);

    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Server);
    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRuntimeLocalInitCalls", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRuntimeEditorInitCalls", 0), 1);

    Engine.RemoveEntity(&LocalEntity);
    Engine.RemoveEntity(&EditorEntity);
    ProcessTickEvent();
    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, LocalCodeComponentRunsWhenAncestorIsSelectedInEditMode)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);
    TestScriptRunner ScriptRunner;

    NgxScriptSystem.OnEnterSpace("ancestor-selection-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("ancestor-selection-space", &Engine);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/ancestor-selection-test.js", R"(
export function script() {
    return () => {
        globalThis.__ngxAncestorSelectionInitCalls = (globalThis.__ngxAncestorSelectionInitCalls || 0) + 1;
    };
}
)");

    csp::multiplayer::SpaceEntity ParentEntity(
        &Engine, ScriptRunner, &LogSystem, csp::multiplayer::SpaceEntityType::Object, 3010, "selection-parent", csp::multiplayer::SpaceTransform {},
        csp::common::LocalClientID, csp::common::Optional<uint64_t> {}, true, true);
    csp::multiplayer::SpaceEntity ChildEntity(
        &Engine, ScriptRunner, &LogSystem, csp::multiplayer::SpaceEntityType::Object, 3011, "selection-child", csp::multiplayer::SpaceTransform {},
        csp::common::LocalClientID, csp::common::Optional<uint64_t> { 3010 }, true, true);
    auto* ChildCodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(ChildEntity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(ChildCodeComponent, nullptr);
    ChildCodeComponent->SetScriptAssetPath("/scripts/modules/ancestor-selection-test.js");

    Engine.AddEntity(&ParentEntity);
    Engine.AddEntity(&ChildEntity);
    ChildEntity.ResolveParentChildRelationship();

    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxAncestorSelectionInitCalls", 0), 0);

    EXPECT_TRUE(ParentEntity.Select());
    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxAncestorSelectionInitCalls", 0), 1);

    Engine.RemoveEntity(&ChildEntity);
    Engine.RemoveEntity(&ParentEntity);
    ProcessTickEvent();
    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, LocalCodeComponentRecreatesWhenRuntimeModeChangesWhileActive)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);
    TestScriptRunner ScriptRunner;

    NgxScriptSystem.OnEnterSpace("mode-restart-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("mode-restart-space", &Engine);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/mode-restart-test.js", R"(
export function script() {
    return () => {
        globalThis.__ngxModeRestartInitCalls = (globalThis.__ngxModeRestartInitCalls || 0) + 1;
    };
}
)");

    csp::multiplayer::SpaceEntity Entity(
        &Engine, ScriptRunner, &LogSystem, csp::multiplayer::SpaceEntityType::Object, 3020, "mode-restart-target", csp::multiplayer::SpaceTransform {},
        csp::common::LocalClientID, csp::common::Optional<uint64_t> {}, true, true);
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/mode-restart-test.js");
    Engine.AddEntity(&Entity);

    EXPECT_TRUE(Entity.Select());
    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxModeRestartInitCalls", 0), 1);

    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxModeRestartInitCalls", 0), 2);

    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxModeRestartInitCalls", 0), 3);

    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxModeRestartInitCalls", 0), 4);

    Engine.RemoveEntity(&Entity);
    ProcessTickEvent();
    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, ServerCodeComponentRunsOnlyInServerRuntimeMode)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);
    TestScriptRunner ScriptRunner;

    NgxScriptSystem.OnEnterSpace("server-scope-runtime-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("server-scope-runtime-space", &Engine);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/runtime-server.js", R"(
export function script() {
    return () => {
        globalThis.__ngxRuntimeServerInitCalls = (globalThis.__ngxRuntimeServerInitCalls || 0) + 1;
    };
}
)");

    csp::multiplayer::SpaceEntity ServerEntity;
    auto* ServerCodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(ServerEntity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(ServerCodeComponent, nullptr);
    ServerCodeComponent->SetScriptAssetPath("/scripts/modules/runtime-server.js");
    ServerCodeComponent->SetCodeScopeType(csp::multiplayer::CodeScopeType::Server);

    Engine.AddEntity(&ServerEntity);

    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRuntimeServerInitCalls", 0), 0);

    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Server);
    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRuntimeServerInitCalls", 0), 1);

    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRuntimeServerInitCalls", 0), 1);

    Engine.RemoveEntity(&ServerEntity);
    ProcessTickEvent();
    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentsRemainDormantUntilRuntimeModeIsExplicitlySet)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Unset);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.RegisterStaticModuleSource("/scripts/modules/runtime-unset-test.js", R"(
export function script() {
    globalThis.__ngxRuntimeUnsetInitCalls = (globalThis.__ngxRuntimeUnsetInitCalls || 0) + 1;
}
)");

    NgxScriptSystem.OnEnterSpace("runtime-unset-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("runtime-unset-space", &Engine);

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/runtime-unset-test.js");
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRuntimeUnsetInitCalls", 0), 0);

    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRuntimeUnsetInitCalls", 0), 1);

    Engine.RemoveEntity(&Entity);
    ProcessTickEvent();
    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, MissingModuleLogsExplicitError)
{
    std::vector<std::string> ErrorMessages;
    csp::common::LogSystem LogSystem;
    LogSystem.SetLogCallback([&ErrorMessages](csp::common::LogLevel Level, const csp::common::String& Message)
    {
        if (Level == csp::common::LogLevel::Error)
        {
            ErrorMessages.emplace_back(Message.c_str());
        }
    });

    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/engine/main.js", "import '/scripts/missing.js';");

    EXPECT_FALSE(NgxScriptSystem.RunModuleForTesting("/scripts/engine/main.js"));

    const auto MissingModuleLog = std::find_if(ErrorMessages.begin(), ErrorMessages.end(),
        [](const std::string& Message) { return Message.find("NgxScript: Missing module '/scripts/missing.js'.") != std::string::npos; });
    EXPECT_NE(MissingModuleLog, ErrorMessages.end());
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, ReloadUsesUpdatedSourceAndRerunsEntrypoint)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/lib/reload.js", "export const value = 5;");
    NgxScriptSystem.SetLoadedModuleSourceForTesting(
        "/scripts/engine/main.js", "import { value } from '/scripts/lib/reload.js'; globalThis.__ngxReloadValue = value;");
    ASSERT_TRUE(NgxScriptSystem.RunModuleForTesting("/scripts/engine/main.js"));
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxReloadValue", -1), 5);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/lib/reload.js", "export const value = 9;");
    ASSERT_TRUE(NgxScriptSystem.RunModuleForTesting("/scripts/engine/main.js"));
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxReloadValue", -1), 9);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeMountsUIScreenAndUnmountsOnRemoval)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.SetUIViewportSize(1000.0f, 600.0f);
    NgxScriptSystem.RegisterStaticModuleSource("/scripts/modules/ui-gallery.js", R"(
import { screen, row, column, button, image } from '@csp/ui';

export function ui() {
    return screen(
        { width: 900, height: 420, alignX: 'center', alignY: 'center', padding: 20, backgroundColor: '#111827FF', cornerRadius: 12 },
        row(
            { width: 'grow', height: 'grow', gap: 20 },
            column(
                { width: 220, gap: 12 },
                button('Alpha', { key: 'alpha', height: 56, backgroundColor: '#334155FF', textColor: '#FFFFFFFF' }),
                button('Beta', { key: 'beta', height: 56, backgroundColor: '#334155FF', textColor: '#FFFFFFFF' }),
            ),
            image(
                { assetCollectionId: 'museum-assets', imageAssetId: 'hero' },
                { key: 'heroImage', width: 'grow', height: 'grow', backgroundColor: '#020617FF' }
            )
        )
    );
}
)");

    NgxScriptSystem.OnEnterSpace("ui-gallery-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("ui-gallery-space", &Engine);

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/ui-gallery.js");
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    ProcessTickEvent();

    const std::string EntityId = std::to_string(Entity.GetId());
    const rapidjson::Document Drawables = ParseJson(NgxScriptSystem.GetUIDrawablesJsonForTesting(EntityId.c_str()).c_str());
    ASSERT_TRUE(Drawables.IsArray());
    EXPECT_NE(FindDrawableById(Drawables, "root.0.0.0"), nullptr);
    EXPECT_NE(FindDrawableById(Drawables, "root.0.1"), nullptr);

    const rapidjson::Document InitialUpdates = ParseJson(NgxScriptSystem.DrainPendingUIUpdates().c_str());
    EXPECT_NE(FindUpdateDrawableByType(InitialUpdates, "add", "button"), nullptr);
    EXPECT_NE(FindUpdateDrawableByType(InitialUpdates, "add", "image"), nullptr);

    Engine.RemoveEntity(&Entity);
    ProcessTickEvent();

    const rapidjson::Document RemovalUpdates = ParseJson(NgxScriptSystem.DrainPendingUIUpdates().c_str());
    EXPECT_NE(FindUpdateDrawableByType(RemovalUpdates, "remove", "button"), nullptr);

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeReactsToUIAttributeChanges)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.SetUIViewportSize(800.0f, 600.0f);
    NgxScriptSystem.RegisterStaticModuleSource("/scripts/modules/ui-score.js", R"(
import { screen, text } from '@csp/ui';

export const schema = {
    score: { type: 'integer', default: 1 }
};

export function ui({ attributes }) {
    return screen(
        { width: 320, height: 120, alignX: 'center', alignY: 'center', backgroundColor: '#0F172AFF', padding: 12 },
        text(`Score: ${attributes.score.value}`, { key: 'scoreText', textColor: '#FFFFFFFF', fontSize: 24 })
    );
}
)");

    NgxScriptSystem.OnEnterSpace("ui-score-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("ui-score-space", &Engine);

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/ui-score.js");
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    ProcessTickEvent();
    NgxScriptSystem.DrainPendingUIUpdates();

    const std::string EntityId = std::to_string(Entity.GetId());
    rapidjson::Document Drawables = ParseJson(NgxScriptSystem.GetUIDrawablesJsonForTesting(EntityId.c_str()).c_str());
    const rapidjson::Value* ScoreDrawable = FindDrawableById(Drawables, "root.0");
    ASSERT_NE(ScoreDrawable, nullptr);
    ASSERT_TRUE(ScoreDrawable->HasMember("text"));
    EXPECT_STREQ((*ScoreDrawable)["text"].GetString(), "Score: 1");

    EXPECT_TRUE(NgxScriptSystem.UpdateAttributeForEntity(EntityId.c_str(), "score", "7"));
    ProcessTickEvent();

    Drawables = ParseJson(NgxScriptSystem.GetUIDrawablesJsonForTesting(EntityId.c_str()).c_str());
    ScoreDrawable = FindDrawableById(Drawables, "root.0");
    ASSERT_NE(ScoreDrawable, nullptr);
    EXPECT_STREQ((*ScoreDrawable)["text"].GetString(), "Score: 7");

    const rapidjson::Document Updates = ParseJson(NgxScriptSystem.DrainPendingUIUpdates().c_str());
    const rapidjson::Value* UpdatedDrawable = FindUpdateDrawableByType(Updates, "update", "text");
    ASSERT_NE(UpdatedDrawable, nullptr);
    ASSERT_TRUE(UpdatedDrawable->HasMember("text"));
    EXPECT_STREQ((*UpdatedDrawable)["text"].GetString(), "Score: 7");

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeSplitsWrappedTextIntoPerLineDrawablesAndAppliesTextAlignment)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.SetUIViewportSize(800.0f, 600.0f);
    NgxScriptSystem.RegisterStaticModuleSource("/scripts/modules/ui-text-align.js", R"(
import { screen, column, text } from '@csp/ui';

export function ui() {
    return screen(
        { width: 420, height: 180, alignX: 'center', alignY: 'center', backgroundColor: '#020617FF', padding: 16 },
        column(
            { key: 'panel', width: 300, padding: 12, backgroundColor: '#111827FF' },
            text(`Longer first line for alignment
short`, {
                key: 'body',
                textColor: '#FFFFFFFF',
                fontSize: 18,
                textAlign: 'right'
            })
        )
    );
}
)");

    NgxScriptSystem.OnEnterSpace("ui-text-align-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("ui-text-align-space", &Engine);

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/ui-text-align.js");
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    ProcessTickEvent();

    const std::string EntityId = std::to_string(Entity.GetId());
    const rapidjson::Document Drawables = ParseJson(NgxScriptSystem.GetUIDrawablesJsonForTesting(EntityId.c_str()).c_str());
    const rapidjson::Value* FirstLine = FindDrawableById(Drawables, "root.0:panel.0:body.__line0");
    const rapidjson::Value* SecondLine = FindDrawableById(Drawables, "root.0:panel.0:body.__line1");
    ASSERT_NE(FirstLine, nullptr);
    ASSERT_NE(SecondLine, nullptr);
    ASSERT_TRUE(FirstLine->HasMember("text"));
    ASSERT_TRUE(SecondLine->HasMember("text"));
    EXPECT_STREQ((*FirstLine)["text"].GetString(), "Longer first line for alignment");
    EXPECT_STREQ((*SecondLine)["text"].GetString(), "short");
    EXPECT_GT((*SecondLine)["x"].GetFloat(), (*FirstLine)["x"].GetFloat());

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeAppliesAspectRatioAndConstrainedGrowSizing)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.SetUIViewportSize(800.0f, 600.0f);
    NgxScriptSystem.RegisterStaticModuleSource("/scripts/modules/ui-responsive-image.js", R"(
import { screen, column, image } from '@csp/ui';

export function ui() {
    return screen(
        { width: 'grow', height: 'grow' },
        column(
            { width: 'grow', height: 'grow', alignX: 'center', alignY: 'center' },
            image(
                { assetCollectionId: 'museum-assets', imageAssetId: 'hero' },
                {
                    key: 'hero',
                    width: { mode: 'grow', max: 500 },
                    height: 'grow',
                    aspectRatio: 2.0
                }
            )
        )
    );
}
)");

    NgxScriptSystem.OnEnterSpace("ui-responsive-image-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("ui-responsive-image-space", &Engine);

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/ui-responsive-image.js");
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    ProcessTickEvent();

    const std::string EntityId = std::to_string(Entity.GetId());
    rapidjson::Document Drawables = ParseJson(NgxScriptSystem.GetUIDrawablesJsonForTesting(EntityId.c_str()).c_str());
    const rapidjson::Value* HeroImage = FindDrawableById(Drawables, "root.0:hero");
    ASSERT_NE(HeroImage, nullptr);
    EXPECT_FLOAT_EQ((*HeroImage)["width"].GetFloat(), 500.0f);
    EXPECT_FLOAT_EQ((*HeroImage)["height"].GetFloat(), 250.0f);

    NgxScriptSystem.SetUIViewportSize(360.0f, 600.0f);
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    ProcessTickEvent();

    Drawables = ParseJson(NgxScriptSystem.GetUIDrawablesJsonForTesting(EntityId.c_str()).c_str());
    HeroImage = FindDrawableById(Drawables, "root.0:hero");
    ASSERT_NE(HeroImage, nullptr);
    EXPECT_FLOAT_EQ((*HeroImage)["width"].GetFloat(), 360.0f);
    EXPECT_FLOAT_EQ((*HeroImage)["height"].GetFloat(), 180.0f);

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeUsesClientTextMeasureCallbackWhenProvided)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    int32_t MeasureCallCount = 0;
    NgxScriptSystem.SetUITextMeasureCallback([&MeasureCallCount](const csp::common::String& Text, float FontSize, float& OutWidth, float& OutHeight) {
        ++MeasureCallCount;
        OutWidth = static_cast<float>(Text.Length()) * FontSize * 0.4f;
        OutHeight = FontSize * 1.1f;
    });

    NgxScriptSystem.SetUIViewportSize(640.0f, 360.0f);
    NgxScriptSystem.RegisterStaticModuleSource("/scripts/modules/ui-text-measure-callback.js", R"(
import { screen, column, text } from '@csp/ui';

export function ui() {
    return screen(
        { width: 'grow', height: 'grow' },
        column(
            { width: 'grow', height: 'grow', alignX: 'center', alignY: 'center' },
            text('Callback path active', { key: 'body', fontSize: 24, textAlign: 'left' })
        )
    );
}
)");

    NgxScriptSystem.OnEnterSpace("ui-text-measure-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("ui-text-measure-space", &Engine);

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/ui-text-measure-callback.js");
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    ProcessTickEvent();

    EXPECT_GT(MeasureCallCount, 0);

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeQueuesAsyncTextMeasureRequestsAndAppliesSubmittedResults)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.SetUIViewportSize(640.0f, 360.0f);
    NgxScriptSystem.RegisterStaticModuleSource("/scripts/modules/ui-text-measure-async.js", R"(
import { screen, column, text } from '@csp/ui';

export function ui() {
    return screen(
        { width: 'grow', height: 'grow' },
        column(
            { width: 'grow', height: 'grow', alignX: 'center', alignY: 'center' },
            text('Async browser measurement', { key: 'body', fontSize: 22, textAlign: 'left' })
        )
    );
}
)");

    NgxScriptSystem.OnEnterSpace("ui-text-measure-async-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("ui-text-measure-async-space", &Engine);

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/ui-text-measure-async.js");
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    ProcessTickEvent();
    NgxScriptSystem.DrainPendingUIUpdates();

    const rapidjson::Document Requests = ParseJson(NgxScriptSystem.DrainPendingUITextMeasureRequests().c_str());
    ASSERT_TRUE(Requests.IsArray());
    ASSERT_EQ(Requests.Size(), 1u);
    ASSERT_TRUE(Requests[0].IsObject());
    ASSERT_TRUE(Requests[0].HasMember("text"));
    ASSERT_TRUE(Requests[0].HasMember("fontSize"));
    EXPECT_STREQ(Requests[0]["text"].GetString(), "Async browser measurement");
    EXPECT_FLOAT_EQ(Requests[0]["fontSize"].GetFloat(), 22.0f);

    EXPECT_TRUE(NgxScriptSystem.SubmitUITextMeasureResults(R"([
        {
            "text": "Async browser measurement",
            "fontSize": 22,
            "width": 246,
            "height": 29
        }
    ])"));

    const rapidjson::Document RemainingRequests = ParseJson(NgxScriptSystem.DrainPendingUITextMeasureRequests().c_str());
    ASSERT_TRUE(RemainingRequests.IsArray());
    EXPECT_EQ(RemainingRequests.Size(), 0u);

    const std::string EntityId = std::to_string(Entity.GetId());
    const rapidjson::Document Drawables = ParseJson(NgxScriptSystem.GetUIDrawablesJsonForTesting(EntityId.c_str()).c_str());
    const rapidjson::Value* BodyDrawable = FindDrawableById(Drawables, "root.0:body");
    ASSERT_NE(BodyDrawable, nullptr);
    EXPECT_FLOAT_EQ((*BodyDrawable)["width"].GetFloat(), 246.0f);
    EXPECT_FLOAT_EQ((*BodyDrawable)["height"].GetFloat(), 29.0f);

    const rapidjson::Document Updates = ParseJson(NgxScriptSystem.DrainPendingUIUpdates().c_str());
    const rapidjson::Value* UpdatedDrawable = FindUpdateDrawableByType(Updates, "update", "text");
    ASSERT_NE(UpdatedDrawable, nullptr);
    EXPECT_FLOAT_EQ((*UpdatedDrawable)["width"].GetFloat(), 246.0f);
    EXPECT_FLOAT_EQ((*UpdatedDrawable)["height"].GetFloat(), 29.0f);

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeMountsWorldUIWithPlacementMetadata)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.RegisterStaticModuleSource("/scripts/modules/ui-world.js", R"(
import { world, column, text, button } from '@csp/ui';

export function ui() {
    return world(
        {
            width: 480,
            height: 240,
            targetEntityId: globalThis.__cspCurrentEntityId,
            worldOffset: { x: 0, y: 1.5, z: 0.25 },
            billboardMode: 'full'
        },
        column(
            { width: 'grow', height: 'grow', gap: 12, padding: 16, backgroundColor: '#111827FF' },
            text('Museum Exhibit', { key: 'title', textColor: '#FFFFFFFF', fontSize: 24 }),
            button('Learn More', { key: 'cta', backgroundColor: '#2563EBFF', textColor: '#FFFFFFFF' })
        )
    );
}
)");

    NgxScriptSystem.OnEnterSpace("ui-world-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("ui-world-space", &Engine);

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/ui-world.js");
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    ProcessTickEvent();

    const std::string EntityId = std::to_string(Entity.GetId());
    const rapidjson::Document Drawables = ParseJson(NgxScriptSystem.GetUIDrawablesJsonForTesting(EntityId.c_str()).c_str());
    const rapidjson::Value* TitleDrawable = FindDrawableById(Drawables, "root.0.0");
    ASSERT_NE(TitleDrawable, nullptr);
    ASSERT_TRUE(TitleDrawable->HasMember("surface"));
    EXPECT_STREQ((*TitleDrawable)["surface"].GetString(), "world");
    ASSERT_TRUE(TitleDrawable->HasMember("targetEntityId"));
    EXPECT_STREQ((*TitleDrawable)["targetEntityId"].GetString(), EntityId.c_str());
    ASSERT_TRUE(TitleDrawable->HasMember("billboardMode"));
    EXPECT_STREQ((*TitleDrawable)["billboardMode"].GetString(), "full");
    ASSERT_TRUE(TitleDrawable->HasMember("worldOffset"));
    ASSERT_TRUE((*TitleDrawable)["worldOffset"].IsObject());
    EXPECT_FLOAT_EQ((*TitleDrawable)["worldOffset"]["y"].GetFloat(), 1.5f);

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeMountsFloatingOverlayUI)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.SetUIViewportSize(640.0f, 360.0f);
    NgxScriptSystem.RegisterStaticModuleSource("/scripts/modules/ui-floating.js", R"(
import { screen, image, floating, button } from '@csp/ui';

export function ui() {
    return screen(
        { width: 640, height: 360, alignX: 'center', alignY: 'center' },
        image(
            { assetCollectionId: 'museum-assets', imageAssetId: 'hero' },
            { key: 'hero', width: 'grow', height: 'grow', backgroundColor: '#020617FF' }
        ),
        floating(
            {
                key: 'ctaOverlay',
                attachTo: 'parent',
                attachX: 'right',
                attachY: 'bottom',
                offset: { x: -12, y: -12 },
                zIndex: 10
            },
            button('Next', {
                key: 'ctaButton',
                width: 120,
                height: 48,
                backgroundColor: '#2563EBFF',
                textColor: '#FFFFFFFF'
            })
        )
    );
}
)");

    NgxScriptSystem.OnEnterSpace("ui-floating-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("ui-floating-space", &Engine);

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/ui-floating.js");
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    ProcessTickEvent();

    const std::string EntityId = std::to_string(Entity.GetId());
    const rapidjson::Document Drawables = ParseJson(NgxScriptSystem.GetUIDrawablesJsonForTesting(EntityId.c_str()).c_str());
    const rapidjson::Value* ButtonDrawable = FindDrawableById(Drawables, "root.1.0");
    ASSERT_NE(ButtonDrawable, nullptr);
    ASSERT_TRUE(ButtonDrawable->HasMember("x"));
    ASSERT_TRUE(ButtonDrawable->HasMember("y"));
    EXPECT_GT((*ButtonDrawable)["x"].GetFloat(), 480.0f);
    EXPECT_GT((*ButtonDrawable)["y"].GetFloat(), 260.0f);

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeWrapsFlowRowChildrenAcrossLines)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.SetUIViewportSize(900.0f, 600.0f);
    NgxScriptSystem.RegisterStaticModuleSource("/scripts/modules/ui-flow-row.js", R"(
import { screen, flowRow, button } from '@csp/ui';

const labels = ['One', 'Two', 'Three', 'Four', 'Five'];

export function ui() {
    return screen(
        { width: 520, height: 240, alignX: 'center', alignY: 'center', padding: 12 },
        flowRow(
            { key: 'cameraFlow', width: 460, columnGap: 16, rowGap: 12 },
            ...labels.map((label, index) => button(label, {
                key: `btn-${index}`,
                width: 140,
                height: 40,
                backgroundColor: '#111111',
                textColor: '#FFFFFFFF'
            }))
        )
    );
}
)");

    NgxScriptSystem.OnEnterSpace("ui-flow-row-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("ui-flow-row-space", &Engine);

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/ui-flow-row.js");
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    ProcessTickEvent();

    const std::string EntityId = std::to_string(Entity.GetId());
    const rapidjson::Document Drawables = ParseJson(NgxScriptSystem.GetUIDrawablesJsonForTesting(EntityId.c_str()).c_str());
    const rapidjson::Value* FirstButton = FindDrawableById(Drawables, "root.0.__row0.0");
    const rapidjson::Value* ThirdButton = FindDrawableById(Drawables, "root.0.__row0.2");
    const rapidjson::Value* FourthButton = FindDrawableById(Drawables, "root.0.__row1.0");
    ASSERT_NE(FirstButton, nullptr);
    ASSERT_NE(ThirdButton, nullptr);
    ASSERT_NE(FourthButton, nullptr);
    EXPECT_NEAR((*FirstButton)["y"].GetFloat(), (*ThirdButton)["y"].GetFloat(), 0.01f);
    EXPECT_GT((*FourthButton)["y"].GetFloat(), (*FirstButton)["y"].GetFloat());

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeDispatchesUIButtonClicks)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.SetUIViewportSize(640.0f, 480.0f);
    NgxScriptSystem.RegisterStaticModuleSource("/scripts/modules/ui-click.js", R"(
import { screen, button } from '@csp/ui';

export function ui() {
    return screen(
        { width: 240, height: 120, alignX: 'center', alignY: 'center', backgroundColor: '#020617FF', padding: 12 },
        button('Increment', {
            key: 'increment',
            backgroundColor: '#2563EBFF',
            textColor: '#FFFFFFFF',
            onClick: () => {
                globalThis.__ngxUIClickCount = (globalThis.__ngxUIClickCount || 0) + 1;
            }
        })
    );
}
)");

    NgxScriptSystem.OnEnterSpace("ui-click-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("ui-click-space", &Engine);

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/ui-click.js");
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    ProcessTickEvent();

    const std::string EntityId = std::to_string(Entity.GetId());
    const rapidjson::Document Drawables = ParseJson(NgxScriptSystem.GetUIDrawablesJsonForTesting(EntityId.c_str()).c_str());
    const rapidjson::Value* ButtonDrawable = FindDrawableById(Drawables, "root.0");
    const rapidjson::Value* ButtonLabelDrawable = FindDrawableById(Drawables, "root.0__label");
    ASSERT_NE(ButtonDrawable, nullptr);
    ASSERT_NE(ButtonLabelDrawable, nullptr);
    ASSERT_TRUE(ButtonDrawable->HasMember("handlerId"));
    ASSERT_TRUE((*ButtonDrawable)["handlerId"].IsString());
    ASSERT_TRUE(ButtonDrawable->HasMember("text"));
    EXPECT_STREQ((*ButtonDrawable)["text"].GetString(), "");
    ASSERT_TRUE(ButtonLabelDrawable->HasMember("text"));
    EXPECT_STREQ((*ButtonLabelDrawable)["text"].GetString(), "Increment");
    ASSERT_TRUE(ButtonLabelDrawable->HasMember("handlerId"));
    EXPECT_STREQ((*ButtonLabelDrawable)["handlerId"].GetString(), "");

    EXPECT_TRUE(NgxScriptSystem.DispatchUIAction(EntityId.c_str(), (*ButtonDrawable)["handlerId"].GetString()));
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxUIClickCount", 0), 1);

    Engine.RemoveEntity(&Entity);
    ProcessTickEvent();

    EXPECT_FALSE(NgxScriptSystem.DispatchUIAction(EntityId.c_str(), (*ButtonDrawable)["handlerId"].GetString()));

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeDispatchesUIButtonClicksThatBatchReactiveUnmounts)
{
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Play);
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.SetUIViewportSize(640.0f, 480.0f);
    NgxScriptSystem.RegisterStaticModuleSource("/scripts/modules/ui-close-screen.js", R"(
import { signal } from '@preact/signals-core';
import { screen, column, button, text } from '@csp/ui';

const showScreen = signal(true);
const typedCharacterCount = signal(12);

export function ui() {
    if (!showScreen.value) {
        return null;
    }

    return screen(
        { width: 320, height: 180, alignX: 'center', alignY: 'center', backgroundColor: '#020617FF', padding: 12 },
        column(
            { width: 'grow', height: 'grow', gap: 12 },
            text(`Typed: ${typedCharacterCount.value}`, { key: 'body', fontSize: 18, textAlign: 'left' }),
            button('Continue', {
                key: 'continue',
                backgroundColor: '#2563EBFF',
                textColor: '#FFFFFFFF',
                onClick: () => {
                    typedCharacterCount.value = 0;
                    showScreen.value = false;
                }
            })
        )
    );
}
)");

    NgxScriptSystem.OnEnterSpace("ui-close-screen-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("ui-close-screen-space", &Engine);

    csp::multiplayer::SpaceEntity Entity;
    auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Entity.AddComponent(csp::multiplayer::ComponentType::Code));
    ASSERT_NE(CodeComponent, nullptr);
    CodeComponent->SetScriptAssetPath("/scripts/modules/ui-close-screen.js");
    Engine.AddEntity(&Entity);

    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    ProcessTickEvent();

    const std::string EntityId = std::to_string(Entity.GetId());
    rapidjson::Document Drawables = ParseJson(NgxScriptSystem.GetUIDrawablesJsonForTesting(EntityId.c_str()).c_str());
    const rapidjson::Value* ButtonDrawable = FindDrawableById(Drawables, "root.0:continue");
    ASSERT_NE(ButtonDrawable, nullptr);
    ASSERT_TRUE(ButtonDrawable->HasMember("handlerId"));
    ASSERT_TRUE((*ButtonDrawable)["handlerId"].IsString());

    EXPECT_TRUE(NgxScriptSystem.DispatchUIAction(EntityId.c_str(), (*ButtonDrawable)["handlerId"].GetString()));
    ProcessTickEvent();

    Drawables = ParseJson(NgxScriptSystem.GetUIDrawablesJsonForTesting(EntityId.c_str()).c_str());
    EXPECT_EQ(Drawables.MemberCount(), 0u);

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
    csp::systems::SpaceSystem::SetRuntimeModeForTesting(csp::systems::ESpaceRuntimeMode::Edit);
}
