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

#include "CallHelpers.h"
#include "Common/Algorithm.h"
#include "LODHelpers.h"
#include "Services/PrototypeService/Api.h"
#include "Systems/ResultHelpers.h"
#include "Web/RemoteFileManager.h"

// StringFormat needs to be here due to clashing headers
#include "CSP/Common/StringFormat.h"


using namespace csp;
using namespace csp::common;

namespace chs = services::generated::prototypeservice;

constexpr int DEFAULT_SKIP_NUMBER		= 0;
constexpr int DEFAULT_RESULT_MAX_NUMBER = 100;


namespace
{

String ConvertAssetCollectionTypeToString(systems::EAssetCollectionType AssetCollectionType)
{
	if (AssetCollectionType == systems::EAssetCollectionType::DEFAULT)
		return "Default";
	else if (AssetCollectionType == systems::EAssetCollectionType::FOUNDATION_INTERNAL)
		return "FoundationInternal";
	else if (AssetCollectionType == systems::EAssetCollectionType::COMMENT_CONTAINER)
		return "CommentContainer";
	else if (AssetCollectionType == systems::EAssetCollectionType::COMMENT)
		return "Comment";
	else if (AssetCollectionType == systems::EAssetCollectionType::SPACE_THUMBNAIL)
		return "SpaceThumbnail";
	else
	{
		assert(false && "Unsupported AssetCollection Type!");
		return "Default";
	}
}

String ConvertAssetTypeToString(systems::EAssetType AssetType)
{
	if (AssetType == systems::EAssetType::IMAGE)
		return "Image";
	else if (AssetType == systems::EAssetType::THUMBNAIL)
		return "Thumbnail";
	else if (AssetType == systems::EAssetType::SIMULATION)
		return "Simulation";
	else if (AssetType == systems::EAssetType::MODEL)
		return "Model";
	else if (AssetType == systems::EAssetType::VIDEO)
		return "Video";
	else if (AssetType == systems::EAssetType::SCRIPT_LIBRARY)
		return "ScriptLibrary";
	else if (AssetType == systems::EAssetType::HOLOCAP_VIDEO)
		return "HolocapVideo";
	else if (AssetType == systems::EAssetType::HOLOCAP_AUDIO)
		return "HolocapAudio";
	else if (AssetType == systems::EAssetType::AUDIO)
		return "Audio";
	else
	{
		assert(false && "Unsupported Asset Type!");
		return "Image";
	}
}

std::shared_ptr<chs::PrototypeDto> CreatePrototypeDto(const Optional<String>& SpaceId,
													  const Optional<String>& ParentAssetCollectionId,
													  const String& AssetCollectionName,
													  const Optional<Map<String, String>>& Metadata,
													  systems::EAssetCollectionType Type,
													  const Optional<Array<String>>& Tags)
{
	auto PrototypeInfo = std::make_shared<chs::PrototypeDto>();
	PrototypeInfo->SetName(AssetCollectionName);

	PrototypeInfo->SetType(ConvertAssetCollectionTypeToString(Type));

	if (SpaceId.HasValue())
	{
		const std::vector<String> GroupIds = {*SpaceId};
		PrototypeInfo->SetGroupIds(GroupIds);
	}

	if (ParentAssetCollectionId.HasValue())
	{
		PrototypeInfo->SetParentId(*ParentAssetCollectionId);
	}

	if (Metadata.HasValue())
	{
		std::map<String, String> DTOMetadata;

		auto* Keys = Metadata->Keys();

		for (auto idx = 0; idx < Keys->Size(); ++idx)
		{
			auto Key   = Keys->operator[](idx);
			auto Value = Metadata->operator[](Key);
			DTOMetadata.insert(std::pair<String, String>(Key, Value));
		}

		PrototypeInfo->SetMetadata(DTOMetadata);
	}

	if (Tags.HasValue())
	{
		std::vector<String> TagsVector;
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

AssetSystem::AssetSystem(web::WebClient* InWebClient) : SystemBase(InWebClient)
{
	PrototypeAPI   = CSP_NEW chs::PrototypeApi(InWebClient);
	AssetDetailAPI = CSP_NEW chs::AssetDetailApi(InWebClient);

	FileManager = CSP_NEW web::RemoteFileManager(InWebClient);
}

AssetSystem::~AssetSystem()
{
	CSP_DELETE(FileManager);

	CSP_DELETE(AssetDetailAPI);
	CSP_DELETE(PrototypeAPI);
}

void AssetSystem::CreateAssetCollection(const Optional<String>& InSpaceId,
										const Optional<String>& ParentAssetCollectionId,
										const String& AssetCollectionName,
										const Optional<Map<String, String>>& Metadata,
										EAssetCollectionType Type,
										const Optional<Array<String>>& Tags,
										AssetCollectionResultCallback Callback)
{
	Optional<String> SpaceId;

	if (InSpaceId.HasValue())
	{
		SpaceId = *InSpaceId;
	}

	const auto PrototypeInfo = CreatePrototypeDto(SpaceId, ParentAssetCollectionId, AssetCollectionName, Metadata, Type, Tags);

	const services::ResponseHandlerPtr ResponseHandler
		= PrototypeAPI->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(
			Callback,
			nullptr,
			web::EResponseCodes::ResponseCreated);

	static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesPost(PrototypeInfo, ResponseHandler);
}

void AssetSystem::DeleteAssetCollection(const AssetCollection& AssetCollection, NullResultCallback Callback)
{
	const String PrototypeId = AssetCollection.Id;

	if (PrototypeId.IsEmpty())
	{
		CSP_LOG_MSG(LogLevel::Error, "A delete of an asset collection was issued without an ID. You have to provide an asset collection ID.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

		return;
	}

	services::ResponseHandlerPtr ResponseHandler
		= PrototypeAPI->CreateHandler<NullResultCallback, NullResult, void, services::NullDto>(Callback,
																							   nullptr,
																							   web::EResponseCodes::ResponseNoContent);

	static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesIdDelete(PrototypeId, ResponseHandler);
}

void AssetSystem::CopyAssetCollectionsToSpace(csp::common::Array<AssetCollection>& SourceAssetCollections,
											  const csp::common::String& DestSpaceId,
											  bool CopyAsync,
											  AssetCollectionsResultCallback Callback)
{
	if (SourceAssetCollections.Size() == 0)
	{
		CSP_LOG_MSG(LogLevel::Error, "No source asset collections were provided whilst attempting to perform a copy to another space.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<AssetCollectionsResult>());
		return;
	}

	csp::common::String SourceSpaceId					= SourceAssetCollections[0].SpaceId;
	std::vector<csp::common::String> AssetCollectionIds = {SourceAssetCollections[0].Id};

	bool AssetCollectionsBelongToSameSpace = true;

	for (size_t i = 1; i < SourceAssetCollections.Size(); ++i)
	{
		AssetCollectionsBelongToSameSpace &= SourceAssetCollections[i].SpaceId == SourceSpaceId;
		AssetCollectionIds.emplace_back(SourceAssetCollections[i].Id);
	}

	// Verify we have a valid space ID to copy from.
	if (SourceSpaceId.IsEmpty())
	{
		CSP_LOG_MSG(
			LogLevel::Error,
			"An asset with no space ID was provided whilst attempting to perform a copy to another space. All assets must have a valid space ID.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<AssetCollectionsResult>());
		return;
	}

	// Verify that all source asset collections belong to the same space. If not, this qualifies as an unsupported operation.
	if (!AssetCollectionsBelongToSameSpace)
	{
		CSP_LOG_MSG(LogLevel::Error, "All asset collections must belong to the same space for a copy operation.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<AssetCollectionsResult>());
		return;
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= PrototypeAPI->CreateHandler<AssetCollectionsResultCallback, AssetCollectionsResult, void, csp::services::DtoArray<chs::PrototypeDto>>(
			Callback,
			nullptr);

	// Use `GET /api/v1/prototypes` and only pass asset collection IDs
	static_cast<chs::PrototypeApi*>(PrototypeAPI)
		->apiV1PrototypesGroupOwnedOriginalGroupIdDuplicateNewGroupIdPost(
			std::nullopt,							// Tags
			std::nullopt,							// TagsAll
			AssetCollectionIds,						// const std::optional<std::vector<utility::string_t>>& Ids
			std::nullopt,							// Names
			std::nullopt,							// PartialNames
			std::nullopt,							// ExcludedIds
			std::nullopt,							// PointOfInterestIds
			std::nullopt,							// ParentId
			std::nullopt,							// GroupIds
			std::nullopt,							// Types
			true,									// HasGroup
			std::nullopt,							// CreatedBy
			std::nullopt,							// PrototypeOwnerIds
			std::nullopt,							// ReadAccessFilters
			std::nullopt,							// WriteAccessFilters
			SourceSpaceId,							// originalGroupId
			DestSpaceId,							// newGroupId
			CopyAsync,								// asyncCall
			ResponseHandler,						// ResponseHandler
			csp::common::CancellationToken::Dummy() // CancellationToken
		);
}

void AssetSystem::GetAssetCollectionById(const String& AssetCollectionId, AssetCollectionResultCallback Callback)
{
	services::ResponseHandlerPtr ResponseHandler
		= PrototypeAPI->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(Callback, nullptr);

	static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesIdGet(AssetCollectionId, ResponseHandler);
}

void AssetSystem::GetAssetCollectionByName(const String& AssetCollectionName, AssetCollectionResultCallback Callback)
{
	services::ResponseHandlerPtr ResponseHandler
		= PrototypeAPI->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(Callback, nullptr);

	static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesNameNameGet(AssetCollectionName, ResponseHandler);
}

void AssetSystem::GetAssetCollectionsByIds(const Array<String>& AssetCollectionIds, AssetCollectionsResultCallback Callback)
{
	if (AssetCollectionIds.IsEmpty())
	{
		CSP_LOG_MSG(LogLevel::Error, "You have to provide at least one AssetCollectionId");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<AssetCollectionsResult>());

		return;
	}

	std::vector<String> Ids;
	Ids.reserve(AssetCollectionIds.Size());

	for (int i = 0; i < AssetCollectionIds.Size(); ++i)
	{
		Ids.push_back(AssetCollectionIds[i]);
	}

	services::ResponseHandlerPtr ResponseHandler
		= PrototypeAPI->CreateHandler<AssetCollectionsResultCallback, AssetCollectionsResult, void, services::DtoArray<chs::PrototypeDto>>(Callback,
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

void AssetSystem::GetAssetCollectionsByCriteria(const Optional<String>& SpaceId,
												const Optional<String>& AssetCollectionParentId,
												const Optional<EAssetCollectionType>& AssetCollectionType,
												const Optional<Array<String>>& AssetCollectionTags,
												const Optional<Array<String>>& AssetCollectionNames,
												const Optional<int>& ResultsSkipNumber,
												const Optional<int>& ResultsMaxNumber,
												AssetCollectionsResultCallback Callback)
{
	std::optional<std::vector<String>> GroupIds;

	if (SpaceId.HasValue())
	{
		GroupIds.emplace({*SpaceId});
	}

	std::optional<String> ParentId;

	if (AssetCollectionParentId.HasValue())
	{
		ParentId = *AssetCollectionParentId;
	}

	std::optional<std::vector<String>> Tags;

	if (AssetCollectionTags.HasValue())
	{
		Tags.emplace(std::vector<String>());
		Tags->reserve(AssetCollectionTags->Size());

		for (size_t idx = 0; idx < AssetCollectionTags->Size(); ++idx)
		{
			Tags->push_back({(*AssetCollectionTags)[idx]});
		}
	}

	std::optional<std::vector<String>> Names;

	if (AssetCollectionNames.HasValue())
	{
		Names.emplace(std::vector<String>());
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

	std::optional<std::vector<String>> AssetsType;

	if (AssetCollectionType.HasValue())
	{
		AssetsType.emplace(std::vector<String>());
		AssetsType->push_back({ConvertAssetCollectionTypeToString(*AssetCollectionType)});
	};

	services::ResponseHandlerPtr ResponseHandler
		= PrototypeAPI->CreateHandler<AssetCollectionsResultCallback, AssetCollectionsResult, void, services::DtoArray<chs::PrototypeDto>>(Callback,
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
												const Map<String, String>& NewMetadata,
												AssetCollectionResultCallback Callback)
{
	auto PrototypeInfo
		= CreatePrototypeDto(AssetCollection.SpaceId, AssetCollection.ParentId, AssetCollection.Name, NewMetadata, AssetCollection.Type, nullptr);

	services::ResponseHandlerPtr ResponseHandler
		= PrototypeAPI->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(Callback, nullptr);

	static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesIdPut(AssetCollection.Id, PrototypeInfo, ResponseHandler);
}

void AssetSystem::CreateAsset(const AssetCollection& AssetCollection,
							  const String& Name,
							  const Optional<String>& ThirdPartyPackagedAssetIdentifier,
							  const Optional<EThirdPartyPlatform>& ThirdPartyPlatform,
							  EAssetType Type,
							  AssetResultCallback Callback)
{
	auto AssetInfo = std::make_shared<chs::AssetDetailDto>();
	AssetInfo->SetName(Name);
	AssetInfo->SetPrototypeId(AssetCollection.Id);
	String InAddressableId;

	if (ThirdPartyPackagedAssetIdentifier.HasValue() || ThirdPartyPlatform.HasValue())
	{
		if (ThirdPartyPackagedAssetIdentifier.HasValue() && ThirdPartyPlatform.HasValue())
		{
			InAddressableId = StringFormat("%s|%d", ThirdPartyPackagedAssetIdentifier->c_str(), static_cast<int>(*ThirdPartyPlatform));
		}
		else if (ThirdPartyPackagedAssetIdentifier.HasValue())
		{
			InAddressableId = StringFormat("%s|%d", ThirdPartyPackagedAssetIdentifier->c_str(), static_cast<int>(EThirdPartyPlatform::NONE));
		}
		else if (ThirdPartyPlatform.HasValue())
		{
			InAddressableId = StringFormat("%s|%d", "", static_cast<int>(*ThirdPartyPlatform));
		}

		// TODO: CHS naming refactor planned for AssetDetailDto.m_AddressableId, becoming AssetDetailDto.m_ThirdPartyReferenceId
		AssetInfo->SetAddressableId(InAddressableId);
	}

	AssetInfo->SetAssetType(ConvertAssetTypeToString(Type));

	// TODO: Move this to a separate function when we have some different values than DEFAULT
	std::vector<String> Styles;
	const auto DefaultStyle = "Default";
	Styles.push_back(DefaultStyle);
	AssetInfo->SetStyle(Styles);

	// TODO: Move this to a separate function when we have some different values than DEFAULT
	std::vector<String> Platform;
	const auto DefaultPlatform = "Default";
	Platform.push_back(DefaultPlatform);
	AssetInfo->SetSupportedPlatforms(Platform);

	services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<AssetResultCallback, AssetResult, void, chs::AssetDetailDto>(Callback,
																									 nullptr,
																									 web::EResponseCodes::ResponseCreated);

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
	AssetInfo->SetStyle(Convert(Asset.Styles));

	// TODO: Move this to a separate function when we have some different values than DEFAULT
	std::vector<String> Platform;
	const auto DefaultPlatform = "Default";
	Platform.push_back(DefaultPlatform);
	AssetInfo->SetSupportedPlatforms(Platform);

	if (!Asset.ExternalUri.IsEmpty() && !Asset.ExternalMimeType.IsEmpty())
	{
		AssetInfo->SetExternalUri(Asset.ExternalUri);
		AssetInfo->SetExternalMimeType(Asset.ExternalMimeType);
	}

	AssetInfo->SetAddressableId(
		StringFormat("%s|%d", Asset.GetThirdPartyPackagedAssetIdentifier().c_str(), static_cast<int>(Asset.GetThirdPartyPlatformType())));

	AssetInfo->SetAssetType(ConvertAssetTypeToString(Asset.Type));
	services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<AssetResultCallback, AssetResult, void, chs::AssetDetailDto>(Callback,
																									 nullptr,
																									 web::EResponseCodes::ResponseCreated);
	static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
		->apiV1PrototypesPrototypeIdAssetDetailsAssetDetailIdPut(Asset.AssetCollectionId, Asset.Id, AssetInfo, ResponseHandler);
}

void AssetSystem::DeleteAsset(const AssetCollection& AssetCollection, const Asset& Asset, NullResultCallback Callback)
{
	services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<NullResultCallback, NullResult, void, services::NullDto>(Callback,
																								 nullptr,
																								 web::EResponseCodes::ResponseNoContent);

	static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
		->apiV1PrototypesPrototypeIdAssetDetailsAssetDetailIdDelete(AssetCollection.Id, Asset.Id, ResponseHandler);
}

void AssetSystem::GetAssetsInCollection(const AssetCollection& AssetCollection, AssetsResultCallback Callback)
{
	std::vector<String> PrototypeIds = {AssetCollection.Id};

	services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<AssetsResultCallback, AssetsResult, void, services::DtoArray<chs::AssetDetailDto>>(Callback, nullptr);

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

void AssetSystem::GetAssetById(const String& AssetCollectionId, const String& AssetId, AssetResultCallback Callback)
{
	services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<AssetResultCallback, AssetResult, void, chs::AssetDetailDto>(Callback, nullptr);

	static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
		->apiV1PrototypesPrototypeIdAssetDetailsAssetDetailIdGet(AssetCollectionId, AssetId, ResponseHandler);
}

void AssetSystem::GetAssetsByCriteria(const Array<String>& AssetCollectionIds,
									  const Optional<Array<String>>& AssetIds,
									  const Optional<Array<String>>& AssetNames,
									  const Optional<Array<EAssetType>>& AssetTypes,
									  AssetsResultCallback Callback)
{
	if (AssetCollectionIds.IsEmpty())
	{
		CSP_LOG_MSG(LogLevel::Error, "You have to provide at least one AssetCollectionId");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<AssetsResult>());

		return;
	}

	std::vector<String> PrototypeIds;
	PrototypeIds.reserve(AssetCollectionIds.Size());

	for (size_t idx = 0; idx < AssetCollectionIds.Size(); ++idx)
	{
		PrototypeIds.push_back(AssetCollectionIds[idx]);
	}

	std::optional<std::vector<String>> AssetDetailIds;

	if (AssetIds.HasValue())
	{
		AssetDetailIds.emplace(std::vector<String>());
		AssetDetailIds->reserve(AssetIds->Size());

		for (size_t idx = 0; idx < AssetIds->Size(); ++idx)
		{
			AssetDetailIds->push_back({(*AssetIds)[idx]});
		}
	}

	std::optional<std::vector<String>> AssetDetailNames;

	if (AssetNames.HasValue())
	{
		AssetDetailNames.emplace(std::vector<String>());
		AssetDetailNames->reserve(AssetNames->Size());

		for (size_t idx = 0; idx < AssetNames->Size(); ++idx)
		{
			AssetDetailNames->push_back((*AssetNames)[idx]);
		}
	}

	std::optional<std::vector<String>> AssetDetailTypes;

	if (AssetTypes.HasValue())
	{
		AssetDetailTypes.emplace(std::vector<String>());
		AssetDetailTypes->reserve(AssetTypes->Size());

		for (size_t idx = 0; idx < AssetTypes->Size(); ++idx)
		{
			AssetDetailTypes->push_back(ConvertAssetTypeToString((*AssetTypes)[idx]));
		}
	}

	services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<AssetsResultCallback, AssetsResult, void, services::DtoArray<chs::AssetDetailDto>>(Callback, nullptr);

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

void AssetSystem::GetAssetsByCollectionIds(const Array<String>& AssetCollectionIds, AssetsResultCallback Callback)
{
	if (AssetCollectionIds.IsEmpty())
	{
		CSP_LOG_MSG(LogLevel::Error, "You have to provide at least one AssetCollectionId");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<AssetsResult>());

		return;
	}

	std::vector<String> Ids;

	for (int i = 0; i < AssetCollectionIds.Size(); ++i)
	{
		Ids.push_back(AssetCollectionIds[i]);
	}

	services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<AssetsResultCallback, AssetsResult, void, services::DtoArray<chs::AssetDetailDto>>(Callback, nullptr);

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
	UploadAssetDataEx(AssetCollection, Asset, AssetDataSource, CancellationToken::Dummy(), Callback);
}

void AssetSystem::UploadAssetDataEx(const AssetCollection& AssetCollection,
									const Asset& Asset,
									const AssetDataSource& AssetDataSource,
									CancellationToken& CancellationToken,
									UriResultCallback Callback)
{
	if (Asset.Name.IsEmpty())
	{
		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<UriResult>());

		return;
	}

	auto FormFile = std::make_shared<web::HttpPayload>();
	AssetDataSource.SetUploadContent(WebClient, FormFile.get(), Asset);

	UriResultCallback InternalCallback = [Callback, Asset](const UriResult& Result)
	{
		if (Result.GetFailureReason() != ERequestFailureReason::None)
		{
			CSP_LOG_ERROR_MSG(String("Asset with Id %s has failed to upload").c_str());
		}

		INVOKE_IF_NOT_NULL(Callback, Result);
	};

	services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<UriResultCallback, UriResult, void, services::NullDto>(InternalCallback,
																							   nullptr,
																							   web::EResponseCodes::ResponseOK);

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
	DownloadAssetDataEx(Asset, CancellationToken::Dummy(), Callback);
}

void AssetSystem::DownloadAssetDataEx(const Asset& Asset, CancellationToken& CancellationToken, AssetDataResultCallback Callback)
{
	services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<AssetDataResultCallback, AssetDataResult, void, services::AssetFileDto>(Callback, nullptr);

	FileManager->GetFile(Asset.Uri, ResponseHandler, CancellationToken);
}

void AssetSystem::GetAssetDataSize(const Asset& Asset, UInt64ResultCallback Callback)
{
	HTTPHeadersResultCallback InternalCallback = [Callback](const HTTPHeadersResult& Result)
	{
		UInt64Result InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

		if (Result.GetResultCode() == EResultCode::Success)
		{
			auto& Headers		= Result.GetValue();
			auto& ContentLength = Headers["content-length"];
			auto Value			= std::strtoull(ContentLength.c_str(), nullptr, 10);
			InternalResult.SetValue(Value);
		}

		INVOKE_IF_NOT_NULL(Callback, InternalResult);
	};

	services::ResponseHandlerPtr ResponseHandler
		= AssetDetailAPI->CreateHandler<HTTPHeadersResultCallback, HTTPHeadersResult, void, services::NullDto>(InternalCallback, nullptr);

	FileManager->GetResponseHeaders(Asset.Uri, ResponseHandler);
}

CSP_ASYNC_RESULT void AssetSystem::GetLODChain(const AssetCollection& AssetCollection, LODChainResultCallback Callback)
{
	auto GetAssetsCallback = [AssetCollection, Callback](const AssetsResult& Result)
	{
		LODChainResult LODResult(Result.GetResultCode(), Result.GetHttpResultCode());

		if (Result.GetResultCode() == EResultCode::Success)
		{
			LODChain Chain = CreateLODChainFromAssets(Result.GetAssets(), AssetCollection.Id);
			LODResult.SetLODChain(std::move(Chain));
		}

		INVOKE_IF_NOT_NULL(Callback, LODResult);
	};

	GetAssetsByCriteria({AssetCollection.Id}, nullptr, nullptr, Array<EAssetType> {EAssetType::MODEL}, GetAssetsCallback);
}

CSP_ASYNC_RESULT_WITH_PROGRESS void
	AssetSystem::RegisterAssetToLODChain(const AssetCollection& AssetCollection, const Asset& InAsset, int LODLevel, AssetResultCallback Callback)
{
	// GetAssetsByCriteria
	auto GetAssetsCallback = [this, AssetCollection, InAsset, LODLevel, Callback](const AssetsResult& Result)
	{
		if (Result.GetResultCode() == EResultCode::InProgress)
		{
			return;
		}

		if (Result.GetResultCode() == EResultCode::Failed)
		{
			INVOKE_IF_NOT_NULL(Callback, Result);

			return;
		}

		const Array<Asset>& Assets = Result.GetAssets();
		LODChain Chain			   = CreateLODChainFromAssets(Assets, AssetCollection.Id);

		if (!ValidateNewLODLevelForChain(Chain, LODLevel))
		{
			CSP_LOG_MSG(LogLevel::Error, "LOD level already exists in chain");

			INVOKE_IF_NOT_NULL(Callback, Result);

			return;
		}

		// UpdateAsset
		auto UpdateAssetCallback = [this, AssetCollection, Callback, Assets](const AssetResult& Result)
		{
			INVOKE_IF_NOT_NULL(Callback, Result);
		};

		// Add new LOD style
		Asset NewAsset = InAsset;
		Array<String> NewStyles(NewAsset.Styles.Size() + 1);

		for (int i = 0; i < NewAsset.Styles.Size(); ++i)
		{
			NewStyles[i] = NewAsset.Styles[i];
		}

		NewStyles[NewAsset.Styles.Size()] = CreateLODStyleVar(LODLevel);
		NewAsset.Styles					  = std::move(NewStyles);

		UpdateAsset(NewAsset, UpdateAssetCallback);
	};

	GetAssetsByCriteria({InAsset.AssetCollectionId}, nullptr, nullptr, Array<EAssetType> {EAssetType::MODEL}, GetAssetsCallback);
}

} // namespace csp::systems
