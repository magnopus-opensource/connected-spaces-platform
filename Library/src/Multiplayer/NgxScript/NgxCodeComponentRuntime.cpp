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

export function setDebugMode(enabled) {
    if (typeof __setUIDebugMode === 'function') {
        __setUIDebugMode(!!enabled);
    }
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
    case csp::multiplayer::CodePropertyType::Vector2:
        return BuildReplicatedValueLiteral(csp::common::ReplicatedValue(Attribute.Vector2Value));
    case csp::multiplayer::CodePropertyType::Vector3:
        return BuildReplicatedValueLiteral(csp::common::ReplicatedValue(Attribute.Vector3Value));
    case csp::multiplayer::CodePropertyType::Vector4:
        return BuildReplicatedValueLiteral(csp::common::ReplicatedValue(Attribute.Vector4Value));
    case csp::multiplayer::CodePropertyType::Quaternion:
        return BuildReplicatedValueLiteral(csp::common::ReplicatedValue(Attribute.QuaternionValue));
    case csp::multiplayer::CodePropertyType::Color:
        return BuildReplicatedValueLiteral(csp::common::ReplicatedValue(Attribute.ColorValue));
    case csp::multiplayer::CodePropertyType::EntityQuery:
        return BuildReplicatedValueLiteral(csp::common::ReplicatedValue(Attribute.EntityQueryValue));
    case csp::multiplayer::CodePropertyType::ModelAsset:
        return BuildReplicatedValueLiteral(csp::common::ReplicatedValue(Attribute.ModelAssetValue));
    case csp::multiplayer::CodePropertyType::ImageAsset:
        return BuildReplicatedValueLiteral(csp::common::ReplicatedValue(Attribute.ImageAssetValue));
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

constexpr const char* CODECOMPONENT_REGISTRY_SOURCE = R"(
import { signal, effect, batch } from '@preact/signals-core';

const codeComponents = new Map();

export const useEffect = (callback) => {
    const entityId = globalThis.__cspCurrentEntityId;
    if (globalThis.__cspUiRenderActive && entityId && codeComponents.has(entityId)) {
        const entry = codeComponents.get(entityId);
        const slotIndex = typeof entry.uiEffectCursor === 'number' ? entry.uiEffectCursor : 0;
        entry.uiEffectCursor = slotIndex + 1;
        if (!Array.isArray(entry.pendingUiEffectCallbacks)) {
            entry.pendingUiEffectCallbacks = [];
        }
        entry.pendingUiEffectCallbacks[slotIndex] = callback;
        return () => {};
    }

    const dispose = effect(() => {
        const previousEntityId = globalThis.__cspCurrentEntityId;
        globalThis.__cspCurrentEntityId = entityId;
        try {
            return callback();
        } finally {
            globalThis.__cspCurrentEntityId = previousEntityId;
        }
    });
    if (entityId && codeComponents.has(entityId)) {
        codeComponents.get(entityId).effects.push(dispose);
    }
    return dispose;
};

export function createScriptRegistry() {
    codeComponents.clear();
    const pendingSchemaSyncs = new Set();
    const pendingModuleActivations = new Set();
    const pendingUIFlushes = new Set();
    let isDestroying = false;

    function isObject(value) {
        return value && typeof value === 'object' && !Array.isArray(value);
    }

    const CLONE_VALUE_MAX_DEPTH = 64;

    function cloneValue(value, depth = 0) {
        if (depth > CLONE_VALUE_MAX_DEPTH) {
            console.error(`[NGX UI][runtime] cloneValue exceeded max depth (${CLONE_VALUE_MAX_DEPTH}) — possible cyclic object in UI prop. Returning null.`);
            return null;
        }

        if (Array.isArray(value)) {
            return value.map((item) => cloneValue(item, depth + 1));
        }

        if (isObject(value)) {
            const result = {};
            for (const key of Object.keys(value)) {
                result[key] = cloneValue(value[key], depth + 1);
            }
            return result;
        }

        return value;
    }

    function cloneAttributes(attributes) {
        return isObject(attributes) ? cloneValue(attributes) : {};
    }

    function sortedKeys(value) {
        return Object.keys(isObject(value) ? value : {}).sort();
    }

    function isFiniteNumber(value) {
        return typeof value === 'number' && Number.isFinite(value);
    }

    function isNumericArray(value, expectedLength) {
        return Array.isArray(value) && value.length === expectedLength && value.every(isFiniteNumber);
    }

    function isObjectVector(value, keys) {
        if (!isObject(value)) {
            return false;
        }

        return keys.every((key) => isFiniteNumber(value[key]));
    }

    function warn(entityId, message) {
        console.warn(`NgxCodeComponentRuntime [${entityId}]: ${message}`);
    }

    function normalizeSchemaType(rawType) {
        if (typeof rawType !== 'string') {
            return null;
        }

        const lower = rawType.toLowerCase();
        if (lower === 'boolean' || lower === 'bool') {
            return 'boolean';
        }

        if (lower === 'integer' || lower === 'int') {
            return 'integer';
        }

        if (lower === 'float' || lower === 'number') {
            return 'float';
        }

        if (lower === 'string') {
            return 'string';
        }

        if (lower === 'vector2' || lower === 'vec2') {
            return 'vector2';
        }

        if (lower === 'vector3' || lower === 'vec3') {
            return 'vector3';
        }

        if (lower === 'vector4' || lower === 'vec4') {
            return 'vector4';
        }

        if (lower === 'quaternion') {
            return 'quaternion';
        }

        if (lower === 'color') {
            return 'color';
        }

        if (lower === 'entity') {
            return 'entity';
        }

        if (lower === 'modelasset' || lower === 'model_asset') {
            return 'modelAsset';
        }

        if (lower === 'imageasset' || lower === 'image_asset') {
            return 'imageAsset';
        }

        return null;
    }

    function valueMatchesType(type, value) {
        if (type === 'boolean') {
            return typeof value === 'boolean';
        }

        if (type === 'integer') {
            return typeof value === 'number' && Number.isInteger(value);
        }

        if (type === 'float') {
            return typeof value === 'number' && Number.isFinite(value);
        }

        if (type === 'string') {
            return typeof value === 'string';
        }

        if (type === 'vector2') {
            return isNumericArray(value, 2) || isObjectVector(value, ['x', 'y']);
        }

        if (type === 'vector3') {
            return isNumericArray(value, 3) || isObjectVector(value, ['x', 'y', 'z']);
        }

        if (type === 'vector4') {
            return isNumericArray(value, 4) || isObjectVector(value, ['x', 'y', 'z', 'w']);
        }

        if (type === 'quaternion') {
            return isNumericArray(value, 4) || isObjectVector(value, ['x', 'y', 'z', 'w']);
        }

        if (type === 'color') {
            return isNumericArray(value, 3) || isObjectVector(value, ['r', 'g', 'b']) || isObjectVector(value, ['x', 'y', 'z']);
        }

        if (type === 'entity') {
            // Accept plain string entity IDs and object forms such as {id: '...'}
            // or {kind: 'id', id: <number>} produced by BuildAttributeValueLiteral
            // for EntityQuery attributes.
            if (typeof value === 'string') {
                return true;
            }
            if (value !== null && typeof value === 'object' && !Array.isArray(value)) {
                const rawId = value.id;
                if (typeof rawId === 'string' && rawId.length > 0) {
                    return true;
                }
                if (typeof rawId === 'number' && Number.isInteger(rawId) && rawId > 0) {
                    return true;
                }
            }
            return false;
        }

        if (type === 'modelAsset') {
            if (value === null || typeof value !== 'object' || Array.isArray(value)) {
                return false;
            }

            return typeof value.assetCollectionId === 'string' && typeof value.assetId === 'string';
        }

        if (type === 'imageAsset') {
            if (value === null || typeof value !== 'object' || Array.isArray(value)) {
                return false;
            }

            return typeof value.assetCollectionId === 'string' && typeof value.imageAssetId === 'string';
        }

        return false;
    }

    function describeValueType(value) {
        if (value === null) {
            return 'null';
        }

        if (Array.isArray(value)) {
            return 'array';
        }

        if (typeof value === 'number') {
            return Number.isInteger(value) ? 'integer' : 'float';
        }

        return typeof value;
    }

    function readSchema(moduleRef, entityId) {
        if (!moduleRef) {
            return null;
        }

        let rawSchema;
        if (typeof moduleRef.schema !== 'undefined') {
            rawSchema = moduleRef.schema;
        } else if (typeof moduleRef.attributes !== 'undefined') {
            rawSchema = moduleRef.attributes;
        } else if (typeof moduleRef.codeComponentSchema !== 'undefined') {
            rawSchema = moduleRef.codeComponentSchema;
        } else {
            return null;
        }

        if (!isObject(rawSchema)) {
            warn(entityId, 'schema export exists but is not an object; ignoring schema.');
            return null;
        }

        const schema = {};
        for (const key of sortedKeys(rawSchema)) {
            const rawEntry = rawSchema[key];
            let rawType;
            let hasDefault = false;
            let defaultValue;
            let min;
            let max;
            let step;

            if (typeof rawEntry === 'string') {
                rawType = rawEntry;
            } else if (isObject(rawEntry)) {
                rawType = rawEntry.type;
                if (typeof rawType === 'undefined') {
                    rawType = rawEntry.valueType;
                }
                if (typeof rawType === 'undefined') {
                    rawType = rawEntry.propertyType;
                }

                if (Object.prototype.hasOwnProperty.call(rawEntry, 'default')) {
                    hasDefault = true;
                    defaultValue = rawEntry.default;
                } else if (Object.prototype.hasOwnProperty.call(rawEntry, 'defaultValue')) {
                    hasDefault = true;
                    defaultValue = rawEntry.defaultValue;
                } else if (Object.prototype.hasOwnProperty.call(rawEntry, 'value')) {
                    hasDefault = true;
                    defaultValue = rawEntry.value;
                }

                if (typeof rawEntry.min === 'number' && Number.isFinite(rawEntry.min)) {
                    min = rawEntry.min;
                }

                if (typeof rawEntry.max === 'number' && Number.isFinite(rawEntry.max)) {
                    max = rawEntry.max;
                }

                if (typeof rawEntry.step === 'number' && Number.isFinite(rawEntry.step) && rawEntry.step > 0) {
                    step = rawEntry.step;
                }
            } else {
                warn(entityId, `schema entry '${key}' is invalid and will be ignored.`);
                continue;
            }

            const type = normalizeSchemaType(rawType);
            if (!type) {
                warn(entityId, `schema entry '${key}' has unsupported type '${String(rawType)}' and will be ignored.`);
                continue;
            }

            if (hasDefault && !valueMatchesType(type, defaultValue)) {
                warn(entityId,
                    `schema entry '${key}' has default of type '${describeValueType(defaultValue)}' but expected '${type}'; default ignored.`);
                hasDefault = false;
                defaultValue = undefined;
            }

            schema[key] = { type, hasDefault, defaultValue, min, max, step };
        }

        return Object.keys(schema).length > 0 ? schema : null;
    }

    function buildSchemaMetadata(schema) {
        if (!schema) {
            return {};
        }

        const result = {};
        for (const key of sortedKeys(schema)) {
            const rule = schema[key];
            const entry = { type: rule.type };
            if (typeof rule.min === 'number') {
                entry.min = rule.min;
            }
            if (typeof rule.max === 'number') {
                entry.max = rule.max;
            }
            if (typeof rule.step === 'number') {
                entry.step = rule.step;
            }
            if (rule.hasDefault) {
                entry.default = cloneValue(rule.defaultValue);
            }
            result[key] = entry;
        }
        return result;
    }

    function getBuiltInDefaultValue(type) {
        if (type === 'boolean') {
            return false;
        }

        if (type === 'integer' || type === 'float') {
            return 0;
        }

        if (type === 'string') {
            return '';
        }

        if (type === 'vector2') {
            return [0, 0];
        }

        if (type === 'vector3') {
            return [0, 0, 0];
        }

        if (type === 'vector4') {
            return [0, 0, 0, 0];
        }

        if (type === 'quaternion') {
            return [0, 0, 0, 1];
        }

        if (type === 'color') {
            return [0, 0, 0];
        }

        if (type === 'entity') {
            return { id: '' };
        }

        if (type === 'modelAsset') {
            return {
                assetCollectionId: '',
                assetId: '',
            };
        }

        if (type === 'imageAsset') {
            return {
                assetCollectionId: '',
                imageAssetId: '',
            };
        }

        return undefined;
    }

    function reconcileAttributes(entityId, schema, incomingAttributes, previousAttributes) {
        const incoming = cloneAttributes(incomingAttributes);
        const previous = cloneAttributes(previousAttributes);

        if (!schema) {
            return incoming;
        }

        const next = {};

        for (const key of sortedKeys(incoming)) {
            if (key.startsWith('$')) {
                next[key] = cloneValue(incoming[key]);
            } else if (!Object.prototype.hasOwnProperty.call(schema, key)) {
                warn(entityId, `attribute '${key}' is not declared in schema and will be ignored.`);
            }
        }

        for (const key of sortedKeys(schema)) {
            const rule = schema[key];
            const hasIncoming = Object.prototype.hasOwnProperty.call(incoming, key);
            const hasPrevious = Object.prototype.hasOwnProperty.call(previous, key);

            if (hasIncoming) {
                const value = incoming[key];
                if (valueMatchesType(rule.type, value)) {
                    next[key] = cloneValue(value);
                    continue;
                }

                warn(entityId,
                    `attribute '${key}' has type '${describeValueType(value)}' but expected '${rule.type}'; value ignored.`);
            }

            if (hasPrevious && valueMatchesType(rule.type, previous[key])) {
                next[key] = cloneValue(previous[key]);
                continue;
            }

            if (rule.hasDefault) {
                next[key] = cloneValue(rule.defaultValue);
                continue;
            }

            const builtInDefault = getBuiltInDefaultValue(rule.type);
            if (typeof builtInDefault !== 'undefined') {
                next[key] = cloneValue(builtInDefault);
            }
        }

        return next;
    }

    function tryParseJSON(value) {
        if (typeof value !== 'string' || value.length === 0) {
            return null;
        }

        try {
            return JSON.parse(value);
        } catch (error) {
            return null;
        }
    }

    function resolveEntityRefState(entityId, previousState) {
        // Normalise entity ID: accept plain strings, {id: string} and
        // {kind: 'id', id: number} objects emitted by BuildAttributeValueLiteral
        // for EntityQuery attributes.
        let normalizedEntityId = entityId;
        if (normalizedEntityId !== null && typeof normalizedEntityId === 'object' && !Array.isArray(normalizedEntityId)) {
            const rawId = normalizedEntityId.id;
            if (typeof rawId === 'string') {
                normalizedEntityId = rawId;
            } else if (typeof rawId === 'number' && Number.isInteger(rawId) && rawId > 0) {
                normalizedEntityId = String(rawId);
            }
        }
        if (typeof normalizedEntityId !== 'string' || normalizedEntityId.length === 0) {
            return {
                id: null,
                status: 'unbound',
                snapshot: null,
            };
        }
        const resolvedEntityId = normalizedEntityId;

        let snapshot = null;
        if (globalThis.csp && typeof globalThis.csp.__getEntitySnapshot === 'function') {
            snapshot = tryParseJSON(globalThis.csp.__getEntitySnapshot(resolvedEntityId));
        }

        return {
            id: resolvedEntityId,
            status: snapshot ? 'resolved' : 'missing',
            snapshot,
        };
    }

    function createComponentRef(entityRefState, componentType, index = 0) {
        if (!entityRefState || entityRefState.status !== 'resolved' || !entityRefState.id) {
            return {
                status: 'missing',
                id: null,
                snapshot: null,
            };
        }

        if (!globalThis.csp || typeof globalThis.csp.__getComponentSnapshot !== 'function') {
            return {
                status: 'missing',
                id: null,
                snapshot: null,
            };
        }

        const componentSnapshot = tryParseJSON(globalThis.csp.__getComponentSnapshot(entityRefState.id, Number(componentType), Number(index)));
        if (!componentSnapshot) {
            return {
                status: 'missing',
                id: null,
                snapshot: null,
            };
        }

        return {
            status: 'resolved',
            id: componentSnapshot.componentId ?? null,
            snapshot: componentSnapshot,
        };
    }

    function toVector3(value) {
        if (Array.isArray(value) && value.length >= 3) {
            return [Number(value[0]), Number(value[1]), Number(value[2])];
        }

        if (isObject(value)) {
            return [Number(value.x), Number(value.y), Number(value.z)];
        }

        return null;
    }

    function toVector4(value) {
        if (Array.isArray(value) && value.length >= 4) {
            return [Number(value[0]), Number(value[1]), Number(value[2]), Number(value[3])];
        }

        if (isObject(value)) {
            return [Number(value.x), Number(value.y), Number(value.z), Number(value.w)];
        }

        return null;
    }

    function createScriptEntityRef(entityRefState) {
        function writePosition(value) {
            if (!globalThis.csp || typeof globalThis.csp.__setEntityPosition !== 'function') {
                return false;
            }

            if (!entityRefState.id) {
                return false;
            }

            const vector = toVector3(value);
            if (!vector || !vector.every((component) => Number.isFinite(component))) {
                return false;
            }

            const success = !!globalThis.csp.__setEntityPosition(entityRefState.id, vector[0], vector[1], vector[2]);
            if (success && entityRefState.snapshot) {
                entityRefState.snapshot.position = [...vector];
            }
            return success;
        }

        function writeRotation(value) {
            if (!globalThis.csp || typeof globalThis.csp.__setEntityRotation !== 'function') {
                return false;
            }

            if (!entityRefState.id) {
                return false;
            }

            const vector = toVector4(value);
            if (!vector || !vector.every((component) => Number.isFinite(component))) {
                return false;
            }

            const success = !!globalThis.csp.__setEntityRotation(entityRefState.id, vector[0], vector[1], vector[2], vector[3]);
            if (success && entityRefState.snapshot) {
                entityRefState.snapshot.rotation = [...vector];
            }
            return success;
        }

        function writeScale(value) {
            if (!globalThis.csp || typeof globalThis.csp.__setEntityScale !== 'function') {
                return false;
            }

            if (!entityRefState.id) {
                return false;
            }

            const vector = toVector3(value);
            if (!vector || !vector.every((component) => Number.isFinite(component))) {
                return false;
            }

            const success = !!globalThis.csp.__setEntityScale(entityRefState.id, vector[0], vector[1], vector[2]);
            if (success && entityRefState.snapshot) {
                entityRefState.snapshot.scale = [...vector];
            }
            return success;
        }

        function writePatch(patch) {
            if (!isObject(patch) || !entityRefState.id) {
                return false;
            }

            let didSucceed = true;

            if (Object.prototype.hasOwnProperty.call(patch, 'position')) {
                didSucceed = writePosition(patch.position) && didSucceed;
            }

            if (Object.prototype.hasOwnProperty.call(patch, 'rotation')) {
                didSucceed = writeRotation(patch.rotation) && didSucceed;
            }

            if (Object.prototype.hasOwnProperty.call(patch, 'scale')) {
                didSucceed = writeScale(patch.scale) && didSucceed;
            }

            if (Object.prototype.hasOwnProperty.call(patch, 'name')
                && globalThis.csp
                && typeof globalThis.csp.__setEntityName === 'function'
                && typeof patch.name === 'string') {
                const success = !!globalThis.csp.__setEntityName(entityRefState.id, patch.name);
                if (success && entityRefState.snapshot) {
                    entityRefState.snapshot.name = patch.name;
                }
                didSucceed = success && didSucceed;
            }

            if (Object.prototype.hasOwnProperty.call(patch, 'thirdPartyRef')
                && globalThis.csp
                && typeof globalThis.csp.__setEntityThirdPartyRef === 'function'
                && typeof patch.thirdPartyRef === 'string') {
                const success = !!globalThis.csp.__setEntityThirdPartyRef(entityRefState.id, patch.thirdPartyRef);
                if (success && entityRefState.snapshot) {
                    entityRefState.snapshot.thirdPartyRef = patch.thirdPartyRef;
                }
                didSucceed = success && didSucceed;
            }

            return didSucceed;
        }

        return {
            get id() {
                return entityRefState.id;
            },
            get status() {
                return entityRefState.status;
            },
            get snapshot() {
                return entityRefState.snapshot ? cloneValue(entityRefState.snapshot) : null;
            },
            patch(value) {
                return writePatch(value);
            },
            setPosition(value) {
                return writePosition(value);
            },
            setRotation(value) {
                return writeRotation(value);
            },
            setScale(value) {
                return writeScale(value);
            },
            getComponent(componentType, index = 0) {
                return createComponentRef(entityRefState, componentType, index);
            },
            getLightComponent(index = 0) {
                return createComponentRef(entityRefState, 10, index);
            },
        };
    }

    // --- Signal attribute management ---

    // Resolve a live entity object from TheEntitySystem for a given entity ID.
    // Returns null when the entity system is unavailable or the entity is not found.
    function resolveEntityFromSystem(entityId) {
        if (!entityId) {
            return null;
        }
        if (globalThis.TheEntitySystem && typeof globalThis.TheEntitySystem.getEntityById === 'function') {
            return globalThis.TheEntitySystem.getEntityById(entityId) ?? null;
        }
        return null;
    }

    function createSignalAttributes(entry, entityId) {
        const attrs = {};
        const schema = entry.schema || {};

        for (const key of sortedKeys(entry.attributes)) {
            if (key.startsWith('$')) {
                continue;
            }

            const rule = schema[key];
            if (rule && rule.type === 'entity') {
                const prevState = entry.entityRefStates[key] || { id: null, status: 'unbound', snapshot: null };
                const nextState = resolveEntityRefState(entry.attributes[key], prevState);
                entry.entityRefStates[key] = nextState;
                attrs[key] = signal(resolveEntityFromSystem(nextState.id));
            } else {
                attrs[key] = signal(entry.attributes[key]);
            }
        }

        return attrs;
    }

    function updateSignalAttributes(entry, entityId) {
        const schema = entry.schema || {};

        batch(() => {
            for (const key of sortedKeys(entry.attributes)) {
                if (key.startsWith('$')) {
                    continue;
                }

                const rule = schema[key];

                if (!entry.signalAttributes[key]) {
                    if (rule && rule.type === 'entity') {
                        const state = resolveEntityRefState(entry.attributes[key],
                            entry.entityRefStates[key] || { id: null, status: 'unbound', snapshot: null });
                        entry.entityRefStates[key] = state;
                        entry.signalAttributes[key] = signal(resolveEntityFromSystem(state.id));
                    } else {
                        entry.signalAttributes[key] = signal(entry.attributes[key]);
                    }
                } else {
                    if (rule && rule.type === 'entity') {
                        const prevState = entry.entityRefStates[key] || { id: null, status: 'unbound', snapshot: null };
                        const nextState = resolveEntityRefState(entry.attributes[key], prevState);
                        entry.entityRefStates[key] = nextState;
                        entry.signalAttributes[key].value = resolveEntityFromSystem(nextState.id);
                    } else {
                        entry.signalAttributes[key].value = entry.attributes[key];
                    }
                }
            }
        });
    }

    // --- UI tree management ---

    function flattenUIChildren(children, out = []) {
        for (const child of Array.isArray(children) ? children : []) {
            if (Array.isArray(child)) {
                flattenUIChildren(child, out);
            } else if (child !== null && typeof child !== 'undefined' && child !== false) {
                out.push(child);
            }
        }
        return out;
    }

    function normalizeUIEntityId(value) {
        if (typeof value === 'string') {
            return value;
        }
        if (value && typeof value === 'object') {
            if (typeof value.id === 'string') {
                return value.id;
            }
            if (typeof value.id === 'number' && Number.isInteger(value.id) && value.id > 0) {
                return String(value.id);
            }
        }
        return null;
    }

    const UI_TREE_MAX_DEPTH = 256;

    function normalizeUINode(node, entityId, handlerMap, path = 'root', depth = 0) {
        if (depth > UI_TREE_MAX_DEPTH) {
            console.error(`[NGX UI][runtime] normalizeUINode exceeded max depth (${UI_TREE_MAX_DEPTH}) at path '${path}' — bailing out (possible cyclic or runaway tree).`);
            return null;
        }

        if (!node || typeof node !== 'object') {
            return null;
        }

        const type = typeof node.type === 'string' ? node.type : null;
        if (!type) {
            return null;
        }

        const props = isObject(node.props) ? { ...node.props } : {};
        const normalized = {
            type,
            id: (typeof props.key === 'string' && props.key.length > 0) ? `${path}:${props.key}` : path,
            props: {},
            children: [],
        };

        for (const [key, value] of Object.entries(props)) {
            if (key === 'key') {
                continue;
            }

            if (typeof value === 'function') {
                if (key === 'onClick') {
                    const handlerId = `${path}:click`;
                    handlerMap.set(handlerId, value);
                    normalized.props.onClickHandlerId = handlerId;
                    continue;
                } else if (key === 'onHover') {
                    const handlerId = `${path}:hover`;
                    handlerMap.set(handlerId, value);
                    normalized.props.onHoverHandlerId = handlerId;
                    continue;
                } else if (key === 'onPointerEnter') {
                    const handlerId = `${path}:pointerEnter`;
                    handlerMap.set(handlerId, value);
                    normalized.props.onPointerEnterHandlerId = handlerId;
                    continue;
                } else if (key === 'onPointerLeave') {
                    const handlerId = `${path}:pointerLeave`;
                    handlerMap.set(handlerId, value);
                    normalized.props.onPointerLeaveHandlerId = handlerId;
                    continue;
                } else if (key === 'onPointerDown') {
                    const handlerId = `${path}:pointerDown`;
                    handlerMap.set(handlerId, value);
                    normalized.props.onPointerDownHandlerId = handlerId;
                    continue;
                } else if (key === 'onPointerUp') {
                    const handlerId = `${path}:pointerUp`;
                    handlerMap.set(handlerId, value);
                    normalized.props.onPointerUpHandlerId = handlerId;
                    continue;
                } else if (key === 'onDrag') {
                    const handlerId = `${path}:drag`;
                    handlerMap.set(handlerId, value);
                    normalized.props.onDragHandlerId = handlerId;
                    continue;
                }
                continue;
            }

            if (key === 'targetEntity' || key === 'entity') {
                const normalizedEntity = normalizeUIEntityId(value);
                if (normalizedEntity) {
                    normalized.props.targetEntityId = normalizedEntity;
                }
                continue;
            }

            if (Array.isArray(value)) {
                normalized.props[key] = value.map((item) => cloneValue(item));
            } else if (isObject(value)) {
                normalized.props[key] = cloneValue(value);
            } else if (typeof value !== 'undefined') {
                normalized.props[key] = value;
            }
        }

        if (type === 'world' && !normalized.props.targetEntityId) {
            normalized.props.targetEntityId = entityId;
        }

        const children = flattenUIChildren(node.children);
        normalized.children = children
            .map((child, index) => normalizeUINode(child, entityId, handlerMap, `${normalized.id}.${index}`, depth + 1))
            .filter((child) => child !== null);

        return normalized;
    }

    function clearUIHandlers(entry) {
        entry.uiHandlers.clear();
        if (entry.pendingUIHandlers) {
            entry.pendingUIHandlers.clear();
        }
    }

    function unmountUI(entityId) {
        if (globalThis.csp && typeof globalThis.csp.__uiUnmount === 'function') {
            try {
                globalThis.csp.__uiUnmount(entityId);
            } catch (error) {
                console.error(`NgxCodeComponentRuntime: failed to unmount UI for entity ${entityId}`, error);
            }
        }
    }

    function queueUIFlush(entityId, entry, treeJson, handlerMap) {
        if (!entry || entry.isDisposing || isDestroying) {
            return;
        }

        entry.pendingUITreeJson = treeJson;
        entry.pendingUIHandlers = handlerMap;
        entry.uiDirty = true;
        pendingUIFlushes.add(entityId);
    }

    function flushPendingUI(entityId, entry) {
        if (!entry || !entry.uiDirty || entry.isDisposing || isDestroying) {
            if (entry) {
                entry.uiDirty = false;
                entry.pendingUITreeJson = null;
                if (entry.pendingUIHandlers) {
                    entry.pendingUIHandlers.clear();
                }
            }
            pendingUIFlushes.delete(entityId);
            return;
        }

        entry.uiDirty = false;
        pendingUIFlushes.delete(entityId);

        entry.uiHandlers = entry.pendingUIHandlers instanceof Map ? entry.pendingUIHandlers : new Map();
        entry.pendingUIHandlers = new Map();

        const treeJson = typeof entry.pendingUITreeJson === 'string' ? entry.pendingUITreeJson : null;
        if (!treeJson) {
            unmountUI(entityId);
            return;
        }

        if (globalThis.csp && typeof globalThis.csp.__uiMount === 'function') {
            try {
                globalThis.csp.__uiMount(entityId, treeJson);
            } catch (error) {
                console.error(`NgxCodeComponentRuntime: failed to mount UI for entity ${entityId}`, error);
                entry.uiHandlers.clear();
                unmountUI(entityId);
            }
        }
    }

    function startUIRuntime(entityId, entry) {
        if (!entry || entry.isDisposing || isDestroying || !entry.module || typeof entry.module.ui !== 'function') {
            return;
        }

        if (typeof entry.uiDispose === 'function') {
            entry.uiDispose();
            entry.uiDispose = null;
        }

        entry.uiDispose = effect(() => {
            if (entry.isDisposing || isDestroying) {
                return;
            }

            const previousEntityId = globalThis.__cspCurrentEntityId;
            const previousUiRenderActive = globalThis.__cspUiRenderActive;
            entry.uiEffectCursor = 0;
            entry.pendingUiEffectCallbacks = [];
            globalThis.__cspCurrentEntityId = entityId;
            globalThis.__cspUiRenderActive = true;
            try {
                const nextHandlers = new Map();
                const rawTree = entry.module.ui({ attributes: entry.signalAttributes, thisEntity: entry.thisEntitySignal, entityId });
                const normalizedTree = normalizeUINode(rawTree, entityId, nextHandlers);

                if (!normalizedTree) {
                    queueUIFlush(entityId, entry, null, nextHandlers);
                    return;
                }

                let serializedTree = null;
                try {
                    serializedTree = JSON.stringify(normalizedTree);
                } catch (stringifyError) {
                    const detail = stringifyError instanceof Error
                        ? (typeof stringifyError.stack === 'string' && stringifyError.stack.length > 0 ? stringifyError.stack : String(stringifyError))
                        : String(stringifyError);
                    console.error(`[NGX UI][runtime] JSON.stringify(normalizedTree) failed for entity ${entityId}. Tree shape: type='${normalizedTree.type}', id='${normalizedTree.id}', childCount=${Array.isArray(normalizedTree.children) ? normalizedTree.children.length : 'n/a'}, propKeys=${JSON.stringify(Object.keys(normalizedTree.props || {}))}. Error: ${detail}`);
                    queueUIFlush(entityId, entry, null, new Map());
                    return;
                }

                queueUIFlush(entityId, entry, serializedTree, nextHandlers);
            } catch (error) {
                console.error(`NgxCodeComponentRuntime: ui() failed for entity ${entityId}`, error);
                queueUIFlush(entityId, entry, null, new Map());
            } finally {
                reconcileUiEffects(entityId, entry);
                globalThis.__cspUiRenderActive = previousUiRenderActive;
                globalThis.__cspCurrentEntityId = previousEntityId;
            }
        });
    }

    function stopUIRuntime(entityId, entry) {
        if (!entry) {
            return;
        }

        if (typeof entry.uiDispose === 'function') {
            try {
                entry.uiDispose();
            } catch (error) {
                console.error(`NgxCodeComponentRuntime: ui dispose failed for entity ${entityId}`, error);
            }
        }
        entry.uiDispose = null;
        pendingUIFlushes.delete(entityId);
        entry.uiDirty = false;
        entry.pendingUITreeJson = null;
        disposeUiEffects(entry);
        clearUIHandlers(entry);
        unmountUI(entityId);
    }

    function dispatchUIAction(entityId, handlerId, eventData) {
        const entry = codeComponents.get(entityId);
        console.log('[NGX UI][runtime] dispatchUIAction called', {
            entityId,
            handlerId,
            hasEntry: !!entry,
            isDisposing: entry && entry.isDisposing,
            isDestroying,
            hasUiHandlers: !!(entry && entry.uiHandlers),
            uiHandlerCount: entry && entry.uiHandlers ? entry.uiHandlers.size : 0,
            registeredHandlerIds: entry && entry.uiHandlers ? Array.from(entry.uiHandlers.keys()) : []
        });
        if (!entry || entry.isDisposing || isDestroying || !entry.uiHandlers || typeof handlerId !== 'string') {
            console.log('[NGX UI][runtime] dispatchUIAction early-exit (entry/handlers missing)');
            return false;
        }

        const handler = entry.uiHandlers.get(handlerId);
        if (typeof handler !== 'function') {
            console.log('[NGX UI][runtime] dispatchUIAction handler not found', {
                handlerId,
                registeredHandlerIds: Array.from(entry.uiHandlers.keys())
            });
            return false;
        }

        try {
            const previousEntityId = globalThis.__cspCurrentEntityId;
            globalThis.__cspCurrentEntityId = entityId;
            try {
                batch(() => {
                    handler({
                        entityId,
                        type: eventData && eventData.type ? eventData.type : 'click',
                        thisEntity: entry.thisEntitySignal ? entry.thisEntitySignal.value : null,
                        ...(eventData ? eventData : {})
                    });
                });
            } finally {
                globalThis.__cspCurrentEntityId = previousEntityId;
            }
            return true;
        } catch (error) {
            console.error(`NgxCodeComponentRuntime: UI handler failed for entity ${entityId}`, error);
            return false;
        }
    }

    // --- Effect lifecycle ---

    function disposeAllEffects(entry) {
        if (!entry) {
            return;
        }

        const effects = Array.isArray(entry.effects) ? [...entry.effects] : [];
        entry.effects = [];

        for (const dispose of effects) {
            try {
                dispose();
            } catch (e) {
                console.error('NgxCodeComponentRuntime: effect dispose failed', e);
            }
        }
    }

    function disposeUiEffects(entry) {
        if (!entry) {
            return;
        }

        const slots = Array.isArray(entry.uiEffects) ? [...entry.uiEffects] : [];
        entry.uiEffects = [];
        entry.pendingUiEffectCallbacks = [];
        entry.uiEffectCursor = 0;

        for (const slot of slots) {
            if (!slot || typeof slot.dispose !== 'function') {
                continue;
            }

            try {
                slot.dispose();
            } catch (e) {
                console.error('NgxCodeComponentRuntime: ui effect dispose failed', e);
            }
        }
    }

    function reconcileUiEffects(entityId, entry) {
        if (!entry) {
            return;
        }

        const nextCallbacks = Array.isArray(entry.pendingUiEffectCallbacks) ? entry.pendingUiEffectCallbacks : [];
        const previousSlots = Array.isArray(entry.uiEffects) ? entry.uiEffects : [];
        const nextSlots = [];

        for (let index = 0; index < nextCallbacks.length; index += 1) {
            const callback = nextCallbacks[index];
            const previousSlot = previousSlots[index];

            if (previousSlot && typeof previousSlot.dispose === 'function') {
                previousSlot.callback = callback;
                nextSlots.push(previousSlot);
                continue;
            }

            const slot = { callback, dispose: null };
            slot.dispose = effect(() => {
                const previousEntityId = globalThis.__cspCurrentEntityId;
                globalThis.__cspCurrentEntityId = entityId;
                try {
                    return slot.callback();
                } finally {
                    globalThis.__cspCurrentEntityId = previousEntityId;
                }
            });

            nextSlots.push(slot);
        }

        for (let index = nextCallbacks.length; index < previousSlots.length; index += 1) {
            const previousSlot = previousSlots[index];
            if (!previousSlot || typeof previousSlot.dispose !== 'function') {
                continue;
            }

            try {
                previousSlot.dispose();
            } catch (e) {
                console.error('NgxCodeComponentRuntime: ui effect dispose failed', e);
            }
        }

        entry.uiEffects = nextSlots;
        entry.pendingUiEffectCallbacks = [];
        entry.uiEffectCursor = 0;
    }

    function removeInputListenersForEntity(entityId) {
        if (globalThis.__cspInput && typeof globalThis.__cspInput.removeListenersForEntity === 'function') {
            globalThis.__cspInput.removeListenersForEntity(entityId);
        }
    }

    function runScriptTeardown(entityId, entry, reason = 'unspecified') {
        if (!entry) {
            return;
        }

        const teardown = entry.teardown;
        entry.teardown = null;

        try {
            if (typeof teardown === 'function') {
                teardown();
            }
        } catch (error) {
            const scriptPath = entry && entry.scriptAssetPath ? entry.scriptAssetPath : '<unknown>';
            console.error(`NgxCodeComponentRuntime: script teardown failed for entity ${entityId} (${scriptPath}) [${reason}]`, error);
        } finally {
            removeInputListenersForEntity(entityId);
        }
    }

    // --- Script initialization ---

    function ensureThisEntitySignal(entityId, entry) {
        if (!entry) {
            return null;
        }

        const liveEntity = (globalThis.TheEntitySystem && typeof globalThis.TheEntitySystem.getEntityById === 'function')
            ? globalThis.TheEntitySystem.getEntityById(entityId)
            : null;

        if (!entry.thisEntitySignal) {
            entry.thisEntitySignal = signal(liveEntity ?? null);
        } else {
            entry.thisEntitySignal.value = liveEntity ?? null;
        }

        return entry.thisEntitySignal;
    }

    function initializeScript(entityId, entry) {
        if (!entry || !entry.module || typeof entry.module.script !== 'function') {
            return;
        }

        try {
            ensureThisEntitySignal(entityId, entry);

            globalThis.__cspCurrentEntityId = entityId;
            try {
                const teardown = entry.module.script({ attributes: entry.signalAttributes, thisEntity: entry.thisEntitySignal, entityId });
                entry.teardown = typeof teardown === 'function' ? teardown : null;
            } finally {
                globalThis.__cspCurrentEntityId = undefined;
            }
        } catch (error) {
            const scriptPath = entry && entry.scriptAssetPath ? entry.scriptAssetPath : '<unknown>';
            const detail = error instanceof Error
                ? (typeof error.stack === 'string' && error.stack.length > 0 ? error.stack : String(error))
                : String(error);
            console.error(`NgxCodeComponentRuntime: script initialization failed for entity ${entityId} (${scriptPath})\n${detail}`);
        }
    }

    function activateImportedModule(entityId, entry) {
        if (!entry || !entry.module || entry.isDisposing || isDestroying) {
            return;
        }

        if (entry.pendingSchemaSync) {
            syncCodeComponentSchema(entityId);
            pendingSchemaSyncs.add(entityId);
        }

        entry.attributes = reconcileAttributes(entityId, entry.schema, entry.attributes, entry.attributes);
        entry.signalAttributes = createSignalAttributes(entry, entityId);
        ensureThisEntitySignal(entityId, entry);
        entry.initialized = true;

        initializeScript(entityId, entry);
        startUIRuntime(entityId, entry);
    }

    // --- Module import with retry ---

    function tryImportCodeComponentModule(entityId, entry, reason = 'unspecified') {
        if (!entry || typeof entry.scriptAssetPath !== 'string' || entry.scriptAssetPath.length === 0) {
            return false;
        }

        if (entry.importInFlight) {
            return false;
        }

        const scriptAssetPath = entry.scriptAssetPath;
        const importGeneration = (typeof entry.importGeneration === 'number' ? entry.importGeneration : 0) + 1;
        entry.importGeneration = importGeneration;
        entry.importInFlight = true;

        import(scriptAssetPath)
            .then((moduleRef) => {
                const current = codeComponents.get(entityId);
                if (!current
                    || current.scriptAssetPath !== scriptAssetPath
                    || current.importGeneration !== importGeneration) {
                    return;
                }

                current.importInFlight = false;
                current.retryDelayFrames = 1;
                current.retryCountdown = 0;
                current.hasWarnedMissingModule = false;
                current.activationFailed = false;
                current.module = moduleRef;

                // If reinitializing (e.g. after a hot-reload), tear down any
                // old effects so they don't accumulate in memory.
                if (current.initialized) {
                    current.isDisposing = true;
                    stopUIRuntime(entityId, current);
                    disposeAllEffects(current);
                    current.thisEntitySignal = null;
                    current.isDisposing = false;
                    current.initialized = false;
                }
                pendingModuleActivations.add(entityId);
            })
            .catch((error) => {
                const current = codeComponents.get(entityId);
                if (!current
                    || current.scriptAssetPath !== scriptAssetPath
                    || current.importGeneration !== importGeneration) {
                    return;
                }

                current.importInFlight = false;
                const delayFrames = typeof current.retryDelayFrames === 'number' && current.retryDelayFrames > 0
                    ? Math.min(current.retryDelayFrames * 2, 300)
                    : 1;
                current.retryDelayFrames = delayFrames;
                current.retryCountdown = delayFrames;

                const errorMessage = (error && typeof error.message === 'string') ? error.message : String(error);
                if (errorMessage.includes('module not found') && !current.hasWarnedMissingModule) {
                    current.hasWarnedMissingModule = true;
                    warn(entityId, `module '${scriptAssetPath}' is not available yet; import will be retried automatically.`);
                }
                console.error(`NgxCodeComponentRuntime: Failed to import module ${scriptAssetPath} (${reason})`, error);
            });

        return true;
    }

    // --- Public API (called from C++) ---

    function syncCodeComponentSchema(entityId) {
        const entry = codeComponents.get(entityId);
        if (!entry) {
            return {};
        }

        if (entry.activationFailed) {
            return buildSchemaMetadata(entry.schema);
        }

        if (!entry.module) {
            entry.pendingSchemaSync = true;
            tryImportCodeComponentModule(entityId, entry, 'syncCodeComponentSchema');
            return {};
        }

        const previousAttributes = cloneAttributes(entry.attributes);
        entry.schema = readSchema(entry.module, entityId);
        entry.attributes = reconcileAttributes(entityId, entry.schema, entry.attributes, previousAttributes);
        entry.pendingSchemaSync = false;

        if (entry.initialized && entry.signalAttributes) {
            updateSignalAttributes(entry, entityId);
        }

        return buildSchemaMetadata(entry.schema);
    }

    function addCodeComponent(entityId, payload = {}) {
        const payloadObject = isObject(payload) ? payload : {};
        const scriptAssetPath = typeof payloadObject.scriptAssetPath === 'string' ? payloadObject.scriptAssetPath : '';
        const inputAttributes = cloneAttributes(payloadObject.attributes);
        const existing = codeComponents.get(entityId);

        if (existing && existing.scriptAssetPath === scriptAssetPath) {
            syncCodeComponentAttributes(entityId, inputAttributes);
            if (!existing.module && scriptAssetPath.length > 0) {
                tryImportCodeComponentModule(entityId, existing, 'addCodeComponent-existing');
            }
            return true;
        }

        if (existing) {
            stopUIRuntime(entityId, existing);
            disposeAllEffects(existing);
            existing.thisEntitySignal = null;
            pendingModuleActivations.delete(entityId);
            codeComponents.delete(entityId);
        }

        const entry = {
            scriptAssetPath,
            attributes: inputAttributes,
            signalAttributes: {},
            thisEntitySignal: null,
            teardown: null,
            module: null,
            schema: null,
            initialized: false,
            pendingSchemaSync: true,
            importGeneration: 0,
            importInFlight: false,
            retryDelayFrames: 1,
            retryCountdown: 0,
            hasWarnedMissingModule: false,
            activationFailed: false,
            entityRefStates: {},
            effects: [],
            uiDispose: null,
            uiHandlers: new Map(),
            pendingUIHandlers: new Map(),
            pendingUITreeJson: null,
            uiEffects: [],
            pendingUiEffectCallbacks: [],
            uiEffectCursor: 0,
            uiDirty: false,
            isDisposing: false,
        };

        codeComponents.set(entityId, entry);

        if (!scriptAssetPath) {
            warn(entityId, 'missing scriptAssetPath; component will not be initialized.');
            return false;
        }

        tryImportCodeComponentModule(entityId, entry, 'addCodeComponent');
        return true;
    }

    function syncCodeComponentAttributes(entityId, attributes = {}) {
        const entry = codeComponents.get(entityId);
        if (!entry) {
            return {};
        }

        if (entry.activationFailed) {
            return cloneAttributes(entry.attributes);
        }

        const previousAttributes = cloneAttributes(entry.attributes);
        entry.attributes = reconcileAttributes(entityId, entry.schema, attributes, previousAttributes);

        if (entry.initialized && entry.signalAttributes) {
            updateSignalAttributes(entry, entityId);
        }

        return cloneAttributes(entry.attributes);
    }

    function updateAttributeForEntity(entityId, key, value) {
        const entry = codeComponents.get(entityId);
        if (!entry) {
            return;
        }

        if (entry.activationFailed) {
            return;
        }

        if (entry.schema) {
            if (!Object.prototype.hasOwnProperty.call(entry.schema, key)) {
                warn(entityId, `attribute '${key}' is not declared in schema and will be ignored.`);
                return;
            }

            const rule = entry.schema[key];
            if (!valueMatchesType(rule.type, value)) {
                warn(entityId, `attribute '${key}' has type '${describeValueType(value)}' but expected '${rule.type}'; update ignored.`);
                return;
            }
        }

        entry.attributes[key] = cloneValue(value);

        if (entry.initialized && entry.signalAttributes && entry.signalAttributes[key]) {
            const schema = entry.schema || {};
            const rule = schema[key];
            if (rule && rule.type === 'entity') {
                const prevState = entry.entityRefStates[key] || { id: null, status: 'unbound', snapshot: null };
                const nextState = resolveEntityRefState(value, prevState);
                entry.entityRefStates[key] = nextState;
                entry.signalAttributes[key].value = resolveEntityFromSystem(nextState.id);
            } else {
                entry.signalAttributes[key].value = value;
            }
        }
    }

    function removeCodeComponent(entityId) {
        const entry = codeComponents.get(entityId);
        if (!entry) {
            return;
        }

        entry.isDisposing = true;

        // Dispose reactive effects before invalidating thisEntity so teardown
        // does not trigger one final rerun against a null entity.
        stopUIRuntime(entityId, entry);
        runScriptTeardown(entityId, entry, 'remove');
        disposeAllEffects(entry);
        entry.thisEntitySignal = null;
        pendingModuleActivations.delete(entityId);
        codeComponents.delete(entityId);
    }

    function tick() {
        for (const [entityId, entry] of codeComponents) {
            if (pendingModuleActivations.has(entityId)) {
                pendingModuleActivations.delete(entityId);
                try {
                    activateImportedModule(entityId, entry);
                } catch (error) {
                    entry.activationFailed = true;
                    const scriptPath = entry && entry.scriptAssetPath ? entry.scriptAssetPath : '<unknown>';
                    const detail = error instanceof Error
                        ? (typeof error.stack === 'string' && error.stack.length > 0 ? error.stack : String(error))
                        : String(error);
                    console.error(`NgxCodeComponentRuntime: module activation failed for entity ${entityId} (${scriptPath})\n${detail}`);
                    continue;
                }
            }

            if (entry.activationFailed) {
                continue;
            }

            if (!entry.module && typeof entry.scriptAssetPath === 'string' && entry.scriptAssetPath.length > 0) {
                if (entry.importInFlight) {
                    continue;
                }

                if (typeof entry.retryCountdown === 'number' && entry.retryCountdown > 0) {
                    entry.retryCountdown -= 1;
                    continue;
                }

                tryImportCodeComponentModule(entityId, entry, 'tick-retry');
            }
        }

        const dirtyUIIds = Array.from(pendingUIFlushes);
        for (const entityId of dirtyUIIds) {
            const entry = codeComponents.get(entityId);
            if (!entry || !entry.initialized) {
                pendingUIFlushes.delete(entityId);
                continue;
            }

            flushPendingUI(entityId, entry);
        }
    }

    function drainPendingSchemaSyncs() {
        const ids = Array.from(pendingSchemaSyncs);
        pendingSchemaSyncs.clear();
        return ids;
    }

    function destroy() {
        isDestroying = true;
        for (const [entityId, entry] of codeComponents) {
            entry.isDisposing = true;
            stopUIRuntime(entityId, entry);
            runScriptTeardown(entityId, entry, 'destroy');
            disposeAllEffects(entry);
            entry.thisEntitySignal = null;
        }
        codeComponents.clear();
        pendingSchemaSyncs.clear();
        pendingModuleActivations.clear();
        pendingUIFlushes.clear();
        isDestroying = false;
    }

    return {
        syncCodeComponentSchema,
        addCodeComponent,
        syncCodeComponentAttributes,
        updateAttributeForEntity,
        removeCodeComponent,
        dispatchUIAction,
        tick,
        drainPendingSchemaSyncs,
        destroy,
    };
}

export default createScriptRegistry;
)";
constexpr const char* CODECOMPONENT_BOOTSTRAP_SOURCE = R"(
import * as __ngxRegistryModule from '/__csp/internal/codecomponent/registry.js';
const __ngxFactory = (typeof __ngxRegistryModule.createScriptRegistry === 'function')
    ? __ngxRegistryModule.createScriptRegistry
    : ((typeof __ngxRegistryModule.default === 'function') ? __ngxRegistryModule.default : null);

if (typeof globalThis.scriptRegistry === 'undefined') {
    if (__ngxFactory !== null) {
        globalThis.scriptRegistry = __ngxFactory();
    } else if (__ngxRegistryModule && typeof __ngxRegistryModule.default === 'object') {
        globalThis.scriptRegistry = __ngxRegistryModule.default;
    }
}
)";

constexpr const char* CODECOMPONENT_SHUTDOWN_SOURCE = R"(
if (typeof globalThis.scriptRegistry !== 'undefined') {
    if (typeof globalThis.scriptRegistry.destroy === 'function') {
        globalThis.scriptRegistry.destroy();
    }
    delete globalThis.scriptRegistry;
}
)";

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
    bool bAllOperationsApplied = true;

    for (const auto& PreviousEntityPair : LastEntitySnapshots)
    {
        if (CurrentSnapshots.find(PreviousEntityPair.first) == CurrentSnapshots.end())
        {
            bAllOperationsApplied = RemoveEntityFromRegistry(PreviousEntityPair.first) && bAllOperationsApplied;
        }
    }

    for (const auto& CurrentEntityPair : CurrentSnapshots)
    {
        const uint64_t EntityId = CurrentEntityPair.first;
        const CodeComponentSnapshot& CurrentSnapshot = CurrentEntityPair.second;

        const auto PreviousSnapshotIt = LastEntitySnapshots.find(EntityId);
        if (PreviousSnapshotIt == LastEntitySnapshots.end())
        {
            bAllOperationsApplied = AddOrReplaceEntityInRegistry(EntityId, CurrentSnapshot) && bAllOperationsApplied;
            continue;
        }

        const CodeComponentSnapshot& PreviousSnapshot = PreviousSnapshotIt->second;
        if ((CurrentSnapshot.ScriptAssetPath != PreviousSnapshot.ScriptAssetPath) || (CurrentSnapshot.ScopeType != PreviousSnapshot.ScopeType)
            || (CurrentSnapshot.RuntimeMode != PreviousSnapshot.RuntimeMode))
        {
            bAllOperationsApplied = AddOrReplaceEntityInRegistry(EntityId, CurrentSnapshot) && bAllOperationsApplied;
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
            bAllOperationsApplied = SyncAttributesInRegistry(EntityId, CurrentSnapshot) && bAllOperationsApplied;
            continue;
        }

        for (const auto& CurrentAttributePair : CurrentSnapshot.Attributes)
        {
            const auto PreviousAttributeIt = PreviousSnapshot.Attributes.find(CurrentAttributePair.first);
            if ((PreviousAttributeIt == PreviousSnapshot.Attributes.end()) || (PreviousAttributeIt->second != CurrentAttributePair.second))
            {
                bAllOperationsApplied
                    = UpdateAttributeInRegistry(EntityId, CurrentAttributePair.first, CurrentAttributePair.second) && bAllOperationsApplied;
            }
        }
    }

    if (bAllOperationsApplied)
    {
        LastEntitySnapshots = CurrentSnapshots;
    }
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
        return GetRuntimeMode() == csp::systems::ESpaceRuntimeMode::Server;

    default:
        return false;
    }
}

bool NgxCodeComponentRuntime::ExecuteRegistrySnippet(const std::string& Snippet, const char* DebugName) const
{
    if (!ScriptSystem.EvaluateSnippet(Snippet.c_str(), DebugName))
    {
        if (ScriptSystem.WasLastEvaluationDeferred())
        {
            return false;
        }

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

bool NgxCodeComponentRuntime::SyncAttributesInRegistry(uint64_t EntityId, const CodeComponentSnapshot& Snapshot)
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

    return ExecuteRegistrySnippet(Snippet, "<ngx-codecomponent-sync-attributes>");
}

bool NgxCodeComponentRuntime::AddOrReplaceEntityInRegistry(uint64_t EntityId, const CodeComponentSnapshot& Snapshot)
{
    const std::string EntityIdString = std::to_string(EntityId);
    const std::string AttributesLiteral = BuildAttributesObjectLiteral(Snapshot.Attributes);
    const std::string Snippet = "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.addCodeComponent === 'function') {\n"
                                "    globalThis.scriptRegistry.addCodeComponent('"
        + EscapeJSStringLiteral(EntityIdString) + "', { scriptAssetPath: '" + EscapeJSStringLiteral(Snapshot.ScriptAssetPath)
        + "', attributes: " + AttributesLiteral + " });\n" + "}\n";

    if (!ExecuteRegistrySnippet(Snippet, "<ngx-codecomponent-add>"))
    {
        return false;
    }
    SyncSchemaInRegistry(EntityId);
    return SyncAttributesInRegistry(EntityId, Snapshot);
}

bool NgxCodeComponentRuntime::UpdateAttributeInRegistry(uint64_t EntityId, const std::string& Key, const csp::multiplayer::CodeAttribute& Attribute)
{
    const std::string EntityIdString = std::to_string(EntityId);
    const std::string AttributeLiteral = BuildAttributeValueLiteral(Attribute);
    const std::string Snippet = "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.updateAttributeForEntity === 'function') {\n"
                                "    globalThis.scriptRegistry.updateAttributeForEntity('"
        + EscapeJSStringLiteral(EntityIdString) + "', '" + EscapeJSStringLiteral(Key) + "', " + AttributeLiteral + ");\n" + "}\n";

    return ExecuteRegistrySnippet(Snippet, "<ngx-codecomponent-update>");
}

bool NgxCodeComponentRuntime::RemoveEntityFromRegistry(uint64_t EntityId)
{
    const std::string EntityIdString = std::to_string(EntityId);
    const std::string Snippet = "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.removeCodeComponent === 'function') {\n"
                                "    globalThis.scriptRegistry.removeCodeComponent('"
        + EscapeJSStringLiteral(EntityIdString) + "');\n" + "}\n";

    return ExecuteRegistrySnippet(Snippet, "<ngx-codecomponent-remove>");
}

} // namespace csp::systems
