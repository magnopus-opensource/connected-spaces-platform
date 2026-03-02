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

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/Components/CodeAttribute.h"

#include <map>
#include <memory>
#include <string>

namespace csp::common
{
class IRealtimeEngine;
class LogSystem;
} // namespace csp::common

namespace csp::systems
{

class NgxScriptSystem;

// Opinionated runtime layer for CodeComponent behaviour.
class NgxCodeComponentRuntime
{
public:
    NgxCodeComponentRuntime(csp::common::LogSystem& InLogSystem, NgxScriptSystem& InNgxScriptSystem);
    ~NgxCodeComponentRuntime();

    void OnEnterSpace(const csp::common::String& InSpaceId, csp::common::IRealtimeEngine* InRealtimeEngine);
    void OnExitSpace();

private:
    struct CodeComponentSnapshot
    {
        std::string ScriptAssetPath;
        std::map<std::string, csp::multiplayer::CodeAttribute> Attributes;
    };

    using EntitySnapshotMap = std::map<uint64_t, CodeComponentSnapshot>;

    class NgxCodeComponentTickEventHandler;

    void RegisterBuiltInModules();
    void OnTick();
    bool CaptureEntitySnapshots(EntitySnapshotMap& OutSnapshots) const;
    void SyncSnapshots(const EntitySnapshotMap& CurrentSnapshots);
    bool BootstrapRegistryIfReady();

    bool ExecuteRegistrySnippet(const std::string& Snippet, const char* DebugName) const;
    void SyncSchemaInRegistry(uint64_t EntityId);
    void SyncAttributesInRegistry(uint64_t EntityId, const CodeComponentSnapshot& Snapshot);
    void AddOrReplaceEntityInRegistry(uint64_t EntityId, const CodeComponentSnapshot& Snapshot);
    void UpdateAttributeInRegistry(uint64_t EntityId, const std::string& Key, const csp::multiplayer::CodeAttribute& Attribute);
    void RemoveEntityFromRegistry(uint64_t EntityId);

    csp::common::LogSystem& LogSystem;
    NgxScriptSystem& ScriptSystem;
    csp::common::IRealtimeEngine* ActiveRealtimeEngine;
    csp::common::String ActiveSpaceId;
    bool IsRegistryBootstrapped;
    uint64_t LastKnownContextGeneration;
    EntitySnapshotMap LastEntitySnapshots;
    std::unique_ptr<NgxCodeComponentTickEventHandler> TickEventHandler;
};

} // namespace csp::systems

