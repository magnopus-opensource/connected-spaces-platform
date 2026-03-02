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
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/Components/CodeAttribute.h"
#include "CSP/Multiplayer/Components/CodeSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "Events/EventId.h"
#include "Events/EventSystem.h"
#include "Multiplayer/NgxScript/NgxCodeComponentRuntime.h"
#include "Multiplayer/NgxScript/NgxScriptSystem.h"
#include "TestHelpers.h"

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
};

csp::multiplayer::CodeAttribute::EntityQueryValueType BuildNameEntityQuery(const csp::common::String& Name)
{
    csp::multiplayer::CodeAttribute::EntityQueryValueType Query;
    Query["kind"] = "name";
    Query["name"] = Name;
    return Query;
}

void ProcessTickEvent()
{
    auto& EventSystem = csp::events::EventSystem::Get();
    csp::events::Event* TickEvent = EventSystem.AllocateEvent(csp::events::FOUNDATION_TICK_EVENT_ID);
    EventSystem.EnqueueEvent(TickEvent);
    EventSystem.ProcessEvents();
}

void ProcessScriptTick(csp::systems::NgxScriptSystem& NgxScriptSystem, double TimestampMs)
{
    EXPECT_TRUE(NgxScriptSystem.TickScriptRegistry(TimestampMs));
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

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, RegistryBootstrapUsesAssetModuleWhenAvailable)
{
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
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, RegistryBootstrapFallsBackToBuiltInWhenAssetMissing)
{
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
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("schema-sync-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("schema-sync-space", &Engine);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/schema-test.js", R"(
export const schema = {
    speed: { type: 'float', default: 1.5 },
    enabled: { type: 'boolean', default: true },
    label: { type: 'string', default: 'player' },
};

export function onInit({ attributes }) {
    globalThis.__ngxSchemaSpeedScaled = Math.round(attributes.speed * 100);
    globalThis.__ngxSchemaEnabled = attributes.enabled ? 1 : 0;
    globalThis.__ngxSchemaLabelLength = attributes.label.length;
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
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentEntityReferenceResolvesWritesAndHandlesDeletion)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("entity-ref-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("entity-ref-space", &Engine);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/entity-ref-test.js", R"(
export const schema = {
    target: { type: 'entity' },
};

export function onInit({ attributes }) {
    globalThis.__ngxEntityRefInitResolved = attributes.target.status === 'resolved' ? 1 : 0;
    globalThis.__ngxEntityRefInitHasId = attributes.target.id !== null ? 1 : 0;
    const lightComponent = attributes.target.getLightComponent();
    globalThis.__ngxEntityRefLightResolved = lightComponent.status === 'resolved' ? 1 : 0;
}

export function onTick({ attributes }) {
    globalThis.__ngxEntityRefTickStatusResolved = attributes.target.status === 'resolved' ? 1 : 0;
    globalThis.__ngxEntityRefTickStatusDeleted = attributes.target.status === 'deleted' ? 1 : 0;

    if (!globalThis.__ngxEntityRefDidWrite) {
        globalThis.__ngxEntityRefDidWrite = attributes.target.setPosition([7, 8, 9]) ? 1 : 0;
    }
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

    ProcessScriptTick(NgxScriptSystem, 16.0);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxEntityRefTickStatusResolved", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxEntityRefDidWrite", 0), 1);
    EXPECT_EQ(TargetEntity.GetPosition(), csp::common::Vector3 { 7.0f, 8.0f, 9.0f });

    Engine.RemoveEntity(&TargetEntity);
    ProcessScriptTick(NgxScriptSystem, 32.0);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxEntityRefTickStatusDeleted", 0), 1);

    Engine.RemoveEntity(&ScriptEntity);
    ProcessTickEvent();

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentFallbackLifecycleIsStableAndIncremental)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("lifecycle-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("lifecycle-space", &Engine);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/lifecycle-test.js", R"(
export function onInit({ attributes }) {
    globalThis.__ngxLifecycleInitCalls = (globalThis.__ngxLifecycleInitCalls || 0) + 1;
    globalThis.__ngxLifecycleLastScore = attributes.score;
}

export function onUpdateAttributes({ key, value }) {
    globalThis.__ngxLifecycleUpdateCalls = (globalThis.__ngxLifecycleUpdateCalls || 0) + 1;
    if (key === 'score') {
        globalThis.__ngxLifecycleLastScore = value;
    }
}

export function onTick() {
    globalThis.__ngxLifecycleTickCalls = (globalThis.__ngxLifecycleTickCalls || 0) + 1;
}

export function onDispose() {
    globalThis.__ngxLifecycleDisposeCalls = (globalThis.__ngxLifecycleDisposeCalls || 0) + 1;
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
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxLifecycleUpdateCalls", 0), 0);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxLifecycleLastScore", -1), 1);

    CodeComponent->SetAttribute("score", csp::multiplayer::CodeAttribute::FromInteger(2));
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();

    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxLifecycleInitCalls", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxLifecycleUpdateCalls", 0), 1);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxLifecycleLastScore", -1), 2);

    ProcessScriptTick(NgxScriptSystem, 16.0);
    ProcessScriptTick(NgxScriptSystem, 32.0);
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxLifecycleTickCalls", 0), 2);

    Engine.RemoveEntity(&Entity);
    ProcessTickEvent();
    ProcessTickEvent();
    NgxScriptSystem.PumpPendingJobs();
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxLifecycleDisposeCalls", 0), 1);

    NgxCodeComponentRuntime.OnExitSpace();
    NgxScriptSystem.OnExitSpace();
}

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, ScriptWithoutSchemaStillExecutesThroughFallbackRegistry)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngineWithEntities Engine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("no-schema-script-space", &Engine);
    NgxCodeComponentRuntime.OnEnterSpace("no-schema-script-space", &Engine);

    NgxScriptSystem.SetLoadedModuleSourceForTesting("/scripts/modules/no-schema-test.js", R"(
export function onInit({ attributes }) {
    globalThis.__ngxNoSchemaInitCalls = (globalThis.__ngxNoSchemaInitCalls || 0) + 1;
    globalThis.__ngxNoSchemaValue = attributes.value;
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
