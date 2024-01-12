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
#include "CSP/Systems/Assets/AssetSystem.h"

#include "CSP/Systems/MaterialOverrides/MaterialDefinition.h"
#include "CallHelpers.h"
#include "Common/Algorithm.h"
#include "LODHelpers.h"
#include "Services/PrototypeService/Api.h"
#include "Systems/ResultHelpers.h"
#include "Web/RemoteFileManager.h"

// StringFormat needs to be here due to clashing headers
#include "CSP/Common/StringFormat.h"


namespace chs = csp::services::generated::prototypeservice;

constexpr int DEFAULT_SKIP_NUMBER					= 0;
constexpr int DEFAULT_RESULT_MAX_NUMBER				= 100;
constexpr auto MATERIALDEFINITION_ASSET_NAME_PREFIX = "OKO_MaterialDefinition_";


namespace
{

csp::common::String ConvertAssetCollectionTypeToString(csp::systems::EAssetCollectionType AssetCollectionType)
{
	if (AssetCollectionType == csp::systems::EAssetCollectionType::DEFAULT)
		return "Default";
	else if (AssetCollectionType == csp::systems::EAssetCollectionType::FOUNDATION_INTERNAL)
		return "FoundationInternal";
	else if (AssetCollectionType == csp::systems::EAssetCollectionType::COMMENT_CONTAINER)
		return "CommentContainer";
	else if (AssetCollectionType == csp::systems::EAssetCollectionType::COMMENT)
		return "Comment";
	else if (AssetCollectionType == csp::systems::EAssetCollectionType::SPACE_THUMBNAIL)
		return "SpaceThumbnail";
	else
	{
		assert(false && "Unsupported AssetCollection Type!");
		return "Default";
	}
}

csp::common::String ConvertAssetTypeToString(csp::systems::EAssetType AssetType)
{
	if (AssetType == csp::systems::EAssetType::IMAGE)
		return "Image";
	else if (AssetType == csp::systems::EAssetType::THUMBNAIL)
		return "Thumbnail";
	else if (AssetType == csp::systems::EAssetType::SIMULATION)
		return "Simulation";
	else if (AssetType == csp::systems::EAssetType::MODEL)
		return "Model";
	else if (AssetType == csp::systems::EAssetType::VIDEO)
		return "Video";
	else if (AssetType == csp::systems::EAssetType::SCRIPT_LIBRARY)
		return "ScriptLibrary";
	else if (AssetType == csp::systems::EAssetType::HOLOCAP_VIDEO)
		return "HolocapVideo";
	else if (AssetType == csp::systems::EAssetType::HOLOCAP_AUDIO)
		return "HolocapAudio";
	else if (AssetType == csp::systems::EAssetType::AUDIO)
		return "Audio";
	else if (AssetType == csp::systems::EAssetType::MATERIAL_DEFINITION)
		return "MaterialDefinition";
	else
	{
		assert(false && "Unsupported Asset Type!");
		return "Image";
	}
}

std::shared_ptr<chs::PrototypeDto>
	CreatePrototypeDto(const csp::common::Optional<csp::common::String>& SpaceId,
					   const csp::common::Optional<csp::common::String>& ParentAssetCollectionId,
					   const csp::common::String& AssetCollectionName,
					   const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& Metadata,
					   csp::systems::EAssetCollectionType Type,
					   const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags)
{
	auto PrototypeInfo = std::make_shared<chs::PrototypeDto>();
	PrototypeInfo->SetName(AssetCollectionName);

	PrototypeInfo->SetType(ConvertAssetCollectionTypeToString(Type));

	if (SpaceId.HasValue())
	{
		const std::vector<csp::common::String> GroupIds = {*SpaceId};
		PrototypeInfo->SetGroupIds(GroupIds);
	}

	if (ParentAssetCollectionId.HasValue())
	{
		PrototypeInfo->SetParentId(*ParentAssetCollectionId);
	}

	if (Metadata.HasValue())
	{
		std::map<csp::common::String, csp::common::String> DTOMetadata;

		auto* Keys = Metadata->Keys();

		for (auto idx = 0; idx < Keys->Size(); ++idx)
		{
			auto Key   = Keys->operator[](idx);
			auto Value = Metadata->operator[](Key);
			DTOMetadata.insert(std::pair<csp::common::String, csp::common::String>(Key, Value));
		}

		PrototypeInfo->SetMetadata(DTOMetadata);
	}

	if (Tags.HasValue())
	{
		std::vector<csp::common::String> TagsVector;
		TagsVector.reserve(Tags->Size());

		for (size_t idx = 0; idx < Tags->Size(); ++idx)
		{
			TagsVector.push_back((*Tags)[idx]);
		}

		PrototypeInfo->SetTags(TagsVector);
	}

	return PrototypeInfo;
}

} // namespace


namespace csp::systems
{

AssetSystem::AssetSystem() : SystemBase(), PrototypeAPI(nullptr), AssetDetailAPI(nullptr), FileManager(nullptr)
{
}

AssetSystem::AssetSystem(csp::web::WebClient* InWebClient) : SystemBase(InWebClient)
{
	PrototypeAPI   = CSP_NEW chs::PrototypeApi(InWebClient);
	AssetDetailAPI = CSP_NEW chs::AssetDetailApi(InWebClient);

	FileManager = CSP_NEW csp::web::RemoteFileManager(InWebClient);
}

AssetSystem::~AssetSystem()
{
	CSP_DELETE(FileManager);

	CSP_DELETE(AssetDetailAPI);
	CSP_DELETE(PrototypeAPI);
}

void AssetSystem::CreateAssetCollection(const csp::common::Optional<csp::common::String>& InSpaceId,
										const csp::common::Optional<csp::common::String>& ParentAssetCollectionId,
										const csp::common::String& AssetCollectionName,
										const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& Metadata,
										EAssetCollectionType Type,
										const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags,
										AssetCollectionResultCallback Callback)
{
	csp::common::Optional<csp::common::String> SpaceId;

	if (InSpaceId.HasValue())
	{
		SpaceId = *InSpaceId;
	}

	const auto PrototypeInfo = CreatePrototypeDto(SpaceId, ParentAssetCollectionId, AssetCollectionName, Metadata, Type, Tags);

	const csp::services::ResponseHandlerPtr ResponseHandler
		= PrototypeAPI->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(
			Callback,
			nullptr,
			csp::web::EResponseCodes::ResponseCreated);

	static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesPost(PrototypeInfo, ResponseHandler);
}

void AssetSystem::DeleteAssetCollection(const AssetCollection& AssetCollection, NullResultCallback Callback)
{
	const csp::common::String PrototypeId = AssetCollection.Id;

	if (PrototypeId.IsEmpty())
	{
		CSP_LOG_MSG(LogLevel::Error, "A delete of an asset collection was issued without an ID. You have to provide an asset collection ID.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

		return;
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= PrototypeAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback,
																									nullptr,
																									csp::web::EResponseCodes::ResponseNoContent);

	static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesIdDelete(PrototypeId, ResponseHandler);
}

void AssetSystem::GetAssetCollectionById(const csp::common::String& AssetCollectionId, AssetCollectionResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= PrototypeAPI->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(Callback, nullptr);

	static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesIdGet(AssetCollectionId, ResponseHandler);
}

void AssetSystem::GetAssetCollectionByName(const csp::common::String& AssetCollectionName, AssetCollectionResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= PrototypeAPI->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(Callback, nullptr);

	static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesNameNameGet(AssetCollectionName, ResponseHandler);
}

void AssetSystem::GetAssetCollectionsByIds(const csp::common::Array<csp::common::String>& AssetCollectionIds, AssetCollectionsResultCallback Callback)
{
	if (AssetCollectionIds.IsEmpty())
	{
		CSP_LOG_MSG(LogLevel::Error, "You have to provide at least one AssetCollectionId");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<AssetCollectionsResult>());

		return;
	}

	std::vector<csp::common::String> Ids;
	Ids.reserve(AssetCollectionIds.Size());

	for (int i = 0; i < AssetCollectionIds.Size(); ++i)
	{
		Ids.push_back(AssetCollectionIds[i]);
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= PrototypeAPI->CreateHandler<AssetCollectionsResultCallback, AssetCollectionsResult, void, csp::services::DtoArray<chs::PrototypeDto>>(
			Callback,
			nullptr);

	// Use `GET /api/v1/prototypes` and only pass asset collection IDs
	static_cast<chs::PrototypeApi*>(PrototypeAPI)
		->apiV1PrototypesGet(std::nullopt,
							 std::nullopt,
							 Ids,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 ResponseHandler);
}

void AssetSystem::GetAssetCollectionsByCriteria(const csp::common::Optional<csp::common::String>& SpaceId,
												const csp::common::Optional<csp::common::String>& AssetCollectionParentId,
												const csp::common::Optional<EAssetCollectionType>& AssetCollectionType,
												const csp::common::Optional<csp::common::Array<csp::common::String>>& AssetCollectionTags,
												const csp::common::Optional<csp::common::Array<csp::common::String>>& AssetCollectionNames,
												const csp::common::Optional<int>& ResultsSkipNumber,
												const csp::common::Optional<int>& ResultsMaxNumber,
												AssetCollectionsResultCallback Callback)
{
	std::optional<std::vector<csp::common::String>> GroupIds;

	if (SpaceId.HasValue())
	{
		GroupIds.emplace({*SpaceId});
	}

	std::optional<csp::common::String> ParentId;

	if (AssetCollectionParentId.HasValue())
	{
		ParentId = *AssetCollectionParentId;
	}

	std::optional<std::vector<csp::common::String>> Tags;

	if (AssetCollectionTags.HasValue())
	{
		Tags.emplace(std::vector<csp::common::String>());
		Tags->reserve(AssetCollectionTags->Size());

		for (size_t idx = 0; idx < AssetCollectionTags->Size(); ++idx)
		{
			Tags->push_back({(*AssetCollectionTags)[idx]});
		}
	}

	std::optional<std::vector<csp::common::String>> Names;

	if (AssetCollectionNames.HasValue())
	{
		Names.emplace(std::vector<csp::common::String>());
		Names->reserve(AssetCollectionNames->Size());

		for (size_t idx = 0; idx < AssetCollectionNames->Size(); ++idx)
		{
			Names->push_back({(*AssetCollectionNames)[idx]});
		}
	}

	std::optional<int32_t> Skip;

	if (ResultsSkipNumber.HasValue())
	{
		Skip = *ResultsSkipNumber;
	}
	else
	{
		Skip = DEFAULT_SKIP_NUMBER;
	}

	std::optional<int32_t> Limit;

	if (ResultsMaxNumber.HasValue())
	{
		Limit = *ResultsMaxNumber;
	}
	else
	{
		Limit = DEFAULT_RESULT_MAX_NUMBER;
	}

	std::optional<std::vector<csp::common::String>> AssetsType;

	if (AssetCollectionType.HasValue())
	{
		AssetsType.emplace(std::vector<csp::common::String>());
		AssetsType->push_back({ConvertAssetCollectionTypeToString(*AssetCollectionType)});
	};

	csp::services::ResponseHandlerPtr ResponseHandler
		= PrototypeAPI->CreateHandler<AssetCollectionsResultCallback, AssetCollectionsResult, void, csp::services::DtoArray<chs::PrototypeDto>>(
			Callback,
			nullptr);

	static_cast<chs::PrototypeApi*>(PrototypeAPI)
		->apiV1PrototypesGet(Tags,
							 std::nullopt,
							 std::nullopt,
							 Names,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 ParentId,
							 GroupIds,
							 AssetsType,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 std::nullopt,
							 Skip,
							 Limit,
							 std::nullopt,
							 std::nullopt,
							 ResponseHandler);
}

void AssetSystem::UpdateAssetCollectionMetadata(const AssetCollection& AssetCollection,
												const csp::common::Map<csp::common::String, csp::common::String>& NewMetadata,
												AssetCollectionResultCallback Callback)
{
	csp::common::Optional<csp::common::String> SpaceId;

	if (!AssetCollection.SpaceIds.IsEmpty())
	{
		SpaceId = AssetCollection.SpaceIds[0];
	}

	auto PrototypeInfo = CreatePrototypeDto(SpaceId, AssetCollection.ParentId, AssetCollection.Name, NewMetadata, AssetCollection.Type, nullptr);

	csp::services::ResponseHandlerPtr ResponseHandler
		= PrototypeAPI->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(Callback, nullptr);

	static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesIdPut(AssetCollection.Id, PrototypeInfo, ResponseHandler);
}

void AssetSystem::CreateAsset(const AssetCollection& AssetCollection,
							  const csp::common::String& Name,
							  const csp::common::Optional<csp::common::String>& ThirdPartyPackagedAssetIdentifier,
							  const csp::common::Optional<csp::systems::EThirdPartyPlatform>& ThirdPartyPlatform,
							  EAssetType Type,
							  AssetResultCallback Callback)
{
	auto AssetInfo = std::make_shared<chs::AssetDetailDto>();
	AssetInfo->SetName(Name);
	AssetInfo->SetPrototypeId(AssetCollection.Id);
	csp::common::String InAddressableId;

	if (ThirdPartyPackagedAssetIdentifier.HasValue() || ThirdPartyPlatform.HasValue())
	{
		if (ThirdPartyPackagedAssetIdentifier.HasValue() && ThirdPartyPlatform.HasValue())
		{
			InAddressableId = csp::common::StringFormat("%s|%d", ThirdPartyPackagedAssetIdentifier->c_str(), static_cast<int>(*ThirdPartyPlatform));
		}
		else if (ThirdPartyPackagedAssetIdentifier.HasValue())
		{
			InAddressableId
				= csp::common::StringFormat("%s|%d", ThirdPartyPackagedAssetIdentifier->c_str(), static_cast<int>(EThirdPartyPlatform::NONE));
		}
		else if (ThirdPartyPlatform.HasValue())
		{
			InAddressableId = csp::common::StringFormat("%s|%d", "", static_cast<int>(*ThirdPartyPlatform));
		}

		// TODO: CHS naming refactor planned for AssetDetailDto.m_AddressableId, becoming AssetDetailDto.m_ThirdPartyReferenceId
		AssetInfo->SetAddressableId(InAddressableId);
	}

	AssetInfo->SetAssetType(ConvertAssetTypeToString(Type));

	// TODO: Move this to a separate function when we have some different values than DEFAULT
	std::vector<csp::common::String> Styles;
	const auto DefaultStyle = "Default";
	Styles.push_back(DefaultStyle);
	AssetInfo->SetStyle(Styles);

	// TODO: Move this to a separate function when we have some different values than DEFAULT
	std::vector<csp::common::String> Platform;
	const auto DefaultPlatform = "Default";
	Platform.push_back(DefaultPlatform);
	AssetInfo->SetSupportedPlatforms(Platform);

	csp::services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<AssetResultCallback, AssetResult, void, chs::AssetDetailDto>(Callback,
																									 nullptr,
																									 csp::web::EResponseCodes::ResponseCreated);

	static_cast<chs::AssetDetailApi*>(AssetDetailAPI)->apiV1PrototypesPrototypeIdAssetDetailsPost(AssetCollection.Id, AssetInfo, ResponseHandler);
}

void AssetSystem::UpdateAsset(const Asset& Asset, AssetResultCallback Callback)
{
	auto AssetInfo = std::make_shared<chs::AssetDetailDto>();
	AssetInfo->SetName(Asset.Name);
	AssetInfo->SetId(Asset.Id);
	AssetInfo->SetFileName(Asset.FileName);
	AssetInfo->SetLanguageCode(Asset.LanguageCode);
	AssetInfo->SetPrototypeId(Asset.AssetCollectionId);
	AssetInfo->SetStyle(csp::common::Convert(Asset.Styles));

	// TODO: Move this to a separate function when we have some different values than DEFAULT
	std::vector<csp::common::String> Platform;
	const auto DefaultPlatform = "Default";
	Platform.push_back(DefaultPlatform);
	AssetInfo->SetSupportedPlatforms(Platform);

	if (!Asset.ExternalUri.IsEmpty() && !Asset.ExternalMimeType.IsEmpty())
	{
		AssetInfo->SetExternalUri(Asset.ExternalUri);
		AssetInfo->SetExternalMimeType(Asset.ExternalMimeType);
	}

	AssetInfo->SetAddressableId(csp::common::StringFormat("%s|%d",
														  Asset.GetThirdPartyPackagedAssetIdentifier().c_str(),
														  static_cast<int>(Asset.GetThirdPartyPlatformType())));

	AssetInfo->SetAssetType(ConvertAssetTypeToString(Asset.Type));
	csp::services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<AssetResultCallback, AssetResult, void, chs::AssetDetailDto>(Callback,
																									 nullptr,
																									 csp::web::EResponseCodes::ResponseCreated);
	static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
		->apiV1PrototypesPrototypeIdAssetDetailsAssetDetailIdPut(Asset.AssetCollectionId, Asset.Id, AssetInfo, ResponseHandler);
}

void AssetSystem::DeleteAsset(const AssetCollection& AssetCollection, const Asset& Asset, NullResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback,
																									  nullptr,
																									  csp::web::EResponseCodes::ResponseNoContent);

	static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
		->apiV1PrototypesPrototypeIdAssetDetailsAssetDetailIdDelete(AssetCollection.Id, Asset.Id, ResponseHandler);
}

void AssetSystem::GetAssetsInCollection(const AssetCollection& AssetCollection, AssetsResultCallback Callback)
{
	std::vector<csp::common::String> PrototypeIds = {AssetCollection.Id};

	csp::services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<AssetsResultCallback, AssetsResult, void, csp::services::DtoArray<chs::AssetDetailDto>>(Callback, nullptr);

	static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
		->apiV1PrototypesAssetDetailsGet(std::nullopt,
										 std::nullopt,
										 std::nullopt,
										 std::nullopt,
										 std::nullopt,
										 std::nullopt,
										 PrototypeIds,
										 std::nullopt,
										 std::nullopt,
										 std::nullopt,
										 ResponseHandler);
}

void AssetSystem::GetAssetById(const csp::common::String& AssetCollectionId, const csp::common::String& AssetId, AssetResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<AssetResultCallback, AssetResult, void, chs::AssetDetailDto>(Callback, nullptr);

	static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
		->apiV1PrototypesPrototypeIdAssetDetailsAssetDetailIdGet(AssetCollectionId, AssetId, ResponseHandler);
}

void AssetSystem::GetAssetsByCriteria(const csp::common::Array<csp::common::String>& AssetCollectionIds,
									  const csp::common::Optional<csp::common::Array<csp::common::String>>& AssetIds,
									  const csp::common::Optional<csp::common::Array<csp::common::String>>& AssetNames,
									  const csp::common::Optional<csp::common::Array<EAssetType>>& AssetTypes,
									  AssetsResultCallback Callback)
{
	if (AssetCollectionIds.IsEmpty())
	{
		CSP_LOG_MSG(LogLevel::Error, "You have to provide at least one AssetCollectionId");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<AssetsResult>());

		return;
	}

	std::vector<csp::common::String> PrototypeIds;
	PrototypeIds.reserve(AssetCollectionIds.Size());

	for (size_t idx = 0; idx < AssetCollectionIds.Size(); ++idx)
	{
		PrototypeIds.push_back(AssetCollectionIds[idx]);
	}

	std::optional<std::vector<csp::common::String>> AssetDetailIds;

	if (AssetIds.HasValue())
	{
		AssetDetailIds.emplace(std::vector<csp::common::String>());
		AssetDetailIds->reserve(AssetIds->Size());

		for (size_t idx = 0; idx < AssetIds->Size(); ++idx)
		{
			AssetDetailIds->push_back({(*AssetIds)[idx]});
		}
	}

	std::optional<std::vector<csp::common::String>> AssetDetailNames;

	if (AssetNames.HasValue())
	{
		AssetDetailNames.emplace(std::vector<csp::common::String>());
		AssetDetailNames->reserve(AssetNames->Size());

		for (size_t idx = 0; idx < AssetNames->Size(); ++idx)
		{
			AssetDetailNames->push_back((*AssetNames)[idx]);
		}
	}

	std::optional<std::vector<csp::common::String>> AssetDetailTypes;

	if (AssetTypes.HasValue())
	{
		AssetDetailTypes.emplace(std::vector<csp::common::String>());
		AssetDetailTypes->reserve(AssetTypes->Size());

		for (size_t idx = 0; idx < AssetTypes->Size(); ++idx)
		{
			AssetDetailTypes->push_back(ConvertAssetTypeToString((*AssetTypes)[idx]));
		}
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<AssetsResultCallback, AssetsResult, void, csp::services::DtoArray<chs::AssetDetailDto>>(Callback, nullptr);

	static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
		->apiV1PrototypesAssetDetailsGet(AssetDetailIds,
										 std::nullopt,
										 AssetDetailTypes,
										 std::nullopt,
										 AssetDetailNames,
										 std::nullopt,
										 PrototypeIds,
										 std::nullopt,
										 std::nullopt,
										 std::nullopt,
										 ResponseHandler);
}

void AssetSystem::GetAssetsByCollectionIds(const csp::common::Array<csp::common::String>& AssetCollectionIds, AssetsResultCallback Callback)
{
	if (AssetCollectionIds.IsEmpty())
	{
		CSP_LOG_MSG(LogLevel::Error, "You have to provide at least one AssetCollectionId");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<AssetsResult>());

		return;
	}

	std::vector<csp::common::String> Ids;

	for (int i = 0; i < AssetCollectionIds.Size(); ++i)
	{
		Ids.push_back(AssetCollectionIds[i]);
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<AssetsResultCallback, AssetsResult, void, csp::services::DtoArray<chs::AssetDetailDto>>(Callback, nullptr);

	// Use `GET /api/v1/prototypes/asset-details` and only pass asset collection IDs
	static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
		->apiV1PrototypesAssetDetailsGet(std::nullopt,
										 std::nullopt,
										 std::nullopt,
										 std::nullopt,
										 std::nullopt,
										 std::nullopt,
										 Ids,
										 std::nullopt,
										 std::nullopt,
										 std::nullopt,
										 ResponseHandler);
}

void AssetSystem::UploadAssetData(const AssetCollection& AssetCollection,
								  const Asset& Asset,
								  const AssetDataSource& AssetDataSource,
								  UriResultCallback Callback)
{
	UploadAssetDataEx(AssetCollection, Asset, AssetDataSource, csp::common::CancellationToken::Dummy(), Callback);
}

void AssetSystem::UploadAssetDataEx(const AssetCollection& AssetCollection,
									const Asset& Asset,
									const AssetDataSource& AssetDataSource,
									csp::common::CancellationToken& CancellationToken,
									UriResultCallback Callback)
{
	if (Asset.Name.IsEmpty())
	{
		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<UriResult>());

		return;
	}

	auto FormFile = std::make_shared<csp::web::HttpPayload>();
	AssetDataSource.SetUploadContent(WebClient, FormFile.get(), Asset);

	UriResultCallback InternalCallback = [Callback, Asset](const UriResult& Result)
	{
		if (Result.GetFailureReason() != systems::ERequestFailureReason::None)
		{
			CSP_LOG_ERROR_MSG(csp::common::String("Asset with Id %s has failed to upload").c_str());
		}

		INVOKE_IF_NOT_NULL(Callback, Result);
	};

	csp::services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<UriResultCallback, UriResult, void, csp::services::NullDto>(InternalCallback,
																									nullptr,
																									csp::web::EResponseCodes::ResponseOK);

	static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
		->apiV1PrototypesPrototypeIdAssetDetailsAssetDetailIdBlobPost(AssetCollection.Id,
																	  Asset.Id,
																	  std::nullopt,
																	  FormFile,
																	  ResponseHandler,
																	  CancellationToken);
}

void AssetSystem::DownloadAssetData(const Asset& Asset, AssetDataResultCallback Callback)
{
	DownloadAssetDataEx(Asset, csp::common::CancellationToken::Dummy(), Callback);
}

void AssetSystem::DownloadAssetDataEx(const Asset& Asset, csp::common::CancellationToken& CancellationToken, AssetDataResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<AssetDataResultCallback, AssetDataResult, void, csp::services::AssetFileDto>(Callback, nullptr);

	FileManager->GetFile(Asset.Uri, ResponseHandler, CancellationToken);
}

void AssetSystem::GetAssetDataSize(const Asset& Asset, UInt64ResultCallback Callback)
{
	HTTPHeadersResultCallback InternalCallback = [Callback](const HTTPHeadersResult& Result)
	{
		UInt64Result InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

		if (Result.GetResultCode() == csp::systems::EResultCode::Success)
		{
			auto& Headers		= Result.GetValue();
			auto& ContentLength = Headers["content-length"];
			auto Value			= std::strtoull(ContentLength.c_str(), nullptr, 10);
			InternalResult.SetValue(Value);
		}

		INVOKE_IF_NOT_NULL(Callback, InternalResult);
	};

	csp::services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<HTTPHeadersResultCallback, HTTPHeadersResult, void, csp::services::NullDto>(InternalCallback, nullptr);

	FileManager->GetResponseHeaders(Asset.Uri, ResponseHandler);
}

CSP_ASYNC_RESULT void AssetSystem::GetLODChain(const AssetCollection& AssetCollection, LODChainResultCallback Callback)
{
	auto GetAssetsCallback = [AssetCollection, Callback](const csp::systems::AssetsResult& AssetsResult)
	{
		LODChainResult LODResult(AssetsResult.GetResultCode(), AssetsResult.GetHttpResultCode());

		if (AssetsResult.GetResultCode() == csp::systems::EResultCode::Success)
		{
			LODChain Chain = CreateLODChainFromAssets(AssetsResult.GetAssets(), AssetCollection.Id);
			LODResult.SetLODChain(std::move(Chain));
		}

		INVOKE_IF_NOT_NULL(Callback, LODResult);
	};

	GetAssetsByCriteria({AssetCollection.Id}, nullptr, nullptr, csp::common::Array<EAssetType> {EAssetType::MODEL}, GetAssetsCallback);
}

CSP_ASYNC_RESULT_WITH_PROGRESS void
	AssetSystem::RegisterAssetToLODChain(const AssetCollection& AssetCollection, const Asset& Asset, int LODLevel, AssetResultCallback Callback)
{
	// GetAssetsByCriteria
	auto GetAssetsCallback = [this, AssetCollection, Asset, LODLevel, Callback](const AssetsResult& AssetsResult)
	{
		if (AssetsResult.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			return;
		}

		if (AssetsResult.GetResultCode() == csp::systems::EResultCode::Failed)
		{
			INVOKE_IF_NOT_NULL(Callback, AssetsResult);

			return;
		}

		const csp::common::Array<csp::systems::Asset>& Assets = AssetsResult.GetAssets();
		LODChain Chain										  = CreateLODChainFromAssets(Assets, AssetCollection.Id);

		if (!ValidateNewLODLevelForChain(Chain, LODLevel))
		{
			CSP_LOG_MSG(LogLevel::Error, "LOD level already exists in chain");

			INVOKE_IF_NOT_NULL(Callback, AssetsResult);

			return;
		}

		// UpdateAsset
		auto UpdateAssetCallback = [this, AssetCollection, Callback, Assets](const AssetResult& AssetResult)
		{
			INVOKE_IF_NOT_NULL(Callback, AssetResult);
		};

		// Add new LOD style
		csp::systems::Asset NewAsset = Asset;
		csp::common::Array<csp::common::String> NewStyles(NewAsset.Styles.Size() + 1);

		for (int i = 0; i < NewAsset.Styles.Size(); ++i)
		{
			NewStyles[i] = NewAsset.Styles[i];
		}

		NewStyles[NewAsset.Styles.Size()] = CreateLODStyleVar(LODLevel);
		NewAsset.Styles					  = std::move(NewStyles);

		UpdateAsset(NewAsset, UpdateAssetCallback);
	};

	GetAssetsByCriteria({Asset.AssetCollectionId}, nullptr, nullptr, csp::common::Array<EAssetType> {EAssetType::MODEL}, GetAssetsCallback);
}

CSP_ASYNC_RESULT void AssetSystem::UploadMaterialDefinition(const MaterialDefinition& Definition,
															const csp::common::String& AssetCollectionId,
															const csp::common::String& AssetId,
															NullResultCallback Callback)
{
	csp::common::String MaterialDefinitionJson = Definition.SerialiseToJson();

	CSP_LOG_MSG(LogLevel::Log, "Parsed the MaterialDefinition object to json.");

	const csp::common::String ModuleDefinitionAssetName
		= csp::common::StringFormat("%s%s", MATERIALDEFINITION_ASSET_NAME_PREFIX, Definition.GetMaterialName().c_str());
	const csp::common::String ModuleDefinitionAssetFileName
		= csp::common::StringFormat("%s%s.json", MATERIALDEFINITION_ASSET_NAME_PREFIX, Definition.GetMaterialName().c_str());

	AssetCollection InternalAssetCollection;
	InternalAssetCollection.Id = AssetCollectionId;
	Asset InternalAsset;
	InternalAsset.Id	   = AssetId;
	InternalAsset.Name	   = ModuleDefinitionAssetName;
	InternalAsset.FileName = ModuleDefinitionAssetFileName;

	BufferAssetDataSource AssetData;
	AssetData.SetMimeType("application/json");
	AssetData.Buffer	   = const_cast<char*>(MaterialDefinitionJson.c_str());
	AssetData.BufferLength = MaterialDefinitionJson.Length();

	UriResultCallback UploadMaterialDefinitionCallback = [Callback](const UriResult& UploadResult)
	{
		if (UploadResult.GetResultCode() == csp::systems::EResultCode::Failed)
		{
			CSP_LOG_FORMAT(LogLevel::Log,
						   "Failed to upload generated material override file. ResCode: %d, HttpResCode: %d",
						   static_cast<int>(UploadResult.GetResultCode()),
						   UploadResult.GetHttpResultCode());
		}

		const NullResult InternalResult(UploadResult.GetResultCode(), UploadResult.GetHttpResultCode());
		Callback(InternalResult);
	};

	UploadAssetData(InternalAssetCollection, InternalAsset, AssetData, UploadMaterialDefinitionCallback);
}

CSP_ASYNC_RESULT void AssetSystem::GetMaterialDefinition(const csp::common::String& AssetCollectionId,
														 const csp::common::String& AssetId,
														 MaterialDefinitionResultCallback Callback)
{
	AssetResultCallback AssetResultCallbackHandler = [this, Callback](const AssetResult& Result)
	{
		if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			return;
		}

		if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
		{
			const MaterialDefinitionResult FailureResult(Result.GetResultCode(), Result.GetHttpResultCode());
			Callback(FailureResult);
			return;
		}

		AssetDataResultCallback AssetDataResultCallbackHandler = [Callback](const AssetDataResult& DataResult)
		{
			if (DataResult.GetResultCode() == csp::systems::EResultCode::InProgress)
			{
				return;
			}

			MaterialDefinitionResult InternalResult(DataResult.GetResultCode(), DataResult.GetHttpResultCode());

			if (DataResult.GetResultCode() == csp::systems::EResultCode::Failed)
			{
				Callback(InternalResult);

				return;
			}

			size_t DownloadedAssetDataSize = DataResult.GetDataLength();
			auto DownloadedAssetData	   = CSP_NEW char[DownloadedAssetDataSize + 1];
			std::memcpy(DownloadedAssetData, DataResult.GetData(), DownloadedAssetDataSize);
			DownloadedAssetData[DownloadedAssetDataSize] = 0x0; // null terminate

			MaterialDefinition& InternalMaterialDefinition = InternalResult.GetMaterialDefinition();

			if (!InternalMaterialDefinition.DeserialiseFromJson(DownloadedAssetData))
			{
				CSP_LOG_MSG(LogLevel::Error, "Deserialisation of MaterialDefinition failed.");
			}

			Callback(InternalResult);
		};

		const Asset& InternalAsset = Result.GetAsset();
		this->DownloadAssetData(InternalAsset, AssetDataResultCallbackHandler);
	};

	GetAssetById(AssetCollectionId, AssetId, AssetResultCallbackHandler);
}

} // namespace csp::systems
