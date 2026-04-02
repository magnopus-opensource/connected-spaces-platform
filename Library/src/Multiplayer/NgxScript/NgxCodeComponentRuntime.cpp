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

#include "Multiplayer/NgxScript/NgxCodeComponentRuntime.h"

#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/Components/CodeSpaceComponent.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "Events/EventId.h"
#include "Events/EventListener.h"
#include "Events/EventSystem.h"
#include "Multiplayer/NgxScript/NgxScriptSystem.h"

#include <cmath>
#include <fmt/format.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace
{

constexpr const char* PLAYER_CONTROLLER_CONFIG_TAG = "player-controller-config";

constexpr const char* CODECOMPONENT_ASSET_REGISTRY_MODULE = "/scripts/engine/registry.js";
constexpr const char* CODECOMPONENT_REGISTRY_MODULE = "/__csp/internal/codecomponent/registry.js";
constexpr const char* CODECOMPONENT_BOOTSTRAP_MODULE = "/__csp/internal/codecomponent/bootstrap.js";
constexpr const char* CODECOMPONENT_SHUTDOWN_MODULE = "/__csp/internal/codecomponent/shutdown.js";
constexpr const char* CODECOMPONENT_HOOKS_MODULE = "@csp/hooks";
constexpr const char* CODECOMPONENT_HOOKS_SOURCE = "export { useEffect } from '/__csp/internal/codecomponent/registry.js';\n";
constexpr const char* CODECOMPONENT_CORE_MODULE = "@csp/core";
constexpr const char* CODECOMPONENT_INPUT_MODULE = "@csp/code";
constexpr const char* CODECOMPONENT_UI_MODULE = "@csp/ui";
constexpr const char* CODECOMPONENT_CORE_SOURCE = R"CORE(
export { useEffect } from '/__csp/internal/codecomponent/registry.js';
export const TheEntitySystem = new Proxy({}, {
    get(_, prop) {
        const sys = globalThis.TheEntitySystem;
        if (sys == null) return undefined;
        const val = sys[prop];
        return typeof val === 'function' ? val.bind(sys) : val;
    },
    set(_, prop, value) {
        if (globalThis.TheEntitySystem == null) return false;
        globalThis.TheEntitySystem[prop] = value;
        return true;
    },
});
)CORE";
constexpr const char* CODECOMPONENT_INPUT_SOURCE = R"INPUT(
function getInputDevice(name) {
    const input = globalThis.__cspInput;
    if (!input || !input[name]) {
        throw new Error(`@csp/code: input device '${name}' is unavailable.`);
    }
    return input[name];
}

export const TheEntitySystem = new Proxy({}, {
    get(_, prop) {
        const sys = globalThis.TheEntitySystem;
        if (sys == null) return undefined;
        const val = sys[prop];
        return typeof val === 'function' ? val.bind(sys) : val;
    },
    set(_, prop, value) {
        if (globalThis.TheEntitySystem == null) return false;
        globalThis.TheEntitySystem[prop] = value;
        return true;
    },
});

export const keyboard = {
    on(type, callback) {
        return getInputDevice('keyboard').on(type, callback);
    },
    off(type, callback) {
        return getInputDevice('keyboard').off(type, callback);
    },
    isPressed(key) {
        return getInputDevice('keyboard').isPressed(key);
    },
};

export const mouse = {
    on(type, callback) {
        return getInputDevice('mouse').on(type, callback);
    },
    off(type, callback) {
        return getInputDevice('mouse').off(type, callback);
    },
    isPressed(button) {
        return getInputDevice('mouse').isPressed(button);
    },
};

export const ThePlayerController = new Proxy({}, {
    get(_, prop) {
        const controller = globalThis.__cspPlayerController;
        if (controller == null) return undefined;
        const val = controller[prop];
        return typeof val === 'function' ? val.bind(controller) : val;
    },
    set(_, prop, value) {
        if (globalThis.__cspPlayerController == null) return false;
        globalThis.__cspPlayerController[prop] = value;
        return true;
    },
});
)INPUT";
constexpr const char* CODECOMPONENT_UI_SOURCE = R"UI(
function normalizeProps(value) {
    return value && typeof value === 'object' && !Array.isArray(value) ? { ...value } : {};
}

function flattenChildren(children, out = []) {
    for (const child of children) {
        if (Array.isArray(child)) {
            flattenChildren(child, out);
        } else if (child !== null && typeof child !== 'undefined' && child !== false) {
            out.push(child);
        }
    }
    return out;
}

function createNode(type, props, children) {
    return {
        __cspUiNode: true,
        type,
        props: normalizeProps(props),
        children: flattenChildren(children),
    };
}

function createContainerFactory(type) {
    return function(props = {}, ...children) {
        return createNode(type, props, children);
    };
}

export const screen = createContainerFactory('screen');
export const world = createContainerFactory('world');
export const row = createContainerFactory('row');
export const column = createContainerFactory('column');
export const flowRow = createContainerFactory('flowRow');
export const floating = createContainerFactory('floating');
export const spacer = (props = {}) => createNode('spacer', props, []);

export function text(value, props = {}) {
    const nextProps = normalizeProps(props);
    nextProps.text = String(value ?? '');
    return createNode('text', nextProps, []);
}

export function image(source = {}, props = {}) {
    const nextProps = normalizeProps(props);
    if (source && typeof source === 'object') {
        if (typeof source.assetCollectionId === 'string') {
            nextProps.assetCollectionId = source.assetCollectionId;
        }
        if (typeof source.imageAssetId === 'string') {
            nextProps.imageAssetId = source.imageAssetId;
        }
    }
    return createNode('image', nextProps, []);
}

export function button(label, props = {}) {
    const nextProps = normalizeProps(props);
    nextProps.text = String(label ?? '');
    return createNode('button', nextProps, []);
}
)UI";

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

const char* RuntimeModeToString(csp::systems::ESpaceRuntimeMode RuntimeMode)
{
    switch (RuntimeMode)
    {
    case csp::systems::ESpaceRuntimeMode::Unset:
        return "Unset";
    case csp::systems::ESpaceRuntimeMode::Edit:
        return "Edit";
    case csp::systems::ESpaceRuntimeMode::Play:
        return "Play";
    case csp::systems::ESpaceRuntimeMode::Server:
        return "Server";
    default:
        return "Unknown";
    }
}

bool IsPlayerControllerConfigEntity(const csp::multiplayer::SpaceEntity* Entity)
{
    return (Entity != nullptr) && Entity->HasTag(PLAYER_CONTROLLER_CONFIG_TAG);
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

std::string BuildReplicatedValueLiteral(const csp::common::ReplicatedValue& Value)
{
    switch (Value.GetReplicatedValueType())
    {
    case csp::common::ReplicatedValueType::Boolean:
        return Value.GetBool() ? "true" : "false";
    case csp::common::ReplicatedValueType::Integer:
        return std::to_string(Value.GetInt());
    case csp::common::ReplicatedValueType::Float:
        if (!std::isfinite(Value.GetFloat()))
        {
            return "0";
        }
        return fmt::format("{:.9g}", static_cast<double>(Value.GetFloat()));
    case csp::common::ReplicatedValueType::String:
        return "'" + EscapeJSStringLiteral(Value.GetString().c_str()) + "'";
    case csp::common::ReplicatedValueType::Vector2:
    {
        const auto& VectorValue = Value.GetVector2();
        return "[" + fmt::format("{:.9g}", static_cast<double>(VectorValue.X)) + "," + fmt::format("{:.9g}", static_cast<double>(VectorValue.Y))
            + "]";
    }
    case csp::common::ReplicatedValueType::Vector3:
    {
        const auto& VectorValue = Value.GetVector3();
        return "[" + fmt::format("{:.9g}", static_cast<double>(VectorValue.X)) + "," + fmt::format("{:.9g}", static_cast<double>(VectorValue.Y)) + ","
            + fmt::format("{:.9g}", static_cast<double>(VectorValue.Z)) + "]";
    }
    case csp::common::ReplicatedValueType::Vector4:
    {
        const auto& VectorValue = Value.GetVector4();
        return "[" + fmt::format("{:.9g}", static_cast<double>(VectorValue.X)) + "," + fmt::format("{:.9g}", static_cast<double>(VectorValue.Y)) + ","
            + fmt::format("{:.9g}", static_cast<double>(VectorValue.Z)) + "," + fmt::format("{:.9g}", static_cast<double>(VectorValue.W)) + "]";
    }
    case csp::common::ReplicatedValueType::StringMap:
    {
        std::string Literal = "{";
        bool bIsFirst = true;
        for (const auto& Pair : Value.GetStringMap())
        {
            if (!bIsFirst)
            {
                Literal += ",";
            }

            bIsFirst = false;
            Literal += "'";
            Literal += EscapeJSStringLiteral(Pair.first.c_str());
            Literal += "':";
            Literal += BuildReplicatedValueLiteral(Pair.second);
        }
        Literal += "}";
        return Literal;
    }
    default:
        return "undefined";
    }
}

std::string BuildAttributeValueLiteral(const csp::multiplayer::CodeAttribute& Attribute)
{
    switch (Attribute.Type)
    {
    case csp::multiplayer::CodePropertyType::Boolean:
        return Attribute.BooleanValue ? "true" : "false";
    case csp::multiplayer::CodePropertyType::Integer:
        return std::to_string(Attribute.IntegerValue);
    case csp::multiplayer::CodePropertyType::Float:
        if (!std::isfinite(Attribute.FloatValue))
        {
            return "0";
        }
        return fmt::format("{:.9g}", static_cast<double>(Attribute.FloatValue));
    case csp::multiplayer::CodePropertyType::String:
        return "'" + EscapeJSStringLiteral(Attribute.StringValue.c_str()) + "'";
    case csp::multiplayer::CodePropertyType::EntityQuery:
        return BuildReplicatedValueLiteral(csp::common::ReplicatedValue(Attribute.EntityQueryValue));
    case csp::multiplayer::CodePropertyType::ModelAsset:
        return BuildReplicatedValueLiteral(csp::common::ReplicatedValue(Attribute.ModelAssetValue));
    default:
        return "undefined";
    }
}

std::string BuildAttributesObjectLiteral(const std::map<std::string, csp::multiplayer::CodeAttribute>& Attributes)
{
    std::string Literal = "{";
    bool bIsFirst = true;

    for (const auto& AttributePair : Attributes)
    {
        if (!bIsFirst)
        {
            Literal += ",";
        }

        bIsFirst = false;
        Literal += "'";
        Literal += EscapeJSStringLiteral(AttributePair.first);
        Literal += "':";
        Literal += BuildAttributeValueLiteral(AttributePair.second);
    }

    Literal += "}";
    return Literal;
}

std::string BuildBootstrapSnippet(const std::string& ModulePath)
{
    const std::string EscapedModulePath = EscapeJSStringLiteral(ModulePath);

    return "import * as __ngxRegistryModule from '" + EscapedModulePath
        + "';\n"
          "const __ngxFactory = (typeof __ngxRegistryModule.createScriptRegistry === 'function')\n"
          "    ? __ngxRegistryModule.createScriptRegistry\n"
          "    : ((typeof __ngxRegistryModule.default === 'function') ? __ngxRegistryModule.default : null);\n"
          "if (typeof globalThis.scriptRegistry === 'undefined') {\n"
          "    if (__ngxFactory !== null) {\n"
          "        globalThis.scriptRegistry = __ngxFactory();\n"
          "    } else if (__ngxRegistryModule && typeof __ngxRegistryModule.default === 'object') {\n"
          "        globalThis.scriptRegistry = __ngxRegistryModule.default;\n"
          "    }\n"
          "}\n";
}

constexpr const char* CODECOMPONENT_REGISTRY_SOURCE = 
"\"\n"
"import { signal, effect, batch } from '@preact/signals-core';\"\n"
"\n"
"const codeComponents = new Map();\n"
"\n"
"export const useEffect = (callback) => {\n"
"    const entityId = globalThis.__cspCurrentEntityId;\n"
"    const dispose = effect(() => {\n"
"        const previousEntityId = globalThis.__cspCurrentEntityId;\n"
"        globalThis.__cspCurrentEntityId = entityId;\n"
"        try {\n"
"            return callback();\n"
"        } finally {\n"
"            globalThis.__cspCurrentEntityId = previousEntityId;\n"
"        }\n"
"    });\n"
"    if (entityId && codeComponents.has(entityId)) {\n"
"        codeComponents.get(entityId).effects.push(dispose);\n"
"    }\n"
"    return dispose;\n"
"};\n"
"\n"
"export function createScriptRegistry() {\n"
"    codeComponents.clear();\n"
"    const pendingSchemaSyncs = new Set();\n"
"    const pendingUIFlushes = new Set();\n"
"    let isDestroying = false;\n"
"\n"
"    function isObject(value) {\n"
"        return value && typeof value === 'object' && !Array.isArray(value);\n"
"    }\n"
"\n"
"    function cloneValue(value) {\n"
"        if (Array.isArray(value)) {\n"
"            return value.map((item) => cloneValue(item));\n"
"        }\n"
"\n"
"        if (isObject(value)) {\n"
"            const result = {};\n"
"            for (const key of Object.keys(value)) {\n"
"                result[key] = cloneValue(value[key]);\n"
"            }\n"
"            return result;\n"
"        }\n"
"\n"
"        return value;\n"
"    }\n"
"\n"
"    function cloneAttributes(attributes) {\n"
"        return isObject(attributes) ? cloneValue(attributes) : {};\n"
"    }\n"
"\n"
"    function sortedKeys(value) {\n"
"        return Object.keys(isObject(value) ? value : {}).sort();\n"
"    }\n"
"\n"
"    function warn(entityId, message) {\n"
"        console.warn(`NgxCodeComponentRuntime [${entityId}]: ${message}`);\n"
"    }\n"
"\n"
"    function normalizeSchemaType(rawType) {\n"
"        if (typeof rawType !== 'string') {\n"
"            return null;\n"
"        }\n"
"\n"
"        const lower = rawType.toLowerCase();\n"
"        if (lower === 'boolean' || lower === 'bool') {\n"
"            return 'boolean';\n"
"        }\n"
"\n"
"        if (lower === 'integer' || lower === 'int') {\n"
"            return 'integer';\n"
"        }\n"
"\n"
"        if (lower === 'float' || lower === 'number') {\n"
"            return 'float';\n"
"        }\n"
"\n"
"        if (lower === 'string') {\n"
"            return 'string';\n"
"        }\n"
"\n"
"        if (lower === 'entity') {\n"
"            return 'entity';\n"
"        }\n"
"\n"
"        if (lower === 'modelasset' || lower === 'model_asset') {\n"
"            return 'modelAsset';\n"
"        }\n"
"\n"
"        return null;\n"
"    }\n"
"\n"
"    function valueMatchesType(type, value) {\n"
"        if (type === 'boolean') {\n"
"            return typeof value === 'boolean';\n"
"        }\n"
"\n"
"        if (type === 'integer') {\n"
"            return typeof value === 'number' && Number.isInteger(value);\n"
"        }\n"
"\n"
"        if (type === 'float') {\n"
"            return typeof value === 'number' && Number.isFinite(value);\n"
"        }\n"
"\n"
"        if (type === 'string') {\n"
"            return typeof value === 'string';\n"
"        }\n"
"\n"
"        if (type === 'entity') {\n"
"            if (typeof value === 'string') {\n"
"                return true;\n"
"            }\n"
"            if (value !== null && typeof value === 'object' && !Array.isArray(value)) {\n"
"                const rawId = value.id;\n"
"                if (typeof rawId === 'string' && rawId.length > 0) {\n"
"                    return true;\n"
"                }\n"
"                if (typeof rawId === 'number' && Number.isInteger(rawId) && rawId > 0) {\n"
"                    return true;\n"
"                }\n"
"            }\n"
"            return false;\n"
"        }\n"
"\n"
"        if (type === 'modelAsset') {\n"
"            if (value === null || typeof value !== 'object' || Array.isArray(value)) {\n"
"                return false;\n"
"            }\n"
"\n"
"            return typeof value.assetCollectionId === 'string' && typeof value.assetId === 'string';\n"
"        }\n"
"\n"
"        return false;\n"
"    }\n"
"\n"
"    function describeValueType(value) {\n"
"        if (value === null) {\n"
"            return 'null';\n"
"        }\n"
"\n"
"        if (Array.isArray(value)) {\n"
"            return 'array';\n"
"        }\n"
"\n"
"        if (typeof value === 'number') {\n"
"            return Number.isInteger(value) ? 'integer' : 'float';\n"
"        }\n"
"\n"
"        return typeof value;\n"
"    }\n"
"\n"
"    function readSchema(moduleRef, entityId) {\n"
"        if (!moduleRef) {\n"
"            return null;\n"
"        }\n"
"\n"
"        let rawSchema;\n"
"        if (typeof moduleRef.schema !== 'undefined') {\n"
"            rawSchema = moduleRef.schema;\n"
"        } else if (typeof moduleRef.attributes !== 'undefined') {\n"
"            rawSchema = moduleRef.attributes;\n"
"        } else if (typeof moduleRef.codeComponentSchema !== 'undefined') {\n"
"            rawSchema = moduleRef.codeComponentSchema;\n"
"        } else {\n"
"            return null;\n"
"        }\n"
"\n"
"        if (!isObject(rawSchema)) {\n"
"            warn(entityId, 'schema export exists but is not an object; ignoring schema.');\n"
"            return null;\n"
"        }\n"
"\n"
"        const schema = {};\n"
"        for (const key of sortedKeys(rawSchema)) {\n"
"            const rawEntry = rawSchema[key];\n"
"            let rawType;\n"
"            let hasDefault = false;\n"
"            let defaultValue;\n"
"            let min;\n"
"            let max;\n"
"            let step;\n"
"\n"
"            if (typeof rawEntry === 'string') {\n"
"                rawType = rawEntry;\n"
"            } else if (isObject(rawEntry)) {\n"
"                rawType = rawEntry.type;\n"
"                if (typeof rawType === 'undefined') {\n"
"                    rawType = rawEntry.valueType;\n"
"                }\n"
"                if (typeof rawType === 'undefined') {\n"
"                    rawType = rawEntry.propertyType;\n"
"                }\n"
"\n"
"                if (Object.prototype.hasOwnProperty.call(rawEntry, 'default')) {\n"
"                    hasDefault = true;\n"
"                    defaultValue = rawEntry.default;\n"
"                } else if (Object.prototype.hasOwnProperty.call(rawEntry, 'defaultValue')) {\n"
"                    hasDefault = true;\n"
"                    defaultValue = rawEntry.defaultValue;\n"
"                } else if (Object.prototype.hasOwnProperty.call(rawEntry, 'value')) {\n"
"                    hasDefault = true;\n"
"                    defaultValue = rawEntry.value;\n"
"                }\n"
"\n"
"                if (typeof rawEntry.min === 'number' && Number.isFinite(rawEntry.min)) {\n"
"                    min = rawEntry.min;\n"
"                }\n"
"\n"
"                if (typeof rawEntry.max === 'number' && Number.isFinite(rawEntry.max)) {\n"
"                    max = rawEntry.max;\n"
"                }\n"
"\n"
"                if (typeof rawEntry.step === 'number' && Number.isFinite(rawEntry.step) && rawEntry.step > 0) {\n"
"                    step = rawEntry.step;\n"
"                }\n"
"            } else {\n"
"                warn(entityId, `schema entry '${key}' is invalid and will be ignored.`);\n"
"                continue;\n"
"            }\n"
"\n"
"            const type = normalizeSchemaType(rawType);\n"
"            if (!type) {\n"
"                warn(entityId, `schema entry '${key}' has unsupported type '${String(rawType)}' and will be ignored.`);\n"
"                continue;\n"
"            }\n"
"\n"
"            if (hasDefault && !valueMatchesType(type, defaultValue)) {\n"
"                warn(entityId,\n"
"                    `schema entry '${key}' has default of type '${describeValueType(defaultValue)}' but expected '${type}'; default ignored.`);\n"
"                hasDefault = false;\n"
"                defaultValue = undefined;\n"
"            }\n"
"\n"
"            schema[key] = { type, hasDefault, defaultValue, min, max, step };\n"
"        }\n"
"\n"
"        return Object.keys(schema).length > 0 ? schema : null;\n"
"    }\n"
"\n"
"    function buildSchemaMetadata(schema) {\n"
"        if (!schema) {\n"
"            return {};\n"
"        }\n"
"\n"
"        const result = {};\n"
"        for (const key of sortedKeys(schema)) {\n"
"            const rule = schema[key];\n"
"            const entry = { type: rule.type };\n"
"            if (typeof rule.min === 'number') {\n"
"                entry.min = rule.min;\n"
"            }\n"
"            if (typeof rule.max === 'number') {\n"
"                entry.max = rule.max;\n"
"            }\n"
"            if (typeof rule.step === 'number') {\n"
"                entry.step = rule.step;\n"
"            }\n"
"            if (rule.hasDefault) {\n"
"                entry.default = cloneValue(rule.defaultValue);\n"
"            }\n"
"            result[key] = entry;\n"
"        }\n"
"        return result;\n"
"    }\n"
"\n"
"    function reconcileAttributes(entityId, schema, incomingAttributes, previousAttributes) {\n"
"        const incoming = cloneAttributes(incomingAttributes);\n"
"        const previous = cloneAttributes(previousAttributes);\n"
"\n"
"        if (!schema) {\n"
"            return incoming;\n"
"        }\n"
"\n"
"        const next = {};\n"
"\n"
"        for (const key of sortedKeys(incoming)) {\n"
"            if (key.startsWith('$')) {\n"
"                next[key] = cloneValue(incoming[key]);\n"
"            } else if (!Object.prototype.hasOwnProperty.call(schema, key)) {\n"
"                warn(entityId, `attribute '${key}' is not declared in schema and will be ignored.`);\n"
"            }\n"
"        }\n"
"\n"
"        for (const key of sortedKeys(schema)) {\n"
"            const rule = schema[key];\n"
"            const hasIncoming = Object.prototype.hasOwnProperty.call(incoming, key);\n"
"            const hasPrevious = Object.prototype.hasOwnProperty.call(previous, key);\n"
"\n"
"            if (hasIncoming) {\n"
"                const value = incoming[key];\n"
"                if (valueMatchesType(rule.type, value)) {\n"
"                    next[key] = cloneValue(value);\n"
"                    continue;\n"
"                }\n"
"\n"
"                warn(entityId,\n"
"                    `attribute '${key}' has type '${describeValueType(value)}' but expected '${rule.type}'; value ignored.`);\n"
"            }\n"
"\n"
"            if (hasPrevious && valueMatchesType(rule.type, previous[key])) {\n"
"                next[key] = cloneValue(previous[key]);\n"
"                continue;\n"
"            }\n"
"\n"
"            if (rule.hasDefault) {\n"
"                next[key] = cloneValue(rule.defaultValue);\n"
"            }\n"
"        }\n"
"\n"
"        return next;\n"
"    }\n"
"\n"
"    function tryParseJSON(value) {\n"
"        if (typeof value !== 'string' || value.length === 0) {\n"
"            return null;\n"
"        }\n"
"\n"
"        try {\n"
"            return JSON.parse(value);\n"
"        } catch (error) {\n"
"            return null;\n"
"        }\n"
"    }\n"
"\n"
"    function resolveEntityRefState(entityId, previousState) {\n"
"        let normalizedEntityId = entityId;\n"
"        if (normalizedEntityId !== null && typeof normalizedEntityId === 'object' && !Array.isArray(normalizedEntityId)) {\n"
"            const rawId = normalizedEntityId.id;\n"
"            if (typeof rawId === 'string') {\n"
"                normalizedEntityId = rawId;\n"
"            } else if (typeof rawId === 'number' && Number.isInteger(rawId) && rawId > 0) {\n"
"                normalizedEntityId = String(rawId);\n"
"            }\n"
"        }\n"
"        if (typeof normalizedEntityId !== 'string' || normalizedEntityId.length === 0) {\n"
"            return {\n"
"                id: null,\n"
"                status: 'unbound',\n"
"                snapshot: null,\n"
"            };\n"
"        }\n"
"        const resolvedEntityId = normalizedEntityId;\n"
"\n"
"        let snapshot = null;\n"
"        if (globalThis.csp && typeof globalThis.csp.__getEntitySnapshot === 'function') {\n"
"            snapshot = tryParseJSON(globalThis.csp.__getEntitySnapshot(resolvedEntityId));\n"
"        }\n"
"\n"
"        return {\n"
"            id: resolvedEntityId,\n"
"            status: snapshot ? 'resolved' : 'missing',\n"
"            snapshot,\n"
"        };\n"
"    }\n"
"\n"
"    function createComponentRef(entityRefState, componentType, index = 0) {\n"
"        if (!entityRefState || entityRefState.status !== 'resolved' || !entityRefState.id) {\n"
"            return {\n"
"                status: 'missing',\n"
"                id: null,\n"
"                snapshot: null,\n"
"            };\n"
"        }\n"
"\n"
"        if (!globalThis.csp || typeof globalThis.csp.__getComponentSnapshot !== 'function') {\n"
"            return {\n"
"                status: 'missing',\n"
"                id: null,\n"
"                snapshot: null,\n"
"            };\n"
"        }\n"
"\n"
"        const componentSnapshot = tryParseJSON(globalThis.csp.__getComponentSnapshot(entityRefState.id, Number(componentType), Number(index)));\n"
"        if (!componentSnapshot) {\n"
"            return {\n"
"                status: 'missing',\n"
"                id: null,\n"
"                snapshot: null,\n"
"            };\n"
"        }\n"
"\n"
"        return {\n"
"            status: 'resolved',\n"
"            id: componentSnapshot.componentId ?? null,\n"
"            snapshot: componentSnapshot,\n"
"        };\n"
"    }\n"
"\n"
"    function toVector3(value) {\n"
"        if (Array.isArray(value) && value.length >= 3) {\n"
"            return [Number(value[0]), Number(value[1]), Number(value[2])];\n"
"        }\n"
"\n"
"        if (isObject(value)) {\n"
"            return [Number(value.x), Number(value.y), Number(value.z)];\n"
"        }\n"
"\n"
"        return null;\n"
"    }\n"
"\n"
"    function toVector4(value) {\n"
"        if (Array.isArray(value) && value.length >= 4) {\n"
"            return [Number(value[0]), Number(value[1]), Number(value[2]), Number(value[3])];\n"
"        }\n"
"\n"
"        if (isObject(value)) {\n"
"            return [Number(value.x), Number(value.y), Number(value.z), Number(value.w)];\n"
"        }\n"
"\n"
"        return null;\n"
"    }\n"
"\n"
"    function createScriptEntityRef(entityRefState) {\n"
"        function writePosition(value) {\n"
"            if (!globalThis.csp || typeof globalThis.csp.__setEntityPosition !== 'function') {\n"
"                return false;\n"
"            }\n"
"\n"
"            if (!entityRefState.id) {\n"
"                return false;\n"
"            }\n"
"\n"
"            const vector = toVector3(value);\n"
"            if (!vector || !vector.every((component) => Number.isFinite(component))) {\n"
"                return false;\n"
"            }\n"
"\n"
"            const success = !!globalThis.csp.__setEntityPosition(entityRefState.id, vector[0], vector[1], vector[2]);\n"
"            if (success && entityRefState.snapshot) {\n"
"                entityRefState.snapshot.position = [...vector];\n"
"            }\n"
"            return success;\n"
"        }\n"
"\n"
"        function writeRotation(value) {\n"
"            if (!globalThis.csp || typeof globalThis.csp.__setEntityRotation !== 'function') {\n"
"                return false;\n"
"            }\n"
"\n"
"            if (!entityRefState.id) {\n"
"                return false;\n"
"            }\n"
"\n"
"            const vector = toVector4(value);\n"
"            if (!vector || !vector.every((component) => Number.isFinite(component))) {\n"
"                return false;\n"
"            }\n"
"\n"
"            const success = !!globalThis.csp.__setEntityRotation(entityRefState.id, vector[0], vector[1], vector[2], vector[3]);\n"
"            if (success && entityRefState.snapshot) {\n"
"                entityRefState.snapshot.rotation = [...vector];\n"
"            }\n"
"            return success;\n"
"        }\n"
"\n"
"        function writeScale(value) {\n"
"            if (!globalThis.csp || typeof globalThis.csp.__setEntityScale !== 'function') {\n"
"                return false;\n"
"            }\n"
"\n"
"            if (!entityRefState.id) {\n"
"                return false;\n"
"            }\n"
"\n"
"            const vector = toVector3(value);\n"
"            if (!vector || !vector.every((component) => Number.isFinite(component))) {\n"
"                return false;\n"
"            }\n"
"\n"
"            const success = !!globalThis.csp.__setEntityScale(entityRefState.id, vector[0], vector[1], vector[2]);\n"
"            if (success && entityRefState.snapshot) {\n"
"                entityRefState.snapshot.scale = [...vector];\n"
"            }\n"
"            return success;\n"
"        }\n"
"\n"
"        function writePatch(patch) {\n"
"            if (!isObject(patch) || !entityRefState.id) {\n"
"                return false;\n"
"            }\n"
"\n"
"            let didSucceed = true;\n"
"\n"
"            if (Object.prototype.hasOwnProperty.call(patch, 'position')) {\n"
"                didSucceed = writePosition(patch.position) && didSucceed;\n"
"            }\n"
"\n"
"            if (Object.prototype.hasOwnProperty.call(patch, 'rotation')) {\n"
"                didSucceed = writeRotation(patch.rotation) && didSucceed;\n"
"            }\n"
"\n"
"            if (Object.prototype.hasOwnProperty.call(patch, 'scale')) {\n"
"                didSucceed = writeScale(patch.scale) && didSucceed;\n"
"            }\n"
"\n"
"            if (Object.prototype.hasOwnProperty.call(patch, 'name')\n"
"                && globalThis.csp\n"
"                && typeof globalThis.csp.__setEntityName === 'function'\n"
"                && typeof patch.name === 'string') {\n"
"                const success = !!globalThis.csp.__setEntityName(entityRefState.id, patch.name);\n"
"                if (success && entityRefState.snapshot) {\n"
"                    entityRefState.snapshot.name = patch.name;\n"
"                }\n"
"                didSucceed = success && didSucceed;\n"
"            }\n"
"\n"
"            if (Object.prototype.hasOwnProperty.call(patch, 'thirdPartyRef')\n"
"                && globalThis.csp\n"
"                && typeof globalThis.csp.__setEntityThirdPartyRef === 'function'\n"
"                && typeof patch.thirdPartyRef === 'string') {\n"
"                const success = !!globalThis.csp.__setEntityThirdPartyRef(entityRefState.id, patch.thirdPartyRef);\n"
"                if (success && entityRefState.snapshot) {\n"
"                    entityRefState.snapshot.thirdPartyRef = patch.thirdPartyRef;\n"
"                }\n"
"                didSucceed = success && didSucceed;\n"
"            }\n"
"\n"
"            return didSucceed;\n"
"        }\n"
"\n"
"        return {\n"
"            get id() {\n"
"                return entityRefState.id;\n"
"            },\n"
"            get status() {\n"
"                return entityRefState.status;\n"
"            },\n"
"            get snapshot() {\n"
"                return entityRefState.snapshot ? cloneValue(entityRefState.snapshot) : null;\n"
"            },\n"
"            patch(value) {\n"
"                return writePatch(value);\n"
"            },\n"
"            setPosition(value) {\n"
"                return writePosition(value);\n"
"            },\n"
"            setRotation(value) {\n"
"                return writeRotation(value);\n"
"            },\n"
"            setScale(value) {\n"
"                return writeScale(value);\n"
"            },\n"
"            getComponent(componentType, index = 0) {\n"
"                return createComponentRef(entityRefState, componentType, index);\n"
"            },\n"
"            getLightComponent(index = 0) {\n"
"                return createComponentRef(entityRefState, 10, index);\n"
"            },\n"
"        };\n"
"    }\n"
"\n"
"    function resolveEntityFromSystem(entityId) {\n"
"        if (!entityId) {\n"
"            return null;\n"
"        }\n"
"        if (globalThis.TheEntitySystem && typeof globalThis.TheEntitySystem.getEntityById === 'function') {\n"
"            return globalThis.TheEntitySystem.getEntityById(entityId) ?? null;\n"
"        }\n"
"        return null;\n"
"    }\n"
"\n"
"    function createSignalAttributes(entry, entityId) {\n"
"        const attrs = {};\n"
"        const schema = entry.schema || {};\n"
"\n"
"        for (const key of sortedKeys(entry.attributes)) {\n"
"            if (key.startsWith('$')) {\n"
"                continue;\n"
"            }\n"
"\n"
"            const rule = schema[key];\n"
"            if (rule && rule.type === 'entity') {\n"
"                const prevState = entry.entityRefStates[key] || { id: null, status: 'unbound', snapshot: null };\n"
"                const nextState = resolveEntityRefState(entry.attributes[key], prevState);\n"
"                entry.entityRefStates[key] = nextState;\n"
"                attrs[key] = signal(resolveEntityFromSystem(nextState.id));\n"
"            } else {\n"
"                attrs[key] = signal(entry.attributes[key]);\n"
"            }\n"
"        }\n"
"\n"
"        return attrs;\n"
"    }\n"
"\n"
"    function updateSignalAttributes(entry, entityId) {\n"
"        const schema = entry.schema || {};\n"
"\n"
"        batch(() => {\n"
"            for (const key of sortedKeys(entry.attributes)) {\n"
"                if (key.startsWith('$')) {\n"
"                    continue;\n"
"                }\n"
"\n"
"                const rule = schema[key];\n"
"\n"
"                if (!entry.signalAttributes[key]) {\n"
"                    if (rule && rule.type === 'entity') {\n"
"                        const state = resolveEntityRefState(entry.attributes[key],\n"
"                            entry.entityRefStates[key] || { id: null, status: 'unbound', snapshot: null });\n"
"                        entry.entityRefStates[key] = state;\n"
"                        entry.signalAttributes[key] = signal(resolveEntityFromSystem(state.id));\n"
"                    } else {\n"
"                        entry.signalAttributes[key] = signal(entry.attributes[key]);\n"
"                    }\n"
"                } else {\n"
"                    if (rule && rule.type === 'entity') {\n"
"                        const prevState = entry.entityRefStates[key] || { id: null, status: 'unbound', snapshot: null };\n"
"                        const nextState = resolveEntityRefState(entry.attributes[key], prevState);\n"
"                        entry.entityRefStates[key] = nextState;\n"
"                        entry.signalAttributes[key].value = resolveEntityFromSystem(nextState.id);\n"
"                    } else {\n"
"                        entry.signalAttributes[key].value = entry.attributes[key];\n"
"                    }\n"
"                }\n"
"            }\n"
"        });\n"
"    }\n"
"\n"
"    function flattenUIChildren(children, out = []) {\n"
"        for (const child of Array.isArray(children) ? children : []) {\n"
"            if (Array.isArray(child)) {\n"
"                flattenUIChildren(child, out);\n"
"            } else if (child !== null && typeof child !== 'undefined' && child !== false) {\n"
"                out.push(child);\n"
"            }\n"
"        }\n"
"        return out;\n"
"    }\n"
"\n"
"    function normalizeUIEntityId(value) {\n"
"        if (typeof value === 'string') {\n"
"            return value;\n"
"        }\n"
"        if (value && typeof value === 'object') {\n"
"            if (typeof value.id === 'string') {\n"
"                return value.id;\n"
"            }\n"
"            if (typeof value.id === 'number' && Number.isInteger(value.id) && value.id > 0) {\n"
"                return String(value.id);\n"
"            }\n"
"        }\n"
"        return null;\n"
"    }\n"
"\n"
"    function normalizeUINode(node, entityId, handlerMap, path = 'root') {\n"
"        if (!node || typeof node !== 'object') {\n"
"            return null;\n"
"        }\n"
"\n"
"        const type = typeof node.type === 'string' ? node.type : null;\n"
"        if (!type) {\n"
"            return null;\n"
"        }\n"
"\n"
"        const props = isObject(node.props) ? { ...node.props } : {};\n"
"        const normalized = {\n"
"            type,\n"
"            id: (typeof props.key === 'string' && props.key.length > 0) ? `${path}:${props.key}` : path,\n"
"            props: {},\n"
"            children: [],\n"
"        };\n"
"\n"
"        for (const [key, value] of Object.entries(props)) {\n"
"            if (key === 'key') {\n"
"                continue;\n"
"            }\n"
"\n"
"            if (key === 'onClick' && typeof value === 'function') {\n"
"                const handlerId = `${path}:click`;\n"
"                handlerMap.set(handlerId, value);\n"
"                normalized.props.onClickHandlerId = handlerId;\n"
"                continue;\n"
"            }\n"
"\n"
"            if (key === 'targetEntity' || key === 'entity') {\n"
"                const normalizedEntity = normalizeUIEntityId(value);\n"
"                if (normalizedEntity) {\n"
"                    normalized.props.targetEntityId = normalizedEntity;\n"
"                }\n"
"                continue;\n"
"            }\n"
"\n"
"            if (typeof value === 'function') {\n"
"                continue;\n"
"            }\n"
"\n"
"            if (Array.isArray(value)) {\n"
"                normalized.props[key] = value.map((item) => cloneValue(item));\n"
"            } else if (isObject(value)) {\n"
"                normalized.props[key] = cloneValue(value);\n"
"            } else if (typeof value !== 'undefined') {\n"
"                normalized.props[key] = value;\n"
"            }\n"
"        }\n"
"\n"
"        if (type === 'world' && !normalized.props.targetEntityId) {\n"
"            normalized.props.targetEntityId = entityId;\n"
"        }\n"
"\n"
"        const children = flattenUIChildren(node.children);\n"
"        normalized.children = children\n"
"            .map((child, index) => normalizeUINode(child, entityId, handlerMap, `${normalized.id}.${index}`))\n"
"            .filter((child) => child !== null);\n"
"\n"
"        return normalized;\n"
"    }\n"
"\n"
"    function clearUIHandlers(entry) {\n"
"        entry.uiHandlers.clear();\n"
"        if (entry.pendingUIHandlers) {\n"
"            entry.pendingUIHandlers.clear();\n"
"        }\n"
"    }\n"
"\n"
"    function unmountUI(entityId) {\n"
"        if (globalThis.csp && typeof globalThis.csp.__uiUnmount === 'function') {\n"
"            try {\n"
"                globalThis.csp.__uiUnmount(entityId);\n"
"            } catch (error) {\n"
"                console.error(`NgxCodeComponentRuntime: failed to unmount UI for entity ${entityId}`, error);\n"
"            }\n"
"        }\n"
"    }\n"
"\n"
"    function queueUIFlush(entityId, entry, treeJson, handlerMap) {\n"
"        if (!entry || entry.isDisposing || isDestroying) {\n"
"            return;\n"
"        }\n"
"\n"
"        entry.pendingUITreeJson = treeJson;\n"
"        entry.pendingUIHandlers = handlerMap;\n"
"        entry.uiDirty = true;\n"
"        pendingUIFlushes.add(entityId);\n"
"    }\n"
"\n"
"    function flushPendingUI(entityId, entry) {\n"
"        if (!entry || !entry.uiDirty || entry.isDisposing || isDestroying) {\n"
"            if (entry) {\n"
"                entry.uiDirty = false;\n"
"                entry.pendingUITreeJson = null;\n"
"                if (entry.pendingUIHandlers) {\n"
"                    entry.pendingUIHandlers.clear();\n"
"                }\n"
"            }\n"
"            pendingUIFlushes.delete(entityId);\n"
"            return;\n"
"        }\n"
"\n"
"        entry.uiDirty = false;\n"
"        pendingUIFlushes.delete(entityId);\n"
"\n"
"        entry.uiHandlers = entry.pendingUIHandlers instanceof Map ? entry.pendingUIHandlers : new Map();\n"
"        entry.pendingUIHandlers = new Map();\n"
"\n"
"        const treeJson = typeof entry.pendingUITreeJson === 'string' ? entry.pendingUITreeJson : null;\n"
"        if (!treeJson) {\n"
"            unmountUI(entityId);\n"
"            return;\n"
"        }\n"
"\n"
"        if (globalThis.csp && typeof globalThis.csp.__uiMount === 'function') {\n"
"            try {\n"
"                globalThis.csp.__uiMount(entityId, treeJson);\n"
"            } catch (error) {\n"
"                console.error(`NgxCodeComponentRuntime: failed to mount UI for entity ${entityId}`, error);\n"
"                entry.uiHandlers.clear();\n"
"                unmountUI(entityId);\n"
"            }\n"
"        }\n"
"    }\n"
"\n"
"    function startUIRuntime(entityId, entry) {\n"
"        if (!entry || entry.isDisposing || isDestroying || !entry.module || typeof entry.module.ui !== 'function') {\n"
"            return;\n"
"        }\n"
"\n"
"        if (typeof entry.uiDispose === 'function') {\n"
"            entry.uiDispose();\n"
"            entry.uiDispose = null;\n"
"        }\n"
"\n"
"        entry.uiDispose = effect(() => {\n"
"            if (entry.isDisposing || isDestroying) {\n"
"                return;\n"
"            }\n"
"\n"
"            const previousEntityId = globalThis.__cspCurrentEntityId;\n"
"            globalThis.__cspCurrentEntityId = entityId;\n"
"            try {\n"
"                const nextHandlers = new Map();\n"
"                const rawTree = entry.module.ui({ attributes: entry.signalAttributes, thisEntity: entry.thisEntitySignal, entityId });\n"
"                const normalizedTree = normalizeUINode(rawTree, entityId, nextHandlers);\n"
"\n"
"                if (!normalizedTree) {\n"
"                    queueUIFlush(entityId, entry, null, nextHandlers);\n"
"                    return;\n"
"                }\n"
"\n"
"                queueUIFlush(entityId, entry, JSON.stringify(normalizedTree), nextHandlers);\n"
"            } catch (error) {\n"
"                console.error(`NgxCodeComponentRuntime: ui() failed for entity ${entityId}`, error);\n"
"                queueUIFlush(entityId, entry, null, new Map());\n"
"            } finally {\n"
"                globalThis.__cspCurrentEntityId = previousEntityId;\n"
"            }\n"
"        });\n"
"    }\n"
"\n"
"    function stopUIRuntime(entityId, entry) {\n"
"        if (!entry) {\n"
"            return;\n"
"        }\n"
"\n"
"        if (typeof entry.uiDispose === 'function') {\n"
"            try {\n"
"                entry.uiDispose();\n"
"            } catch (error) {\n"
"                console.error(`NgxCodeComponentRuntime: ui dispose failed for entity ${entityId}`, error);\n"
"            }\n"
"        }\n"
"        entry.uiDispose = null;\n"
"        pendingUIFlushes.delete(entityId);\n"
"        entry.uiDirty = false;\n"
"        entry.pendingUITreeJson = null;\n"
"        clearUIHandlers(entry);\n"
"        unmountUI(entityId);\n"
"    }\n"
"\n"
"    function dispatchUIAction(entityId, handlerId) {\n"
"        const entry = codeComponents.get(entityId);\n"
"        if (!entry || entry.isDisposing || isDestroying || !entry.uiHandlers || typeof handlerId !== 'string') {\n"
"            return false;\n"
"        }\n"
"\n"
"        const handler = entry.uiHandlers.get(handlerId);\n"
"        if (typeof handler !== 'function') {\n"
"            return false;\n"
"        }\n"
"\n"
"        try {\n"
"            const previousEntityId = globalThis.__cspCurrentEntityId;\n"
"            globalThis.__cspCurrentEntityId = entityId;\n"
"            try {\n"
"                handler({\n"
"                    entityId,\n"
"                    type: 'click',\n"
"                    thisEntity: entry.thisEntitySignal ? entry.thisEntitySignal.value : null,\n"
"                });\n"
"            } finally {\n"
"                globalThis.__cspCurrentEntityId = previousEntityId;\n"
"            }\n"
"            return true;\n"
"        } catch (error) {\n"
"            console.error(`NgxCodeComponentRuntime: UI handler failed for entity ${entityId}`, error);\n"
"            return false;\n"
"        }\n"
"    }\n"
"\n"
"    function disposeAllEffects(entry) {\n"
"        if (!entry) {\n"
"            return;\n"
"        }\n"
"\n"
"        const effects = Array.isArray(entry.effects) ? [...entry.effects] : [];\n"
"        entry.effects = [];\n"
"\n"
"        for (const dispose of effects) {\n"
"            try {\n"
"                dispose();\n"
"            } catch (e) {\n"
"                console.error('NgxCodeComponentRuntime: effect dispose failed', e);\n"
"            }\n"
"        }\n"
"    }\n"
"\n"
"    function removeInputListenersForEntity(entityId) {\n"
"        if (globalThis.__cspInput && typeof globalThis.__cspInput.removeListenersForEntity === 'function') {\n"
"            globalThis.__cspInput.removeListenersForEntity(entityId);\n"
"        }\n"
"    }\n"
"\n"
"    function runScriptTeardown(entityId, entry, reason = 'unspecified') {\n"
"        if (!entry) {\n"
"            return;\n"
"        }\n"
"\n"
"        const teardown = entry.teardown;\n"
"        entry.teardown = null;\n"
"\n"
"        try {\n"
"            if (typeof teardown === 'function') {\n"
"                teardown();\n"
"            }\n"
"        } catch (error) {\n"
"            const scriptPath = entry && entry.scriptAssetPath ? entry.scriptAssetPath : '<unknown>';\n"
"            console.error(`NgxCodeComponentRuntime: script teardown failed for entity ${entityId} (${scriptPath}) [${reason}]`, error);\n"
"        } finally {\n"
"            removeInputListenersForEntity(entityId);\n"
"        }\n"
"    }\n"
"\n"
"    function ensureThisEntitySignal(entityId, entry) {\n"
"        if (!entry) {\n"
"            return null;\n"
"        }\n"
"\n"
"        const liveEntity = (globalThis.TheEntitySystem && typeof globalThis.TheEntitySystem.getEntityById === 'function')\n"
"            ? globalThis.TheEntitySystem.getEntityById(entityId)\n"
"            : null;\n"
"\n"
"        if (!entry.thisEntitySignal) {\n"
"            entry.thisEntitySignal = signal(liveEntity ?? null);\n"
"        } else {\n"
"            entry.thisEntitySignal.value = liveEntity ?? null;\n"
"        }\n"
"\n"
"        return entry.thisEntitySignal;\n"
"    }\n"
"\n"
"    function initializeScript(entityId, entry) {\n"
"        if (!entry || !entry.module || typeof entry.module.script !== 'function') {\n"
"            return;\n"
"        }\n"
"\n"
"        try {\n"
"            ensureThisEntitySignal(entityId, entry);\n"
"\n"
"            globalThis.__cspCurrentEntityId = entityId;\n"
"            try {\n"
"                const teardown = entry.module.script({ attributes: entry.signalAttributes, thisEntity: entry.thisEntitySignal, entityId });\n"
"                entry.teardown = typeof teardown === 'function' ? teardown : null;\n"
"            } finally {\n"
"                globalThis.__cspCurrentEntityId = undefined;\n"
"            }\n"
"        } catch (error) {\n"
"            const scriptPath = entry && entry.scriptAssetPath ? entry.scriptAssetPath : '<unknown>';\n"
"            const detail = error instanceof Error\n"
"                ? (typeof error.stack === 'string' && error.stack.length > 0 ? error.stack : String(error))\n"
"                : String(error);\n"
"            console.error(`NgxCodeComponentRuntime: script initialization failed for entity ${entityId} (${scriptPath})\\\\n${detail}`);\n"
"        }\n"
"    }\n"
"\n"
"    function tryImportCodeComponentModule(entityId, entry, reason = 'unspecified') {\n"
"        if (!entry || typeof entry.scriptAssetPath !== 'string' || entry.scriptAssetPath.length === 0) {\n"
"            return false;\n"
"        }\n"
"\n"
"        if (entry.importInFlight) {\n"
"            return false;\n"
"        }\n"
"\n"
"        const scriptAssetPath = entry.scriptAssetPath;\n"
"        const importGeneration = (typeof entry.importGeneration === 'number' ? entry.importGeneration : 0) + 1;\n"
"        entry.importGeneration = importGeneration;\n"
"        entry.importInFlight = true;\n"
"\n"
"        import(scriptAssetPath)\n"
"            .then((moduleRef) => {\n"
"                const current = codeComponents.get(entityId);\n"
"                if (!current\n"
"                    || current.scriptAssetPath !== scriptAssetPath\n"
"                    || current.importGeneration !== importGeneration) {\n"
"                    return;\n"
"                }\n"
"\n"
"                current.importInFlight = false;\n"
"                current.retryDelayFrames = 1;\n"
"                current.retryCountdown = 0;\n"
"                current.hasWarnedMissingModule = false;\n"
"                current.module = moduleRef;\n"
"\n"
"                if (current.initialized) {\n"
"                    current.isDisposing = true;\n"
"                    stopUIRuntime(entityId, current);\n"
"                    disposeAllEffects(current);\n"
"                    current.thisEntitySignal = null;\n"
"                    current.isDisposing = false;\n"
"                    current.initialized = false;\n"
"                }\n"
"\n"
"                if (current.pendingSchemaSync) {\n"
"                    syncCodeComponentSchema(entityId);\n"
"                    pendingSchemaSyncs.add(entityId);\n"
"                }\n"
"\n"
"                current.attributes = reconcileAttributes(entityId, current.schema, current.attributes, current.attributes);\n"
"                current.signalAttributes = createSignalAttributes(current, entityId);\n"
"                ensureThisEntitySignal(entityId, current);\n"
"                current.initialized = true;\n"
"\n"
"                initializeScript(entityId, current);\n"
"                startUIRuntime(entityId, current);\n"
"            })\n"
"            .catch((error) => {\n"
"                const current = codeComponents.get(entityId);\n"
"                if (!current\n"
"                    || current.scriptAssetPath !== scriptAssetPath\n"
"                    || current.importGeneration !== importGeneration) {\n"
"                    return;\n"
"                }\n"
"\n"
"                current.importInFlight = false;\n"
"                const delayFrames = typeof current.retryDelayFrames === 'number' && current.retryDelayFrames > 0\n"
"                    ? Math.min(current.retryDelayFrames * 2, 300)\n"
"                    : 1;\n"
"                current.retryDelayFrames = delayFrames;\n"
"                current.retryCountdown = delayFrames;\n"
"\n"
"                const errorMessage = (error && typeof error.message === 'string') ? error.message : String(error);\n"
"                if (errorMessage.includes('module not found') && !current.hasWarnedMissingModule) {\n"
"                    current.hasWarnedMissingModule = true;\n"
"                    warn(entityId, `module '${scriptAssetPath}' is not available yet; import will be retried automatically.`);\n"
"                }\n"
"                console.error(`NgxCodeComponentRuntime: Failed to import module ${scriptAssetPath} (${reason})`, error);\n"
"            });\n"
"\n"
"        return true;\n"
"    }\n"
"\n"
"    function syncCodeComponentSchema(entityId) {\n"
"        const entry = codeComponents.get(entityId);\n"
"        if (!entry) {\n"
"            return {};\n"
"        }\n"
"\n"
"        if (!entry.module) {\n"
"            entry.pendingSchemaSync = true;\n"
"            tryImportCodeComponentModule(entityId, entry, 'syncCodeComponentSchema');\n"
"            return {};\n"
"        }\n"
"\n"
"        const previousAttributes = cloneAttributes(entry.attributes);\n"
"        entry.schema = readSchema(entry.module, entityId);\n"
"        entry.attributes = reconcileAttributes(entityId, entry.schema, entry.attributes, previousAttributes);\n"
"        entry.pendingSchemaSync = false;\n"
"\n"
"        if (entry.initialized && entry.signalAttributes) {\n"
"            updateSignalAttributes(entry, entityId);\n"
"        }\n"
"\n"
"        return buildSchemaMetadata(entry.schema);\n"
"    }\n"
"\n"
"    function addCodeComponent(entityId, payload = {}) {\n"
"        const payloadObject = isObject(payload) ? payload : {};\n"
"        const scriptAssetPath = typeof payloadObject.scriptAssetPath === 'string' ? payloadObject.scriptAssetPath : '';\n"
"        const inputAttributes = cloneAttributes(payloadObject.attributes);\n"
"        const existing = codeComponents.get(entityId);\n"
"\n"
"        if (existing && existing.scriptAssetPath === scriptAssetPath) {\n"
"            syncCodeComponentAttributes(entityId, inputAttributes);\n"
"            if (!existing.module && scriptAssetPath.length > 0) {\n"
"                tryImportCodeComponentModule(entityId, existing, 'addCodeComponent-existing');\n"
"            }\n"
"            return true;\n"
"        }\n"
"\n"
"        if (existing) {\n"
"            stopUIRuntime(entityId, existing);\n"
"            disposeAllEffects(existing);\n"
"            existing.thisEntitySignal = null;\n"
"            codeComponents.delete(entityId);\n"
"        }\n"
"\n"
"        const entry = {\n"
"            scriptAssetPath,\n"
"            attributes: inputAttributes,\n"
"            signalAttributes: {},\n"
"            thisEntitySignal: null,\n"
"            teardown: null,\n"
"            module: null,\n"
"            schema: null,\n"
"            initialized: false,\n"
"            pendingSchemaSync: true,\n"
"            importGeneration: 0,\n"
"            importInFlight: false,\n"
"            retryDelayFrames: 1,\n"
"            retryCountdown: 0,\n"
"            hasWarnedMissingModule: false,\n"
"            entityRefStates: {},\n"
"            effects: [],\n"
"            uiDispose: null,\n"
"            uiHandlers: new Map(),\n"
"            pendingUIHandlers: new Map(),\n"
"            pendingUITreeJson: null,\n"
"            uiDirty: false,\n"
"            isDisposing: false,\n"
"        };\n"
"\n"
"        codeComponents.set(entityId, entry);\n"
"\n"
"        if (!scriptAssetPath) {\n"
"            warn(entityId, 'missing scriptAssetPath; component will not be initialized.');\n"
"            return false;\n"
"        }\n"
"\n"
"        tryImportCodeComponentModule(entityId, entry, 'addCodeComponent');\n"
"        return true;\n"
"    }\n"
"\n"
"    function syncCodeComponentAttributes(entityId, attributes = {}) {\n"
"        const entry = codeComponents.get(entityId);\n"
"        if (!entry) {\n"
"            return {};\n"
"        }\n"
"\n"
"        const previousAttributes = cloneAttributes(entry.attributes);\n"
"        entry.attributes = reconcileAttributes(entityId, entry.schema, attributes, previousAttributes);\n"
"\n"
"        if (entry.initialized && entry.signalAttributes) {\n"
"            updateSignalAttributes(entry, entityId);\n"
"        }\n"
"\n"
"        return cloneAttributes(entry.attributes);\n"
"    }\n"
"\n"
"    function updateAttributeForEntity(entityId, key, value) {\n"
"        const entry = codeComponents.get(entityId);\n"
"        if (!entry) {\n"
"            return;\n"
"        }\n"
"\n"
"        if (entry.schema) {\n"
"            if (!Object.prototype.hasOwnProperty.call(entry.schema, key)) {\n"
"                warn(entityId, `attribute '${key}' is not declared in schema and will be ignored.`);\n"
"                return;\n"
"            }\n"
"\n"
"            const rule = entry.schema[key];\n"
"            if (!valueMatchesType(rule.type, value)) {\n"
"                warn(entityId, `attribute '${key}' has type '${describeValueType(value)}' but expected '${rule.type}'; update ignored.`);\n"
"                return;\n"
"            }\n"
"        }\n"
"\n"
"        entry.attributes[key] = cloneValue(value);\n"
"\n"
"        if (entry.initialized && entry.signalAttributes && entry.signalAttributes[key]) {\n"
"            const schema = entry.schema || {};\n"
"            const rule = schema[key];\n"
"            if (rule && rule.type === 'entity') {\n"
"                const prevState = entry.entityRefStates[key] || { id: null, status: 'unbound', snapshot: null };\n"
"                const nextState = resolveEntityRefState(value, prevState);\n"
"                entry.entityRefStates[key] = nextState;\n"
"                entry.signalAttributes[key].value = resolveEntityFromSystem(nextState.id);\n"
"            } else {\n"
"                entry.signalAttributes[key].value = value;\n"
"            }\n"
"        }\n"
"    }\n"
"\n"
"    function removeCodeComponent(entityId) {\n"
"        const entry = codeComponents.get(entityId);\n"
"        if (!entry) {\n"
"            return;\n"
"        }\n"
"\n"
"        entry.isDisposing = true;\n"
"\n"
"        stopUIRuntime(entityId, entry);\n"
"        runScriptTeardown(entityId, entry, 'remove');\n"
"        disposeAllEffects(entry);\n"
"        entry.thisEntitySignal = null;\n"
"        codeComponents.delete(entityId);\n"
"    }\n"
"\n"
"    function tick() {\n"
"        for (const [entityId, entry] of codeComponents) {\n"
"            if (!entry.module && typeof entry.scriptAssetPath === 'string' && entry.scriptAssetPath.length > 0) {\n"
"                if (entry.importInFlight) {\n"
"                    continue;\n"
"                }\n"
"\n"
"                if (typeof entry.retryCountdown === 'number' && entry.retryCountdown > 0) {\n"
"                    entry.retryCountdown -= 1;\n"
"                    continue;\n"
"                }\n"
"\n"
"                tryImportCodeComponentModule(entityId, entry, 'tick-retry');\n"
"            }\n"
"        }\n"
"\n"
"        const dirtyUIIds = Array.from(pendingUIFlushes);\n"
"        for (const entityId of dirtyUIIds) {\n"
"            const entry = codeComponents.get(entityId);\n"
"            if (!entry || !entry.initialized) {\n"
"                pendingUIFlushes.delete(entityId);\n"
"                continue;\n"
"            }\n"
"\n"
"            flushPendingUI(entityId, entry);\n"
"        }\n"
"    }\n"
"\n"
"    function drainPendingSchemaSyncs() {\n"
"        const ids = Array.from(pendingSchemaSyncs);\n"
"        pendingSchemaSyncs.clear();\n"
"        return ids;\n"
"    }\n"
"\n"
"    function destroy() {\n"
"        isDestroying = true;\n"
"        for (const [entityId, entry] of codeComponents) {\n"
"            entry.isDisposing = true;\n"
"            stopUIRuntime(entityId, entry);\n"
"            runScriptTeardown(entityId, entry, 'destroy');\n"
"            disposeAllEffects(entry);\n"
"            entry.thisEntitySignal = null;\n"
"        }\n"
"        codeComponents.clear();\n"
"        pendingSchemaSyncs.clear();\n"
"        pendingUIFlushes.clear();\n"
"        isDestroying = false;\n"
"    }\n"
"\n"
"    return {\n"
"        syncCodeComponentSchema,\n"
"        addCodeComponent,\n"
"        syncCodeComponentAttributes,\n"
"        updateAttributeForEntity,\n"
"        removeCodeComponent,\n"
"        dispatchUIAction,\n"
"        tick,\n"
"        drainPendingSchemaSyncs,\n"
"        destroy,\n"
"    };\n"
"}\n"
"\n"
"export default createScriptRegistry;\n"
"\"\n";

constexpr const char* CODECOMPONENT_BOOTSTRAP_SOURCE = 
"import * as __ngxRegistryModule from '/__csp/internal/codecomponent/registry.js';\n"
"const __ngxFactory = (typeof __ngxRegistryModule.createScriptRegistry === 'function')\n"
"    ? __ngxRegistryModule.createScriptRegistry\n"
"    : ((typeof __ngxRegistryModule.default === 'function') ? __ngxRegistryModule.default : null);\n"
"\n"
"if (typeof globalThis.scriptRegistry === 'undefined') {\n"
"    if (__ngxFactory !== null) {\n"
"        globalThis.scriptRegistry = __ngxFactory();\n"
"    } else if (__ngxRegistryModule && typeof __ngxRegistryModule.default === 'object') {\n"
"        globalThis.scriptRegistry = __ngxRegistryModule.default;\n"
"    }\n"
"}\n";

constexpr const char* CODECOMPONENT_SHUTDOWN_SOURCE = 
"if (typeof globalThis.scriptRegistry !== 'undefined') {\n"
"    if (typeof globalThis.scriptRegistry.destroy === 'function') {\n"
"        globalThis.scriptRegistry.destroy();\n"
"    }\n"
"    delete globalThis.scriptRegistry;\n"
"}\n";

} // namespace

namespace csp::systems
{

class NgxCodeComponentRuntime::NgxCodeComponentTickEventHandler final : public csp::events::EventListener
{
public:
    explicit NgxCodeComponentTickEventHandler(NgxCodeComponentRuntime* InRuntime)
        : Runtime(InRuntime)
    {
    }

    void OnEvent(const csp::events::Event& InEvent) override
    {
        if (InEvent.GetId() == csp::events::FOUNDATION_TICK_EVENT_ID)
        {
            Runtime->OnTick();
        }
    }

private:
    NgxCodeComponentRuntime* Runtime;
};

NgxCodeComponentRuntime::NgxCodeComponentRuntime(csp::common::LogSystem& InLogSystem, NgxScriptSystem& InNgxScriptSystem)
    : LogSystem(InLogSystem)
    , ScriptSystem(InNgxScriptSystem)
    , ActiveRealtimeEngine(nullptr)
    , ActiveSpaceId("")
    , IsRegistryBootstrapped(false)
    , LastKnownContextGeneration(0)
    , LastObservedRuntimeMode(csp::systems::ESpaceRuntimeMode::Unset)
    , LastEntitySnapshots()
    , TickEventHandler(std::make_unique<NgxCodeComponentTickEventHandler>(this))
{
    RegisterBuiltInModules();
    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, TickEventHandler.get());
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxCodeComponentRuntime Trace: Tick listener registered.");
}

NgxCodeComponentRuntime::~NgxCodeComponentRuntime()
{
    OnExitSpace();
    csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, TickEventHandler.get());
}

void NgxCodeComponentRuntime::OnEnterSpace(const csp::common::String& InSpaceId, csp::common::IRealtimeEngine* InRealtimeEngine)
{
    const csp::systems::ESpaceRuntimeMode RuntimeMode = GetRuntimeMode();
    const std::string EnterMessage = fmt::format(
        "NgxCodeComponentRuntime Trace: OnEnterSpace(spaceId='{}', engineType={}, runtimeMode={}).",
        InSpaceId.c_str(), RealtimeEngineTypeToString(InRealtimeEngine), RuntimeModeToString(RuntimeMode));
    LogSystem.LogMsg(csp::common::LogLevel::Log, EnterMessage.c_str());

    ActiveSpaceId = InSpaceId;
    ActiveRealtimeEngine = InRealtimeEngine;
    IsRegistryBootstrapped = false;
    LastKnownContextGeneration = ScriptSystem.GetContextGeneration();
    LastObservedRuntimeMode = RuntimeMode;
    LastEntitySnapshots.clear();

    RegisterBuiltInModules();
    LogSystem.LogMsg(
        csp::common::LogLevel::Log, "NgxCodeComponentRuntime Trace: Deferring script registry bootstrap until script module loading completes.");
}

void NgxCodeComponentRuntime::OnExitSpace()
{
    if ((ActiveRealtimeEngine == nullptr) && ActiveSpaceId.IsEmpty() && LastEntitySnapshots.empty() && !IsRegistryBootstrapped)
    {
        return;
    }

    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxCodeComponentRuntime Trace: OnExitSpace().");
    LastEntitySnapshots.clear();
    ActiveSpaceId = "";
    ActiveRealtimeEngine = nullptr;
    LastKnownContextGeneration = 0;
    LastObservedRuntimeMode = csp::systems::ESpaceRuntimeMode::Unset;

    if (IsRegistryBootstrapped)
    {
        ScriptSystem.ExecuteModule(CODECOMPONENT_SHUTDOWN_MODULE);
        ScriptSystem.PumpPendingJobs();
    }

    IsRegistryBootstrapped = false;
}

void NgxCodeComponentRuntime::RegisterBuiltInModules()
{
    ScriptSystem.RegisterStaticModuleSource(CODECOMPONENT_REGISTRY_MODULE, CODECOMPONENT_REGISTRY_SOURCE);
    ScriptSystem.RegisterStaticModuleSource(CODECOMPONENT_BOOTSTRAP_MODULE, CODECOMPONENT_BOOTSTRAP_SOURCE);
    ScriptSystem.RegisterStaticModuleSource(CODECOMPONENT_SHUTDOWN_MODULE, CODECOMPONENT_SHUTDOWN_SOURCE);
    ScriptSystem.RegisterStaticModuleSource(CODECOMPONENT_HOOKS_MODULE, CODECOMPONENT_HOOKS_SOURCE);
    ScriptSystem.RegisterStaticModuleSource(CODECOMPONENT_CORE_MODULE, CODECOMPONENT_CORE_SOURCE);
    ScriptSystem.RegisterStaticModuleSource(CODECOMPONENT_INPUT_MODULE, CODECOMPONENT_INPUT_SOURCE);
    ScriptSystem.RegisterStaticModuleSource(CODECOMPONENT_UI_MODULE, CODECOMPONENT_UI_SOURCE);
}

void NgxCodeComponentRuntime::OnTick()
{
    if (ActiveRealtimeEngine == nullptr)
    {
        return;
    }

    const csp::systems::ESpaceRuntimeMode RuntimeMode = GetRuntimeMode();
    if (RuntimeMode != LastObservedRuntimeMode)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Log,
            fmt::format("NgxCodeComponentRuntime Trace: Observed runtime mode change {} -> {} while active in space '{}'.",
                RuntimeModeToString(LastObservedRuntimeMode), RuntimeModeToString(RuntimeMode), ActiveSpaceId.c_str())
                .c_str());
        LastObservedRuntimeMode = RuntimeMode;
    }

    ScriptSystem.PumpPendingJobs();

    const uint64_t CurrentContextGeneration = ScriptSystem.GetContextGeneration();
    if (CurrentContextGeneration != LastKnownContextGeneration)
    {
        LastKnownContextGeneration = CurrentContextGeneration;
        IsRegistryBootstrapped = false;
        LastEntitySnapshots.clear();
        LogSystem.LogMsg(csp::common::LogLevel::Log,
            "NgxCodeComponentRuntime Trace: Script context generation changed; forcing registry re-bootstrap and full entity resync.");
    }

    if (!BootstrapRegistryIfReady())
    {
        return;
    }

    EntitySnapshotMap CurrentSnapshots;
    if (!CaptureEntitySnapshots(CurrentSnapshots))
    {
        return;
    }

    SyncSnapshots(CurrentSnapshots);
    DrainPendingSchemaSyncs();
    ScriptSystem.FlushPendingCodeComponentUI();
    ScriptSystem.PumpPendingJobs();

    // requestAnimationFrame callbacks are driven by the client display loop via NgxScriptSystem::TickAnimationFrame.
    // Keep animation-frame dispatch decoupled from foundation tick cadence.
}

bool NgxCodeComponentRuntime::BootstrapRegistryIfReady()
{
    if (IsRegistryBootstrapped)
    {
        return true;
    }

    if (!ScriptSystem.AreScriptModulesLoaded())
    {
        return false;
    }

    RegisterBuiltInModules();

    const bool bHasAssetRegistry = ScriptSystem.HasModuleSource(CODECOMPONENT_ASSET_REGISTRY_MODULE);
    if (bHasAssetRegistry)
    {
        LogSystem.LogMsg(
            csp::common::LogLevel::Log, "NgxCodeComponentRuntime Trace: Bootstrap strategy uses asset module '/scripts/engine/registry.js'.");
        const std::string AssetBootstrapSnippet = BuildBootstrapSnippet(CODECOMPONENT_ASSET_REGISTRY_MODULE);
        if (ScriptSystem.EvaluateSnippet(AssetBootstrapSnippet.c_str(), "<ngx-codecomponent-asset-bootstrap>"))
        {
            IsRegistryBootstrapped = true;
            return true;
        }

        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxCodeComponentRuntime: Failed to bootstrap script registry from asset module path.");
    }

    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxCodeComponentRuntime Trace: Bootstrap strategy uses built-in fallback registry module.");
    if (ScriptSystem.ExecuteModule(CODECOMPONENT_BOOTSTRAP_MODULE))
    {
        IsRegistryBootstrapped = true;
        return true;
    }

    LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxCodeComponentRuntime: Failed to bootstrap built-in script registry module.");
    return false;
}

bool NgxCodeComponentRuntime::CaptureEntitySnapshots(EntitySnapshotMap& OutSnapshots) const
{
    OutSnapshots.clear();
    if (ActiveRealtimeEngine == nullptr)
    {
        return false;
    }

    ActiveRealtimeEngine->LockEntityUpdate();
    struct UnlockGuard
    {
        explicit UnlockGuard(csp::common::IRealtimeEngine* InEngine)
            : Engine(InEngine)
        {
        }

        ~UnlockGuard() { Engine->UnlockEntityUpdate(); }
        csp::common::IRealtimeEngine* Engine;
    };
    UnlockGuard Guard(ActiveRealtimeEngine);

    const size_t EntityCount = ActiveRealtimeEngine->GetNumEntities();
    for (size_t EntityIndex = 0; EntityIndex < EntityCount; ++EntityIndex)
    {
        auto* Entity = ActiveRealtimeEngine->GetEntityByIndex(EntityIndex);
        if (Entity == nullptr)
        {
            continue;
        }

        auto* Component = Entity->FindFirstComponentOfType(csp::multiplayer::ComponentType::Code);
        if (Component == nullptr)
        {
            continue;
        }

        auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Component);
        if (!ShouldActivateCodeComponent(Entity, CodeComponent))
        {
            continue;
        }

        CodeComponentSnapshot Snapshot;
        Snapshot.ScriptAssetPath = CodeComponent->GetScriptAssetPath().c_str();
        Snapshot.ScopeType = CodeComponent->GetCodeScopeType();
        Snapshot.RuntimeMode = GetRuntimeMode();

        const auto AttributeKeys = CodeComponent->GetAttributeKeys();
        for (size_t KeyIndex = 0; KeyIndex < AttributeKeys.Size(); ++KeyIndex)
        {
            csp::multiplayer::CodeAttribute Attribute;
            if (CodeComponent->GetAttribute(AttributeKeys[KeyIndex], Attribute))
            {
                Snapshot.Attributes[AttributeKeys[KeyIndex].c_str()] = Attribute;
            }
        }

        OutSnapshots[Entity->GetId()] = std::move(Snapshot);
    }

    return true;
}

void NgxCodeComponentRuntime::SyncSnapshots(const EntitySnapshotMap& CurrentSnapshots)
{
    for (const auto& PreviousEntityPair : LastEntitySnapshots)
    {
        if (CurrentSnapshots.find(PreviousEntityPair.first) == CurrentSnapshots.end())
        {
            RemoveEntityFromRegistry(PreviousEntityPair.first);
        }
    }

    for (const auto& CurrentEntityPair : CurrentSnapshots)
    {
        const uint64_t EntityId = CurrentEntityPair.first;
        const CodeComponentSnapshot& CurrentSnapshot = CurrentEntityPair.second;

        const auto PreviousSnapshotIt = LastEntitySnapshots.find(EntityId);
        if (PreviousSnapshotIt == LastEntitySnapshots.end())
        {
            AddOrReplaceEntityInRegistry(EntityId, CurrentSnapshot);
            continue;
        }

        const CodeComponentSnapshot& PreviousSnapshot = PreviousSnapshotIt->second;
        if ((CurrentSnapshot.ScriptAssetPath != PreviousSnapshot.ScriptAssetPath) || (CurrentSnapshot.ScopeType != PreviousSnapshot.ScopeType)
            || (CurrentSnapshot.RuntimeMode != PreviousSnapshot.RuntimeMode))
        {
            AddOrReplaceEntityInRegistry(EntityId, CurrentSnapshot);
            continue;
        }

        bool bHasRemovedAttributes = false;
        for (const auto& PreviousAttributePair : PreviousSnapshot.Attributes)
        {
            if (CurrentSnapshot.Attributes.find(PreviousAttributePair.first) == CurrentSnapshot.Attributes.end())
            {
                bHasRemovedAttributes = true;
                break;
            }
        }

        if (bHasRemovedAttributes)
        {
            SyncAttributesInRegistry(EntityId, CurrentSnapshot);
            continue;
        }

        for (const auto& CurrentAttributePair : CurrentSnapshot.Attributes)
        {
            const auto PreviousAttributeIt = PreviousSnapshot.Attributes.find(CurrentAttributePair.first);
            if ((PreviousAttributeIt == PreviousSnapshot.Attributes.end()) || (PreviousAttributeIt->second != CurrentAttributePair.second))
            {
                UpdateAttributeInRegistry(EntityId, CurrentAttributePair.first, CurrentAttributePair.second);
            }
        }
    }

    LastEntitySnapshots = CurrentSnapshots;
}

csp::systems::ESpaceRuntimeMode NgxCodeComponentRuntime::GetRuntimeMode() const
{
    return csp::systems::SpaceSystem::GetGlobalRuntimeMode();
}

uint64_t NgxCodeComponentRuntime::GetLocalClientId() const
{
    if ((ActiveRealtimeEngine != nullptr) && (ActiveRealtimeEngine->GetRealtimeEngineType() == csp::common::RealtimeEngineType::Online))
    {
        const auto* OnlineRealtimeEngine = static_cast<const csp::multiplayer::OnlineRealtimeEngine*>(ActiveRealtimeEngine);
        if (OnlineRealtimeEngine != nullptr)
        {
            if (auto* MultiplayerConnection = OnlineRealtimeEngine->GetMultiplayerConnectionInstance(); MultiplayerConnection != nullptr)
            {
                return MultiplayerConnection->GetClientId();
            }
        }
    }

    return csp::multiplayer::OfflineRealtimeEngine::LocalClientId();
}

bool NgxCodeComponentRuntime::IsEntityOrAncestorSelectedByLocalClient(const csp::multiplayer::SpaceEntity* Entity) const
{
    const uint64_t LocalClientId = GetLocalClientId();
    for (auto* CurrentEntity = Entity; CurrentEntity != nullptr; CurrentEntity = CurrentEntity->GetParentEntity())
    {
        if (CurrentEntity->IsSelected() && (CurrentEntity->GetSelectingClientID() == LocalClientId))
        {
            return true;
        }
    }

    return false;
}

bool NgxCodeComponentRuntime::ShouldActivateCodeComponent(
    const csp::multiplayer::SpaceEntity* Entity, const csp::multiplayer::CodeSpaceComponent* CodeComponent) const
{
    if ((Entity == nullptr) || (CodeComponent == nullptr))
    {
        return false;
    }

    switch (CodeComponent->GetCodeScopeType())
    {
    case csp::multiplayer::CodeScopeType::Local:
        if (IsPlayerControllerConfigEntity(Entity))
        {
            return GetRuntimeMode() == csp::systems::ESpaceRuntimeMode::Play;
        }

        if (GetRuntimeMode() == csp::systems::ESpaceRuntimeMode::Unset)
        {
            return false;
        }

        if (GetRuntimeMode() == csp::systems::ESpaceRuntimeMode::Play)
        {
            return true;
        }

        return (GetRuntimeMode() == csp::systems::ESpaceRuntimeMode::Edit) && IsEntityOrAncestorSelectedByLocalClient(Entity);

    case csp::multiplayer::CodeScopeType::Editor:
        return GetRuntimeMode() == csp::systems::ESpaceRuntimeMode::Edit;

    case csp::multiplayer::CodeScopeType::Server:
        return false;

    default:
        return false;
    }
}

bool NgxCodeComponentRuntime::ExecuteRegistrySnippet(const std::string& Snippet, const char* DebugName) const
{
    if (!ScriptSystem.EvaluateSnippet(Snippet.c_str(), DebugName))
    {
        const std::string WarningMessage = fmt::format("NgxCodeComponentRuntime: Failed to evaluate registry snippet '{}'.", DebugName);
        LogSystem.LogMsg(csp::common::LogLevel::Warning, WarningMessage.c_str());
        return false;
    }

    return true;
}

void NgxCodeComponentRuntime::SyncSchemaInRegistry(uint64_t EntityId)
{
    const std::string EntityIdString = std::to_string(EntityId);

    // Use the NgxScriptSystem bridge which returns a schema metadata object only.
    const csp::common::String SchemaResultJson = ScriptSystem.SyncCodeComponentSchema(EntityIdString.c_str());

    // Parse and write the schema object to the code component's dedicated schema property.
    if (ActiveRealtimeEngine == nullptr)
    {
        return;
    }

    rapidjson::Document ResultDoc;
    ResultDoc.Parse(SchemaResultJson.c_str());
    if (ResultDoc.HasParseError() || !ResultDoc.IsObject())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning,
            fmt::format("NgxCodeComponentRuntime: SyncSchemaInRegistry(entity={}) schema JSON parse failed or was not an object.", EntityIdString)
                .c_str());
        return;
    }

    rapidjson::StringBuffer SchemaBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> SchemaWriter(SchemaBuffer);
    ResultDoc.Accept(SchemaWriter);
    const std::string SchemaJsonString = SchemaBuffer.GetString();

    // Find the entity and write the schema to it.
    ActiveRealtimeEngine->LockEntityUpdate();
    const size_t EntityCount = ActiveRealtimeEngine->GetNumEntities();
    for (size_t EntityIndex = 0; EntityIndex < EntityCount; ++EntityIndex)
    {
        auto* Entity = ActiveRealtimeEngine->GetEntityByIndex(EntityIndex);
        if (Entity != nullptr && Entity->GetId() == EntityId)
        {
            auto* Component = Entity->FindFirstComponentOfType(csp::multiplayer::ComponentType::Code);
            if (Component != nullptr)
            {
                auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Component);
                CodeComponent->SetSchema(SchemaJsonString.c_str());
            }
            break;
        }
    }
    ActiveRealtimeEngine->UnlockEntityUpdate();
}

void NgxCodeComponentRuntime::DrainPendingSchemaSyncs()
{
    const csp::common::String ResultJson = ScriptSystem.DrainPendingSchemaSyncs();
    if (ResultJson.IsEmpty() || strcmp(ResultJson.c_str(), "[]") == 0)
    {
        return;
    }

    rapidjson::Document Doc;
    Doc.Parse(ResultJson.c_str());
    if (Doc.HasParseError() || !Doc.IsArray())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning,
            "NgxCodeComponentRuntime: DrainPendingSchemaSyncs result was not a valid JSON array.");
        return;
    }

    for (const auto& IdValue : Doc.GetArray())
    {
        if (!IdValue.IsString())
        {
            continue;
        }
        try
        {
            const uint64_t EntityId = std::stoull(IdValue.GetString());
            SyncSchemaInRegistry(EntityId);
        }
        catch (...)
        {
        }
    }
}

void NgxCodeComponentRuntime::SyncAttributesInRegistry(uint64_t EntityId, const CodeComponentSnapshot& Snapshot)
{
    const std::string EntityIdString = std::to_string(EntityId);
    const std::string AttributesLiteral = BuildAttributesObjectLiteral(Snapshot.Attributes);
    std::string Snippet = "if (globalThis.scriptRegistry) {\n"
                          "    if (typeof globalThis.scriptRegistry.syncCodeComponentAttributes === 'function') {\n"
                          "        globalThis.scriptRegistry.syncCodeComponentAttributes('"
        + EscapeJSStringLiteral(EntityIdString) + "', " + AttributesLiteral
        + ");\n"
          "    }\n"
          "}\n";

    ExecuteRegistrySnippet(Snippet, "<ngx-codecomponent-sync-attributes>");
}

void NgxCodeComponentRuntime::AddOrReplaceEntityInRegistry(uint64_t EntityId, const CodeComponentSnapshot& Snapshot)
{
    const std::string EntityIdString = std::to_string(EntityId);
    const std::string AttributesLiteral = BuildAttributesObjectLiteral(Snapshot.Attributes);
    const std::string Snippet = "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.addCodeComponent === 'function') {\n"
                                "    globalThis.scriptRegistry.addCodeComponent('"
        + EscapeJSStringLiteral(EntityIdString) + "', { scriptAssetPath: '" + EscapeJSStringLiteral(Snapshot.ScriptAssetPath)
        + "', attributes: " + AttributesLiteral + " });\n" + "}\n";

    ExecuteRegistrySnippet(Snippet, "<ngx-codecomponent-add>");
    SyncSchemaInRegistry(EntityId);
    SyncAttributesInRegistry(EntityId, Snapshot);
}

void NgxCodeComponentRuntime::UpdateAttributeInRegistry(uint64_t EntityId, const std::string& Key, const csp::multiplayer::CodeAttribute& Attribute)
{
    const std::string EntityIdString = std::to_string(EntityId);
    const std::string AttributeLiteral = BuildAttributeValueLiteral(Attribute);
    const std::string Snippet = "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.updateAttributeForEntity === 'function') {\n"
                                "    globalThis.scriptRegistry.updateAttributeForEntity('"
        + EscapeJSStringLiteral(EntityIdString) + "', '" + EscapeJSStringLiteral(Key) + "', " + AttributeLiteral + ");\n" + "}\n";

    ExecuteRegistrySnippet(Snippet, "<ngx-codecomponent-update>");
}

void NgxCodeComponentRuntime::RemoveEntityFromRegistry(uint64_t EntityId)
{
    const std::string EntityIdString = std::to_string(EntityId);
    const std::string Snippet = "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.removeCodeComponent === 'function') {\n"
                                "    globalThis.scriptRegistry.removeCodeComponent('"
        + EscapeJSStringLiteral(EntityIdString) + "');\n" + "}\n";

    ExecuteRegistrySnippet(Snippet, "<ngx-codecomponent-remove>");
}

} // namespace csp::systems
