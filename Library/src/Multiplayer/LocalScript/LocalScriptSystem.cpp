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
#include "CSP/Multiplayer/LocalScript/LocalScriptSystem.h"
#include "CSP/Common/StringFormat.h"
#include "CSP/Multiplayer/LocalScript/signals.h"
#include "CSP/Multiplayer/Script/EntityScriptMessages.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "Multiplayer/Script/EntityScriptBinding.h"
#include "quickjspp.hpp"
#include <async++.h>

using namespace std;

namespace csp::systems
{

LocalScriptSystem::LocalScriptSystem(csp::multiplayer::SpaceEntitySystem* InEntitySystem)
    : EntitySystem(InEntitySystem)
{
    this->SpaceId = "";
    Runtime = nullptr;
    Context = nullptr;
    ScriptBinding = nullptr;
}

LocalScriptSystem::~LocalScriptSystem()
{
    delete Context;
    delete Runtime;
    delete ScriptBinding;
}

void LocalScriptSystem::Initialize()
{
    CSP_LOG_MSG(csp::systems::LogLevel::Log, "LocalScriptSystem cleanup");
    try
    {
        if (ScriptBinding != nullptr)
        {
            delete ScriptBinding;
            ScriptBinding = nullptr;
        }
        if (Context != nullptr)
        {
            delete Context;
            Context = nullptr;
        }
        if (Runtime != nullptr)
        {
            delete Runtime;
            Runtime = nullptr;
        }
        Runtime = new qjs::Runtime();
        Context = new qjs::Context(*Runtime);
        ScriptBinding = new csp::multiplayer::EntityScriptBinding(EntitySystem);
        // Set the custom module loader on the context
        Context->moduleLoader = [this](std::string_view filename) -> qjs::Context::ModuleData
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "Loading module: %s", filename.data());
            csp::common::String Url = csp::common::String(filename.data(), filename.length());
            if (LoadedScripts.HasKey(Url.c_str()))
            {
                const auto& source_code = LoadedScripts[Url];
                return qjs::Context::ModuleData { Url.c_str(), source_code.c_str(), std::nullopt };
            }
            if (Url == "@preact/signals-core")
            {
                // Return the CSP signals module
                return qjs::Context::ModuleData { Url.c_str(), csp::systems::SignalsScriptCode.c_str(), std::nullopt };
            }
            // Return empty module if not found
            return qjs::Context::ModuleData { Url.c_str(), "", std::nullopt };
        };

        // Built in library
        qjs::Context::Module& CSP_Module = Context->addModule("csp");
        Context->eval(csp::systems::SignalsScriptCode.c_str(), "@preact/signals-core", JS_EVAL_TYPE_MODULE);

        // Bind the existing script functions to the context
        ScriptBinding->BindLocalScriptRoot(Context, &CSP_Module);
    }
    catch (qjs::exception& e)
    { // Catch by reference
        CSP_LOG_ERROR_MSG("QuickJS exception");
    }
    catch (const std::exception& e)
    {
        CSP_LOG_ERROR_FORMAT("std::exception caught: %s", e.what());
    }
    catch (...)
    {
        CSP_LOG_ERROR_MSG("Unknown C++ exception caught.");
    }
}

void LocalScriptSystem::TickAnimationFrame(float timestamp)
{
    if (Runtime == nullptr || Context == nullptr)
    {
        return;
    }
    // This is the javascript "Event Loop"
    // needed to process promises and async/await operations
    while (Runtime->isJobPending())
    {
        Runtime->executePendingJob();
    }

    std::stringstream ss;
    ss << "typeof scriptRegistry !== 'undefined' && scriptRegistry.tick(" << timestamp << ");\n";
    Context->eval(ss.str(), "<import>", JS_EVAL_TYPE_MODULE);
}

void LocalScriptSystem::LoadScriptModules()
{
    // Get asset system to load scripts
    csp::systems::AssetSystem* assetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    if (!assetSystem)
    {
        CSP_LOG_ERROR_MSG("Failed to get AssetSystem");
        return;
    }

    // Create callback for processing loaded scripts
    auto scriptLoadedCallback = [this](const csp::systems::LocalScriptResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            // Get all the loaded scripts
            const auto& scripts = Result.GetLocalScripts();
            // Store the scripts in the class instance to ensure they remain in memory
            this->LoadedScripts = scripts;
            std::stringstream ss;
            ss << "import { createScriptRegistry } from '/scripts/engine/registry.js';\n";
            ss << "const scriptRegistry = createScriptRegistry();\n";
            ss << "globalThis.scriptRegistry = scriptRegistry;\n";
            Context->eval(ss.str(), "<import>", JS_EVAL_TYPE_MODULE);
        }
        else
        {
            CSP_LOG_ERROR_FORMAT("Failed to load scripts: Result code %d", (int)Result.GetResultCode());
        }
    };

    // Load scripts from the asset system
    assetSystem->LoadScripts(this->SpaceId, scriptLoadedCallback);
}

void LocalScriptSystem::RegisterCodeComponentInRegistry(uint64_t EntityId)
{
    std::string entityIdStr = std::to_string(EntityId);
    csp::multiplayer::CodeSpaceComponent* codeComponent = getCodeComponentForEntity(EntityId);
    csp::common::List<csp::common::String> Keys = codeComponent->GetAttributeKeys();
    std::string jsValue = "const attributes = {\n";
    // iterate through the attributes and pass their values to the script registry
    for (size_t i = 0; i < Keys.Size(); ++i)
    {
        
        csp::multiplayer::CodeAttribute* attribute = codeComponent->GetAttribute(Keys[i]);
        if (attribute != nullptr)
        {
            // for each attribute build a string representing a javascript object of the attributes
            // to pass into addCodeComponent
            jsValue += "   '" + Keys[i] + "': ";
            switch (attribute->GetType())
            {   
                case csp::multiplayer::CodePropertyType::STRING:
                    jsValue += "'" + std::string(attribute->GetStringValue().c_str()) + "'";
                    break;
                case csp::multiplayer::CodePropertyType::NUMBER:
                case csp::multiplayer::CodePropertyType::SLIDER:
                    jsValue += std::to_string(attribute->GetFloatValue());
                    break;
                case csp::multiplayer::CodePropertyType::BOOLEAN:

                    jsValue += attribute->GetBoolValue() ? "true" : "false";
                    break;
                case csp::multiplayer::CodePropertyType::VECTOR2:
                    jsValue += "[" + std::to_string(attribute->GetVector2Value().X) + ", " + std::to_string(attribute->GetVector2Value().Y) + "]";
                    break;
                case csp::multiplayer::CodePropertyType::VECTOR3:
                case csp::multiplayer::CodePropertyType::COLOR3:
                    jsValue += "[" + std::to_string(attribute->GetVector3Value().X) + ", " + std::to_string(attribute->GetVector3Value().Y) + ", "
                        + std::to_string(attribute->GetVector3Value().Z) + "]";
                    break;
                case csp::multiplayer::CodePropertyType::VECTOR4:
                    jsValue += "[" + std::to_string(attribute->GetVector4Value().X) + ", " + std::to_string(attribute->GetVector4Value().Y) + ", "
                        + std::to_string(attribute->GetVector4Value().Z) + ", " + std::to_string(attribute->GetVector4Value().W) + "]";
                    break; 
                default:
                    CSP_LOG_ERROR_FORMAT("Unknown attribute type: %d for entity %llu", static_cast<int>(attribute->GetType()), EntityId);
                    continue; // Skip unknown types
            }
        }
        // add comma and newline for next attribute
        jsValue += ",\n";
    }
    jsValue += "};\n";
    std::string out = jsValue + "scriptRegistry.addCodeComponent(parseInt(" + entityIdStr + ", 10), attributes);";
    CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "Registering code component for entity %s w", out.c_str());
    evalScript(csp::common::String(out.c_str()));
}

void LocalScriptSystem::ParseAttributesForEntity(uint64_t EntityId)
{
    std::string entityIdStr = std::to_string(EntityId);
    csp::multiplayer::CodeSpaceComponent* codeComponent = getCodeComponentForEntity(EntityId);
    if (!codeComponent)
    {
        // Error logging already done in getCodeComponentForEntity
        return;
    }

    csp::common::String out = /*javascript*/ R"(
            const scriptAssetPath = ')"
        + codeComponent->GetScriptAssetPath() + R"(';
            const EntityId = ')"
        + entityIdStr.c_str() + R"(';
            const registerModule = async () => {
                console.log(`Register module: ${scriptAssetPath} for entity: ${EntityId}`);
                try {
                    const module = await import(scriptAssetPath);
                    console.log(`Loaded module: ${scriptAssetPath}`);

                    if (!module.attributes) {
                        console.warn(`No attributes found in module: ${scriptAssetPath}`);
                        return;
                    }

                    const {attributes} = module;
                    const typesToNumber = {
                        'number': 0,    // NUMBER = 0
                        'string': 1,    // STRING = 1
                        'vector2': 2,   // VECTOR2 = 2
                        'vector3': 3,   // VECTOR3 = 3
                        'vector4': 4,   // VECTOR4 = 4
                        'color3': 5,    // COLOR3 = 5
                        'boolean': 6,   // BOOLEAN = 6
                        'slider': 7,    // SLIDER = 7
                    };
                    const normalTypes = {
                        'string': 'setAttributeString',
                        'boolean': 'setAttributeBoolean',
                        'vector2': 'setAttributeVector2',
                        'vector3': 'setAttributeVector3',
                        'vector4': 'setAttributeVector4',
                        'color3': 'setAttributeVector3',
                    };
                    const numbericTypes = {
                        'number': 'setAttributeFloat',
                        'slider': 'setAttributeFloat',
                    };

                    // Parse EntityId as a number
                    const entityIdNum = parseInt(EntityId, 10);

                    console.log(`Registering ${Object.keys(attributes).length} attributes for entity ${entityIdNum}`);
                    TheEntitySystem.clearAttributes(entityIdNum);
                    for (const [key, value] of Object.entries(attributes)) {
                        let fn = normalTypes[value.type];
                        const typeNum = typesToNumber[value.type];

                        if (fn) {
                            TheEntitySystem[fn](entityIdNum, key, typeNum, value.defaultValue);
                        } else {
                            fn = numbericTypes[value.type];
                            if (fn) {
                                const min = value.min !== undefined ? value.min : 0;
                                const max = value.max !== undefined ? value.max : 0;
                                TheEntitySystem[fn](entityIdNum, key, typeNum, min, max, value.defaultValue);
                            } else {
                                console.warn(`No handler found for attribute type: ${value.type}`);
                            }
                        }
                    }
                } catch (error) {
                    console.error(`Error loading module: ${error.message || error}`);
                }
            };
            registerModule();
    )";
    evalScript(out);
}

void LocalScriptSystem::evalScript(const csp::common::String& script)
{
    JSValue v = Context->eval(script.c_str(), "<import>", JS_EVAL_TYPE_MODULE);
    if (JS_IsException(v))
    {
        CSP_LOG_ERROR_FORMAT("QuickJS exception: %s", script.c_str());
    }
}

void LocalScriptSystem::UpdateAttributeForEntity(uint64_t EntityId, const csp::common::String& Key, const csp::multiplayer::CodeAttribute& Attribute)
{
    // call the script registry in quickjs to update the attribute for the entity
    std::string entityIdStr = std::to_string(EntityId);
    std::string jsValue;

    // Determine the correct value based on attribute type
    switch (Attribute.GetType())
    {
    case csp::multiplayer::CodePropertyType::STRING:
        jsValue = "'" + std::string(Attribute.GetStringValue().c_str()) + "'";
        break;
    case csp::multiplayer::CodePropertyType::BOOLEAN:
        jsValue = Attribute.GetBoolValue() ? "true" : "false";
        break;
    case csp::multiplayer::CodePropertyType::NUMBER:
    case csp::multiplayer::CodePropertyType::SLIDER:
        jsValue = std::to_string(Attribute.GetFloatValue());
        break;
    case csp::multiplayer::CodePropertyType::VECTOR2:
        // Vector2 as a JavaScript array
        jsValue = "[" + std::to_string(Attribute.GetVector2Value().X) + ", " + std::to_string(Attribute.GetVector2Value().Y) + "]";
        break;
    case csp::multiplayer::CodePropertyType::VECTOR3:
    case csp::multiplayer::CodePropertyType::COLOR3:
        jsValue = "[" + std::to_string(Attribute.GetVector3Value().X) + ", " + std::to_string(Attribute.GetVector3Value().Y) + ", "
            + std::to_string(Attribute.GetVector3Value().Z) + "]";
        break;
    case csp::multiplayer::CodePropertyType::VECTOR4:
        jsValue = "[" + std::to_string(Attribute.GetVector4Value().X) + ", " + std::to_string(Attribute.GetVector4Value().Y) + ", "
            + std::to_string(Attribute.GetVector4Value().Z) + ", " + std::to_string(Attribute.GetVector4Value().W) + "]";
        break;
    default:
        CSP_LOG_ERROR_FORMAT("Unknown attribute type: %d", static_cast<int>(Attribute.GetType()));
        return;
    }

    // Create a formatted JavaScript string with proper variable interpolation
    csp::common::String out = csp::common::StringFormat(/*javascript*/ R"(
        scriptRegistry.updateAttributeForEntity(parseInt('%s', 10), '%s', %s);
    )",
        entityIdStr.c_str(), Key.c_str(), jsValue.c_str());

    evalScript(out);
}

csp::multiplayer::CodeSpaceComponent* LocalScriptSystem::getCodeComponentForEntity(uint64_t EntityId)
{
    csp::multiplayer::SpaceEntity* entity = EntitySystem->FindSpaceEntityById(EntityId);
    if (!entity)
    {
        CSP_LOG_ERROR_FORMAT("Entity with ID %llu not found.", EntityId);
        return nullptr;
    }

    csp::multiplayer::ComponentBase* baseComponent = entity->FindFirstComponentOfType(csp::multiplayer::ComponentType::Code);
    if (!baseComponent)
    {
        CSP_LOG_ERROR_FORMAT("Entity with ID %llu does not have a Code component.", EntityId);
        return nullptr;
    }

    csp::multiplayer::CodeSpaceComponent* codeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(baseComponent);
    if (!codeComponent)
    {
        CSP_LOG_ERROR_FORMAT("Entity with ID %llu has a Code component, but it's not a CodeSpaceComponent.", EntityId);
        return nullptr;
    }

    return codeComponent;
}

}
