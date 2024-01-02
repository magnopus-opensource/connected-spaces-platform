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
#include "CSP/Common/StringFormat.h"
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

#include "Common/Logger.h"

#include <map>
#include <sstream>


using namespace csp::common;


// For safer printfs
constexpr int MAX_SCRIPT_FUNCTION_LEN = 256;

constexpr auto ASSET_COLLECTION_NAME_PREFIX = "OKO_SCRIPTMODULENAMESPACE_";
constexpr auto LOOKUP_TABLE_METADATA_KEY	= "module_lookup_table";

constexpr auto SCRIPT_MODULE_PREFIX			 = "SCRIPT_MODULE_";
constexpr size_t SCRIPT_MODULE_PREFIX_LENGTH = std::char_traits<char>::length(SCRIPT_MODULE_PREFIX);


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
		CSP_LOG_ERROR_MSG("ScriptSystem::Initialise already called\n");
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
	// CSP_LOG_FORMAT(LogLevel::Verbose, "RunScript: %s\n", ScriptText.c_str());

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
	CSP_LOG_FORMAT(LogLevel::Verbose, "RunScriptFile: %s\n", ScriptFilePath.c_str());

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

Map<String, String>& ScriptModuleCollection::GetMutableLookupTable() const
{
	return const_cast<decltype(LookupTable)&>(LookupTable);
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

ScriptModuleAsset::~ScriptModuleAsset() {};

const String& ScriptModuleAsset::GetId() const
{
	return Id;
}

const String& ScriptModuleAsset::GetName() const
{
	return ModuleName;
}

const String& ScriptModuleAsset::GetNamespace() const
{
	return ModuleNamespace;
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

void ScriptModuleAssetResult::SetResponseBody(const csp::common::String& Value)
{
	ResponseBody = Value;
}

ScriptModuleAssetNames::ScriptModuleAssetNames() {};

ScriptModuleAssetNames::~ScriptModuleAssetNames() {};

const csp::common::Array<csp::common::String>& ScriptModuleAssetNames::GetScriptModuleAssetNames() const
{
	return ModuleAssetNames;
}

ScriptModuleAssetNames& ScriptModuleAssetNamesResult::GetModuleAssetNames()
{
	return ModuleAssets;
}

const ScriptModuleAssetNames& ScriptModuleAssetNamesResult::GetModuleAssetNames() const
{
	return ModuleAssets;
}

void ScriptSystem::GetScriptModuleAsset(const csp::common::String& ModuleNamespace,
										const csp::common::String& ModuleName,
										const ScriptModuleAssetResultCallback& Callback)
{
	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

	const csp::common::String ScriptModuleName
		= csp::common::StringFormat("%s%s_%s", SCRIPT_MODULE_PREFIX, ModuleNamespace.c_str(), ModuleName.c_str());
	const csp::common::String ScriptModuleNamespace = csp::common::StringFormat("%s%s", SCRIPT_MODULE_PREFIX, ModuleNamespace.c_str());

	AssetsResultCallback GetAssetsCallback = [this, Callback, AssetSystem, ModuleName, ModuleNamespace](const AssetsResult& Result)
	{
		if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			return;
		}

		if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
		{
			ScriptModuleAssetResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
			Callback(InternalResult);

			return;
		}

		const Asset& InternalAsset = Result.GetAssets()[0];

		// todo: look at replacing these with a macro to make the intent clearer.
		AssetDataResultCallback GetAssetDataCallback
			= std::bind(&ScriptSystem::OnDownloadAssetData, this, ModuleNamespace, ModuleName, InternalAsset.Id, Callback, std::placeholders::_1);

		AssetSystem->DownloadAssetData(InternalAsset, GetAssetDataCallback);
	};

	AssetSystem->GetAssetsByCriteria(nullptr, {ScriptModuleNamespace}, nullptr, {ScriptModuleName}, nullptr, GetAssetsCallback);
}

// todo: make these past tense rather, ie AssetDataDownloaded
void ScriptSystem::OnDownloadAssetData(const csp::common::String& ModuleNamespace,
									   const csp::common::String& ModuleName,
									   const csp::common::String& ScriptModuleAssetId,
									   const ScriptModuleAssetResultCallback& Callback,
									   const AssetDataResult& Result)
{
	if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
	{
		return;
	}

	ScriptModuleAssetResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

	if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
	{
		Callback(InternalResult);

		return;
	}

	ScriptModuleAsset& InternalScriptModuleAsset = InternalResult.GetModule();

	size_t DownloadedAssetDataSize = Result.GetDataLength();
	auto DownloadedAssetData	   = CSP_NEW char[DownloadedAssetDataSize];
	std::memcpy(DownloadedAssetData, Result.GetData(), DownloadedAssetDataSize);

	InternalScriptModuleAsset.Id			  = ScriptModuleAssetId;
	InternalScriptModuleAsset.ModuleName	  = ModuleName;
	InternalScriptModuleAsset.ModuleNamespace = ModuleNamespace;
	InternalScriptModuleAsset.ModuleText	  = DownloadedAssetData;

	Callback(InternalResult);
}

void ScriptSystem::GetScriptModuleAssetNames(const csp::common::String& ModuleNamespace, const ScriptModuleAssetNamesResultCallback& Callback)
{
	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

	const csp::common::String ScriptModuleNamespace = csp::common::StringFormat("%s%s", SCRIPT_MODULE_PREFIX, ModuleNamespace.c_str());

	AssetsResultCallback GetAssetsResultCallback = [Callback, ModuleNamespace](const AssetsResult& Result)
	{
		if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			return;
		}

		ScriptModuleAssetNamesResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

		if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
		{

			Callback(InternalResult);

			return;
		}

		const csp::common::Array<Asset>& InternalModuleAssets = Result.GetAssets();

		ScriptModuleAssetNames& InternalScriptModuleAssetNames = InternalResult.GetModuleAssetNames();
		csp::common::Array<csp::common::String> ModuleAssetNames(InternalModuleAssets.Size());

		for (size_t i = 0; i < InternalModuleAssets.Size(); ++i)
		{
			csp::common::String AssetName  = InternalModuleAssets[i].Name;
			size_t Index				   = SCRIPT_MODULE_PREFIX_LENGTH + ModuleNamespace.Length() + sizeof('_');
			csp::common::String ModuleName = AssetName.SubString(Index);
			ModuleAssetNames[i]			   = ModuleName;
		}

		InternalScriptModuleAssetNames.ModuleAssetNames = ModuleAssetNames;

		Callback(InternalResult);
	};

	AssetSystem->GetAssetsByCriteria(nullptr, {ScriptModuleNamespace}, nullptr, nullptr, nullptr, GetAssetsResultCallback);
}

void ScriptSystem::CreateScriptModuleAsset(const csp::common::String& ModuleNamespace,
										   const csp::common::String& ModuleName,
										   const csp::common::Optional<csp::common::String>& ScriptSource,
										   UriResultCallback Callback)
{
	// 1. Try and get the modulenamespace AssetCollection. If the module exists goto 3, if not goto 2.
	// 2. AssetCollection/prototype does not exist - Create an ModuleNamespace AssetCollection and goto 3.
	// 3. AssetCollection/prototype exists - Create a module.
	// 4. If ScriptSource specified then upload Asset Data.

	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

	const csp::common::String ScriptModuleNamespace = csp::common::StringFormat("%s%s", SCRIPT_MODULE_PREFIX, ModuleNamespace.c_str());

	const AssetCollectionResultCallback GetAssetCollectionCallback(std::bind(&ScriptSystem::OnScriptModuleAssetCollectionRetrieved,
																			 this,
																			 ModuleName,
																			 ScriptModuleNamespace,
																			 ScriptSource,
																			 Callback,
																			 std::placeholders::_1));

	// 1. Try and get the modulenamespace AssetCollection. If the module exists goto 3, if not goto 2.
	AssetSystem->GetAssetCollectionByName(ScriptModuleNamespace, GetAssetCollectionCallback);
}

void ScriptSystem::OnScriptModuleAssetCollectionRetrieved(const csp::common::String& ModuleName,
														  const csp::common::String& ScriptModuleNamespace,
														  const csp::common::Optional<csp::common::String>& ScriptSource,
														  UriResultCallback Callback,
														  const AssetCollectionResult& Result)
{
	if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
	{
		return;
	}

	if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
	{
		// 2. AssetCollection/prototype does not exist - Create an ModuleNamespace AssetCollection and goto 3.
		const ScriptModuleCollectionResultCallback CreateScriptModuleCollectionCallback(std::bind(&ScriptSystem::OnScriptModuleCollectionCreated,
																								  this,
																								  ModuleName,
																								  ScriptModuleNamespace,
																								  ScriptSource,
																								  Callback,
																								  std::placeholders::_1));

		CreateScriptModuleNamespace(ScriptModuleNamespace, CreateScriptModuleCollectionCallback);
		return;
	}

	// 3. AssetCollection/prototype exists - Create a module.
	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

	const csp::common::String ScriptModuleName
		= csp::common::StringFormat("%s%s_%s", SCRIPT_MODULE_PREFIX, ScriptModuleNamespace.c_str(), ModuleName.c_str());

	const AssetResultCallback CreateScriptModuleAssetCallback(
		std::bind(&ScriptSystem::OnScriptModuleAssetCreated, this, Result.GetAssetCollection(), ScriptSource, Callback, std::placeholders::_1));

	AssetSystem
		->CreateAsset(Result.GetAssetCollection(), ScriptModuleName, nullptr, nullptr, EAssetType::SCRIPT_MODULE, CreateScriptModuleAssetCallback);
}

void ScriptSystem::OnScriptModuleCollectionCreated(const csp::common::String& ScriptModuleName,
												   const csp::common::String& ScriptModuleNamespace,
												   const csp::common::Optional<csp::common::String>& ScriptSource,
												   UriResultCallback Callback,
												   ScriptModuleCollectionResult& Result)
{
	if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
	{
		return;
	}

	if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
	{
		const UriResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
		Callback(InternalResult);
		return;
	}

	// 3. AssetCollection/prototype exists - Create a module.
	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

	const csp::common::String FullScriptModuleName
		= csp::common::StringFormat("%s%s_%s", SCRIPT_MODULE_PREFIX, ScriptModuleNamespace.c_str(), ScriptModuleName.c_str());

	csp::systems::AssetCollection InternalAssetCollection;
	InternalAssetCollection.Id = Result.GetCollection().GetId();

	const AssetResultCallback CreateScriptModuleAssetCallback(
		std::bind(&ScriptSystem::OnScriptModuleAssetCreated, this, InternalAssetCollection, ScriptSource, Callback, std::placeholders::_1));

	AssetSystem
		->CreateAsset(InternalAssetCollection, FullScriptModuleName, nullptr, nullptr, EAssetType::SCRIPT_MODULE, CreateScriptModuleAssetCallback);
}

void ScriptSystem::OnScriptModuleAssetCreated(const AssetCollection& ScriptModuleAssetCollection,
											  const csp::common::Optional<csp::common::String>& ScriptSource,
											  UriResultCallback Callback,
											  const AssetResult& Result)
{
	if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
	{
		return;
	}

	if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
	{
		const UriResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
		Callback(InternalResult);
		return;
	}

	// No Script Module source provided.
	if (!ScriptSource.HasValue())
	{
		UriResult InternalResult(csp::systems::EResultCode::Success, Result.GetHttpResultCode());
		InternalResult.SetResponseBody("Script Module Asset created, no script source specified.");
		InternalResult.Uri = "";

		Callback(InternalResult);
		return;
	}

	// 4. If ScriptSource specified then upload Asset Data.
	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

	const auto& Asset = Result.GetAsset();

	BufferAssetDataSource AssetData;
	AssetData.SetMimeType("text/javascript");
	AssetData.Buffer	   = const_cast<char*>(ScriptSource->c_str());
	AssetData.BufferLength = ScriptSource->Length();

	const UriResultCallback UploadScriptModuleSourceCallback(
		std::bind(&ScriptSystem::OnScriptModuleSourceUploaded, this, Callback, std::placeholders::_1));

	AssetSystem->UploadAssetData(ScriptModuleAssetCollection, Asset, AssetData, UploadScriptModuleSourceCallback);
}

void ScriptSystem::OnScriptModuleSourceUploaded(UriResultCallback Callback, const UriResult& Result)
{
	if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
	{
		return;
	}

	if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
	{
		UriResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
		InternalResult.SetResponseBody("Script module source failed to upload.");
		InternalResult.Uri = "";

		Callback(InternalResult);
		return;
	}

	UriResult InternalResult(csp::systems::EResultCode::Success, Result.GetHttpResultCode());
	InternalResult.SetResponseBody(Result.GetResponseBody());
	InternalResult.Uri = Result.GetUri();

	Callback(InternalResult);
}

void ScriptSystem::UpdateScriptModuleAsset(const csp::common::String& ModuleNamespace,
										   const csp::common::String& ModuleName,
										   const csp::common::String& ScriptSource,
										   UriResultCallback Callback)
{
	// Move all of this to use futures. That way we could use generic functions and then chain them.
	// https://www.educba.com/c-plus-plus-async-await/

	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

	const csp::common::String ScriptModuleName
		= csp::common::StringFormat("%s%s_%s", SCRIPT_MODULE_PREFIX, ModuleNamespace.c_str(), ModuleName.c_str());
	const csp::common::String ScriptModuleNamespace = csp::common::StringFormat("%s%s", SCRIPT_MODULE_PREFIX, ModuleNamespace.c_str());

	AssetsResultCallback GetAssetsCallback = [this, Callback, AssetSystem, ScriptSource, ModuleName, ModuleNamespace](const AssetsResult& Result)
	{
		if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			return;
		}

		if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
		{
			UriResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
			InternalResult.SetResponseBody("No Script Module found with that name in the specified namespace.");
			InternalResult.Uri = "";
			return;
		}

		const Asset& InternalAsset = Result.GetAssets()[0];

		csp::systems::AssetCollection InternalAssetCollection;
		InternalAssetCollection.Id = InternalAsset.AssetCollectionId;

		BufferAssetDataSource AssetData;
		AssetData.SetMimeType("text/javascript");
		AssetData.Buffer	   = const_cast<char*>(ScriptSource.c_str());
		AssetData.BufferLength = ScriptSource.Length();

		const UriResultCallback UploadScriptModuleSourceCallback(
			std::bind(&ScriptSystem::OnScriptModuleSourceUploaded, this, Callback, std::placeholders::_1));

		AssetSystem->UploadAssetData(InternalAssetCollection, InternalAsset, AssetData, UploadScriptModuleSourceCallback);
	};

	AssetSystem->GetAssetsByCriteria(nullptr, {ScriptModuleNamespace}, nullptr, {ScriptModuleName}, nullptr, GetAssetsCallback);
}

void ScriptSystem::DeleteScriptModuleAsset(const csp::common::String& ModuleNamespace,
										   const csp::common::String& ModuleName,
										   const NullResultCallback& Callback)
{
	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

	const csp::common::String ScriptModuleNamespace = csp::common::StringFormat("%s%s", SCRIPT_MODULE_PREFIX, ModuleNamespace.c_str());
	const csp::common::String ScriptModuleName
		= csp::common::StringFormat("%s%s_%s", SCRIPT_MODULE_PREFIX, ModuleNamespace.c_str(), ModuleName.c_str());

	AssetsResultCallback GetAssetResultCallback(std::bind(&ScriptSystem::OnScriptModuleAssetRetrieved, this, Callback, std::placeholders::_1));

	AssetSystem->GetAssetsByCriteria(nullptr, {ScriptModuleNamespace}, nullptr, {ScriptModuleName}, nullptr, GetAssetResultCallback);
}

void ScriptSystem::OnScriptModuleAssetRetrieved(const NullResultCallback& Callback, const AssetsResult& Result)
{
	if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
	{
		return;
	}

	if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
	{
		NullResult InvalidNullResult = NullResult::Invalid();
		Callback(InvalidNullResult);
		return;
	}

	const NullResultCallback DeleteAssetCallback = [Callback](const NullResult& Result)
	{
		if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			return;
		}

		if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
		{
			NullResult InvalidNullResult = NullResult::Invalid();
			Callback(InvalidNullResult);
			return;
		}

		NullResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
		Callback(InternalResult);
	};

	const csp::common::Array<Asset>& InternalModuleAsset = Result.GetAssets();

	csp::systems::AssetCollection InternalAssetCollection;
	InternalAssetCollection.Id = InternalModuleAsset[0].AssetCollectionId;

	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();
	AssetSystem->DeleteAsset(InternalAssetCollection, InternalModuleAsset[0], DeleteAssetCallback);
}

void ScriptSystem::CreateScriptModuleNamespace(const String& ModuleNamespace, const ScriptModuleCollectionResultCallback& Callback)
{
	const csp::common::String ScriptModuleNamespace = csp::common::StringFormat("%s%s", SCRIPT_MODULE_PREFIX, ModuleNamespace.c_str());

	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

	AssetCollectionResultCallback CreateAssetCollectionCallback = [AssetSystem, Callback](const AssetCollectionResult& Result)
	{
		const auto& AssetCollection = Result.GetAssetCollection();

		ScriptModuleCollectionResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

		auto& Collection = InternalResult.GetCollection();
		Collection.Id	 = AssetCollection.Id;

		Callback(InternalResult);
	};

	// Create asset collection that represents script module collection
	AssetSystem->CreateAssetCollection(nullptr,
									   nullptr,
									   ScriptModuleNamespace,
									   nullptr,
									   EAssetCollectionType::SCRIPT_MODULE_COLLECTION,
									   nullptr,
									   CreateAssetCollectionCallback);
}

} // namespace csp::systems
