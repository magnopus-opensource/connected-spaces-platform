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

#include "CSP/Systems/Script/ScriptSystem.h"

#include "CSP/CSPFoundation.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Memory/MemoryManager.h"
#include "Multiplayer/Script/ScriptHelpers.h"
#include "Systems/Script/ScriptContext.h"
#include "Systems/Script/ScriptRuntime.h"
#include "quickjspp.hpp"

#include <rapidjson/document.h>

// Enable optional 'os' and 'std' modules for file access etc
// #define SCRIPTS_INCLUDE_STD_LIBS

#if defined(SCRIPTS_INCLUDE_STD_LIBS)
	#include "quickjs-libc.h"
#endif

#include <map>
#include <sstream>


using namespace csp::common;


// For safer printfs
constexpr int MAX_SCRIPT_FUNCTION_LEN = 256;

constexpr auto ASSET_COLLECTION_NAME_PREFIX = "OKO_SCRIPTMODULENAMESPACE_";


// Template specializations for some custom csp types we want to use
template <> struct qjs::js_property_traits<String>
{
	static void set_property(JSContext* ctx, JSValue this_obj, String str, JSValue value)
	{
		int err = JS_SetPropertyStr(ctx, this_obj, str.c_str(), value);

		if (err < 0)
			throw exception {ctx};
	}

	static JSValue get_property(JSContext* ctx, JSValue this_obj, String str) noexcept
	{
		return JS_GetPropertyStr(ctx, this_obj, str.c_str());
	}
};

template <> struct qjs::js_traits<String>
{
	static String unwrap(JSContext* ctx, JSValueConst v)
	{
		size_t plen;
		const char* ptr = JS_ToCStringLen(ctx, &plen, v);

		if (!ptr)
			throw exception {ctx};

		return String(ptr, plen);
	}

	static JSValue wrap(JSContext* ctx, String str) noexcept
	{
		return JS_NewStringLen(ctx, str.c_str(), str.Length());
	}
};


namespace
{

typedef Map<String, String> LookupTableMap;


LookupTableMap JsonToLookupTable(const String& JsonString)
{
	rapidjson::Document Json;
	Json.Parse(JsonString.c_str(), JsonString.Length());

	LookupTableMap LookupTable;

	for (const auto& Entry : Json.GetObject())
	{
		LookupTable[Entry.name.GetString(), Entry.value.GetString()];
	}

	return LookupTable;
}

String LookupTableToJson(const LookupTableMap& LookupTable)
{
	rapidjson::Document Json(rapidjson::kObjectType);
	auto Object = Json.GetObject();

	auto* Keys = LookupTable.Keys();

	for (int i = 0; i < Keys->Size(); ++i)
	{
		auto& Key = Keys->operator[](i);

		rapidjson::Value Name(Key.c_str(), Json.GetAllocator());
		rapidjson::Value Value(LookupTable[Key].c_str(), Json.GetAllocator());

		Object.AddMember(Name, Value, Json.GetAllocator());
	}

	CSP_DELETE(Keys);

	return Json.GetString();
}

} // namespace


namespace csp::systems
{

ScriptSystem::ScriptSystem() : TheScriptRuntime(nullptr)
{
}

ScriptSystem::~ScriptSystem()
{
	Shutdown();
}

void ScriptSystem::Initialise()
{
	if (TheScriptRuntime != nullptr)
	{
		FOUNDATION_LOG_ERROR_MSG("ScriptSystem::Initialise already called\n");
		return;
	}

	TheScriptRuntime = CSP_NEW ScriptRuntime(this);

#if defined(SCRIPTS_INCLUDE_STD_LIBS)
	js_std_init_handlers(TheScriptRuntime->Runtime->rt);
	JS_SetModuleLoaderFunc(TheScriptRuntime->Runtime->rt, nullptr, js_module_loader, nullptr);
	js_std_add_helpers(TheScriptRuntime->Context->ctx, 0, nullptr);

	js_init_module_std(TheScriptRuntime->Context->ctx, "std");
	js_init_module_os(TheScriptRuntime->Context->ctx, "os");
#else
// @todo WASM build may need a custom module loader
//	JS_SetModuleLoaderFunc(TheScriptRuntime->Runtime->rt, nullptr, js_module_loader, nullptr);
#endif

	// Define a module name alias to be used when importing a module by name in a script
	AddModuleUrlAlias(OLD_SCRIPT_NAMESPACE, SCRIPT_NAMESPACE);
}

void ScriptSystem::Shutdown()
{
	if (TheScriptRuntime != nullptr)
	{
		CSP_DELETE(TheScriptRuntime);
		TheScriptRuntime = nullptr;
	}
}

bool ScriptSystem::RunScript(int64_t ContextId, const String& ScriptText)
{
	// FOUNDATION_LOG_FORMAT(LogLevel::Verbose, "RunScript: %s\n", ScriptText.c_str());

	ScriptContext* TheScriptContext = TheScriptRuntime->GetContext(ContextId);
	if (TheScriptContext == nullptr)
	{
		return false;
	}

	qjs::Value Result = TheScriptContext->Context->eval(ScriptText.c_str(), "<eval>", JS_EVAL_TYPE_MODULE);
	bool HasErrors	  = Result.isException();
	return !HasErrors;
}

bool ScriptSystem::RunScriptFile(int64_t ContextId, const String& ScriptFilePath)
{
	FOUNDATION_LOG_FORMAT(LogLevel::Verbose, "RunScriptFile: %s\n", ScriptFilePath.c_str());

	ScriptContext* TheScriptContext = TheScriptRuntime->GetContext(ContextId);
	if (TheScriptContext == nullptr)
	{
		return false;
	}

	qjs::Value Result = TheScriptContext->Context->evalFile(ScriptFilePath.c_str(), JS_EVAL_TYPE_MODULE);
	bool HasErrors	  = Result.isException();
	return !HasErrors;
}

bool ScriptSystem::CreateContext(int64_t ContextId)
{
	return TheScriptRuntime->AddContext(ContextId);
}

bool ScriptSystem::DestroyContext(int64_t ContextId)
{
	return TheScriptRuntime->RemoveContext(ContextId);
}

bool ScriptSystem::BindContext(int64_t ContextId)
{
	return TheScriptRuntime->BindContext(ContextId);
}


bool ScriptSystem::ResetContext(int64_t ContextId)
{
	return TheScriptRuntime->ResetContext(ContextId);
}


bool ScriptSystem::ExistsInContext(int64_t ContextId, const String& ObjectName)
{
	return TheScriptRuntime->ExistsInContext(ContextId, ObjectName);
}


void* ScriptSystem::GetContext(int64_t ContextId)
{
	return (void*) TheScriptRuntime->GetContext(ContextId)->Context;
}

void* ScriptSystem::GetModule(int64_t ContextId, const String& ModuleName)
{
	ScriptContext* TheScriptContext = TheScriptRuntime->GetContext(ContextId);
	return (void*) TheScriptContext->GetModule(ModuleName)->Module;
}

void ScriptSystem::RegisterScriptBinding(IScriptBinding* ScriptBinding)
{
	TheScriptRuntime->RegisterScriptBinding(ScriptBinding);
}

void ScriptSystem::UnregisterScriptBinding(IScriptBinding* ScriptBinding)
{
	TheScriptRuntime->UnregisterScriptBinding(ScriptBinding);
}

void ScriptSystem::SetModuleSource(String ModuleUrl, String Source)
{
	TheScriptRuntime->SetModuleSource(ModuleUrl, Source);
}

void ScriptSystem::AddModuleUrlAlias(const String& ModuleUrl, const String& ModuleUrlAlias)
{
	TheScriptRuntime->AddModuleUrlAlias(ModuleUrl, ModuleUrlAlias);
}

void ScriptSystem::ClearModuleSource(String ModuleUrl)
{
	TheScriptRuntime->ClearModuleSource(ModuleUrl);
}

String ScriptSystem::GetModuleSource(String ModuleUrl)
{
	return TheScriptRuntime->GetModuleSource(ModuleUrl);
}

bool ScriptSystem::GetModuleUrlAlias(const String& ModuleUrl, String& OutModuleUrlAlias)
{
	return TheScriptRuntime->GetModuleUrlAlias(ModuleUrl, OutModuleUrlAlias);
}

size_t ScriptSystem::GetNumImportedModules(int64_t ContextId) const
{
	return TheScriptRuntime->GetContext(ContextId)->GetNumImportedModules();
}

const char* ScriptSystem::GetImportedModule(int64_t ContextId, size_t Index) const
{
	return TheScriptRuntime->GetContext(ContextId)->GetImportedModule(Index);
}


ScriptModuleCollection::ScriptModuleCollection() {};
ScriptModuleCollection::~ScriptModuleCollection() {};

const String& ScriptModuleCollection::GetId() const
{
	return Id;
}

const Map<String, String>& ScriptModuleCollection::GetLookupTable() const
{
	return LookupTable;
}

ScriptModuleCollection& ScriptModuleCollectionResult::GetCollection()
{
	return Collection;
}

const ScriptModuleCollection& ScriptModuleCollectionResult::GetCollection() const
{
	return Collection;
}


ScriptModuleAsset::ScriptModuleAsset() {};

ScriptModuleAsset ::~ScriptModuleAsset() {};

const String& ScriptModuleAsset::GetId() const
{
	return Id;
}

const String& ScriptModuleAsset::GetModuleText() const
{
	return ModuleText;
}


ScriptModuleAsset& ScriptModuleAssetResult::GetModule()
{
	return Module;
}

const ScriptModuleAsset& ScriptModuleAssetResult::GetModule() const
{
	return Module;
}


void ScriptSystem::CreateScriptModuleCollection(const String& Namespace, const ScriptModuleCollectionResultCallback& Callback)
{
	auto AssetCollectionName = String(ASSET_COLLECTION_NAME_PREFIX);
	AssetCollectionName.Append(Namespace);

	String AssetName = AssetCollectionName;
	AssetName.Append("_LOOKUPTABLE");

	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

	AssetCollectionResultCallback CreateAssetCollectionCallback = [AssetSystem, AssetName, Callback](const AssetCollectionResult& Result)
	{
		const auto& AssetCollection = Result.GetAssetCollection();

		AssetResultCallback CreateAssetCallback = [AssetSystem, Callback, AssetCollection](const AssetResult& Result)
		{
			const auto& Asset = Result.GetAsset();

			AssetCollectionResultCallback UpdateAssetCollectionCallback = [AssetSystem, Asset, Callback](const AssetCollectionResult& Result)
			{
				const auto& AssetCollection = Result.GetAssetCollection();

				BufferAssetDataSource AssetData;
				AssetData.SetMimeType("application/json");
				AssetData.Buffer	   = "{}";
				AssetData.BufferLength = 2;

				UriResultCallback UploadAssetDataCallback = [AssetCollection, Callback](const UriResult& Result)
				{
					ScriptModuleCollectionResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

					auto& Collection		 = InternalResult.GetCollection();
					Collection.Id			 = AssetCollection.Id;
					Collection.LookupTableId = AssetCollection.GetMetadataImmutable()["lookup_table_id"];

					Callback(InternalResult);
				};

				AssetSystem->UploadAssetData(AssetCollection, Asset, AssetData, UploadAssetDataCallback);
			};

			AssetSystem->UpdateAssetCollectionMetadata(AssetCollection, {{"lookup_table_id", Asset.Id}}, UpdateAssetCollectionCallback);
		};

		// Create module lookup table
		AssetSystem->CreateAsset(AssetCollection, AssetName, nullptr, nullptr, EAssetType::SCRIPT_MODULE, CreateAssetCallback);
	};

	// Create asset collection that represents script module collection
	AssetSystem->CreateAssetCollection(nullptr,
									   nullptr,
									   AssetCollectionName,
									   nullptr,
									   EAssetCollectionType::SCRIPT_MODULE_COLLECTION,
									   nullptr,
									   CreateAssetCollectionCallback);
}

void ScriptSystem::GetLookupTableById(const String& CollectionId, const String& Id, std::function<void(const LookupTableMap&)> Callback)
{
	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

	AssetResultCallback GetAssetCallback = [AssetSystem, Callback](const AssetResult& Result)
	{
		const auto& Asset = Result.GetAsset();

		AssetDataResultCallback DownloadAssetDataCallback = [Callback](const AssetDataResult& Result)
		{
			auto Data		= Result.GetData();
			auto DataLength = Result.GetDataLength();

			auto LookupTable = JsonToLookupTable(String(reinterpret_cast<const char*>(Data), DataLength));

			Callback(LookupTable);
		};

		// The table itself is stored as a JSON document
		AssetSystem->DownloadAssetData(Asset, DownloadAssetDataCallback);
	};

	// A ScriptModuleCollection's lookup table is stored as an Asset
	AssetSystem->GetAssetById(CollectionId, Id, GetAssetCallback);
}

void ScriptSystem::GetScriptModuleCollection(const csp::common::String& Namespace, const ScriptModuleCollectionResultCallback& Callback)
{
	auto AssetCollectionName = String(ASSET_COLLECTION_NAME_PREFIX);
	AssetCollectionName.Append(Namespace);

	String AssetName = AssetCollectionName;
	AssetName.Append("_LOOKUPTABLE");

	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

	AssetCollectionResultCallback GetAssetCollectionCallback = [AssetSystem, AssetName, Callback](const AssetCollectionResult& Result)
	{
		const auto& AssetCollection = Result.GetAssetCollection();
		String AssetCollectionId	= AssetCollection.Id;
		const auto& Metadata		= AssetCollection.GetMetadataImmutable();
		const auto& LookupTableId	= Metadata["lookup_table_id"];

		auto GetLookupTableCallback = [AssetCollectionId, LookupTableId, Callback](const LookupTableMap& Result)
		{
			// TODO: Handle failures in GetLookupTableById
			ScriptModuleCollectionResult InternalResult(csp::services::EResultCode::Success, 400);

			auto& Collection		 = InternalResult.GetCollection();
			Collection.Id			 = AssetCollectionId;
			Collection.LookupTable	 = Result;
			Collection.LookupTableId = LookupTableId;

			Callback(InternalResult);
		};

		GetLookupTableById(AssetCollection.Id, LookupTableId, GetLookupTableCallback);
	};

	// ScriptModuleCollection is stored as an AssetCollection on CHS
	AssetSystem->GetAssetCollectionByName(AssetCollectionName, GetAssetCollectionCallback);
}

void ScriptSystem::DeleteScriptModuleCollection(const ScriptModuleCollection& Collection, const NullResultCallback& Callback)
{
	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

	AssetSystem->DeleteAssetCollection(Collection.GetId(), Callback);
}

void ScriptSystem::CreateScriptModuleAsset(const ScriptModuleCollection& Collection,
										   const String& Name,
										   const String& ModuleText,
										   const NullResultCallback& Callback)
{
	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

	// check collection doesn't have module already. if it does create an invalid nullresult::invalid and set the response body to something
	// meaningful
	// We need to fetch the lookuptable asset - create a function to do this for us and converts json into map.
	const auto& LookupTable = Collection.GetLookupTable();
	if (LookupTable.HasKey(Name))
	{
		NullResult InvalidNullResult = NullResult::Invalid();
		Callback(InvalidNullResult);
	}

	// create the script module asset that will hold the script
	// create the asset data which stores moduleText and upload it
	// get the lookuptable from the collection
	// add entry to the lookup table, turn back into json and then uuse the lookuptableid to update the assetdata call
	// callback
	// Create new functions for converting to/from json/map

	csp::systems::AssetCollection InternalAssetCollection;
	InternalAssetCollection.Id	 = Collection.GetId();
	InternalAssetCollection.Type = EAssetCollectionType::SCRIPT_MODULE_COLLECTION;

	AssetResultCallback CreateAssetCallback
		= [AssetSystem, InternalAssetCollection, Collection, Name, ModuleText, Callback](const AssetResult& Result)
	{
		const auto& Asset = Result.GetAsset();

		BufferAssetDataSource AssetData;
		AssetData.SetMimeType("text/javascript");
		AssetData.Buffer	   = const_cast<char*>(ModuleText.c_str());
		AssetData.BufferLength = ModuleText.Length();

		UriResultCallback UploadAssetDataCallback = [Collection, Name, Asset, InternalAssetCollection, AssetSystem, Callback](const UriResult& Result)
		{
			ScriptModuleCollectionResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

			auto& InternalCollection = InternalResult.GetCollection();

			AssetResultCallback GetAssetCallback = [InternalCollection, AssetSystem, InternalAssetCollection, Callback](const AssetResult& Result)
			{
				const auto& Asset = Result.GetAsset();

				const auto& LookupTable = InternalCollection.GetLookupTable();
				auto Json				= LookupTableToJson(LookupTable);

				BufferAssetDataSource AssetData;
				AssetData.SetMimeType("application/json");
				AssetData.Buffer	   = const_cast<char*>(Json.c_str());
				AssetData.BufferLength = Json.Length();

				UriResultCallback UploadAssetDataCallback = [Callback](const UriResult& Result)
				{
					NullResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

					Callback(InternalResult);
				};

				AssetSystem->UploadAssetData(InternalAssetCollection, Asset, AssetData, UploadAssetDataCallback);
			};

			// Get the Lookuptable Asset.
			AssetSystem->GetAssetById(InternalCollection.GetId(), InternalCollection.LookupTableId, GetAssetCallback);
		};

		AssetSystem->UploadAssetData(InternalAssetCollection, Asset, AssetData, UploadAssetDataCallback);
	};

	// Change this to instead take the AssetCollectionId - update method
	AssetSystem->CreateAsset(InternalAssetCollection, Name, nullptr, nullptr, EAssetType::SCRIPT_MODULE, CreateAssetCallback);
}

} // namespace csp::systems
