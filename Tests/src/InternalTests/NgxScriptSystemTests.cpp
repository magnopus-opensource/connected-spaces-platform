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

void ProcessTickEvent()
{
    auto& EventSystem = csp::events::EventSystem::Get();
    csp::events::Event* TickEvent = EventSystem.AllocateEvent(csp::events::FOUNDATION_TICK_EVENT_ID);
    EventSystem.EnqueueEvent(TickEvent);
    EventSystem.ProcessEvents();
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

CSP_INTERNAL_TEST(CSPEngine, NgxScriptSystemTests, CodeComponentRuntimeBootstrapsAndTearsDownRegistryGlobal)
{
    csp::common::LogSystem LogSystem;
    csp::systems::NgxScriptSystem NgxScriptSystem(LogSystem);
    csp::systems::NgxCodeComponentRuntime NgxCodeComponentRuntime(LogSystem, NgxScriptSystem);
    TestRealtimeEngine OfflineEngine(csp::common::RealtimeEngineType::Offline);

    NgxScriptSystem.OnEnterSpace("codecomponent-runtime-test-space", &OfflineEngine);
    NgxCodeComponentRuntime.OnEnterSpace("codecomponent-runtime-test-space", &OfflineEngine);

    ASSERT_TRUE(NgxScriptSystem.EvaluateModuleScriptForTesting(
        "globalThis.__ngxRegistryPresent = (typeof globalThis.scriptRegistry !== 'undefined') ? 1 : 0;"));
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRegistryPresent", -1), 1);

    NgxCodeComponentRuntime.OnExitSpace();

    ASSERT_TRUE(NgxScriptSystem.EvaluateModuleScriptForTesting(
        "globalThis.__ngxRegistryRemoved = (typeof globalThis.scriptRegistry === 'undefined') ? 1 : 0;"));
    EXPECT_EQ(NgxScriptSystem.GetGlobalIntForTesting("__ngxRegistryRemoved", -1), 1);

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
