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
#include "Multiplayer/EventSerialisation.h"
#include "Services/PrototypeService/Api.h"
#include "Systems/ResultHelpers.h"
#include "Web/RemoteFileManager.h"

#include "Json/JsonSerializer.h"

// StringFormat needs to be here due to clashing headers
#include "CSP/Common/StringFormat.h"

using namespace csp;
using namespace csp::common;

namespace chs = services::generated::prototypeservice;

namespace
{
constexpr int DEFAULT_SKIP_NUMBER = 0;
constexpr int DEFAULT_RESULT_MAX_NUMBER = 100;

constexpr const char* MATERIAL_ASSET_COLLECTION_NAME_PREFIX = "ASSET_COLLECTION_MATERIAL_";
constexpr const char* MATERIAL_ASSET_NAME_PREFIX = "ASSET_MATERIAL_";
constexpr const char* MATERIAL_FILE_NAME_PREFIX = "ASSET_MATERIAL_FILE_";

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
    else if (AssetType == systems::EAssetType::GAUSSIAN_SPLAT)
        return "GaussianSplat";
    else if (AssetType == systems::EAssetType::MATERIAL)
        return "Material";
    else
    {
        assert(false && "Unsupported Asset Type!");
        return "Image";
    }
}

std::shared_ptr<chs::PrototypeDto> CreatePrototypeDto(const Optional<String>& SpaceId, const Optional<String>& ParentAssetCollectionId,
    const String& AssetCollectionName, const Optional<Map<String, String>>& Metadata, systems::EAssetCollectionType Type,
    const Optional<Array<String>>& Tags)
{
    auto PrototypeInfo = std::make_shared<chs::PrototypeDto>();
    PrototypeInfo->SetName(AssetCollectionName);

    PrototypeInfo->SetType(ConvertAssetCollectionTypeToString(Type));

    if (SpaceId.HasValue())
    {
        const std::vector<String> GroupIds = { *SpaceId };
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
            auto Key = Keys->operator[](idx);
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

csp::common::String CreateUniqueMaterialAssetCollectionName(const csp::common::String& Name, const csp::common::String& SpaceId)
{
    return MATERIAL_ASSET_COLLECTION_NAME_PREFIX + SpaceId + "_" + Name;
}

csp::common::String CreateUniqueMaterialAssetName(const csp::common::String& Name, const csp::common::String& SpaceId)
{
    return MATERIAL_ASSET_NAME_PREFIX + SpaceId + "_" + Name;
}

csp::common::String CreateUniqueMaterialFileName(const csp::common::String& Name, const csp::common::String& SpaceId)
{
    return MATERIAL_FILE_NAME_PREFIX + SpaceId + "_" + Name + ".json";
}

} // namespace

namespace csp::systems
{

AssetSystem::AssetSystem()
    : SystemBase(nullptr, nullptr)
    , PrototypeAPI(nullptr)
    , AssetDetailAPI(nullptr)
    , FileManager(nullptr)
{
}

AssetSystem::AssetSystem(web::WebClient* InWebClient, multiplayer::EventBus* InEventBus)
    : SystemBase(InWebClient, InEventBus)
{
    PrototypeAPI = CSP_NEW chs::PrototypeApi(InWebClient);
    AssetDetailAPI = CSP_NEW chs::AssetDetailApi(InWebClient);

    FileManager = CSP_NEW web::RemoteFileManager(InWebClient);

    RegisterSystemCallback();
}

AssetSystem::~AssetSystem()
{
    CSP_DELETE(FileManager);

    CSP_DELETE(AssetDetailAPI);
    CSP_DELETE(PrototypeAPI);

    DeregisterSystemCallback();
}

void AssetSystem::DeleteAssetCollectionById(const csp::common::String& AssetCollectionId, NullResultCallback Callback)
{
    const String PrototypeId = AssetCollectionId;

    if (PrototypeId.IsEmpty())
    {
        CSP_LOG_MSG(LogLevel::Error, "A delete of an asset collection was issued without an ID. You have to provide an asset collection ID.");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

        return;
    }

    services::ResponseHandlerPtr ResponseHandler = PrototypeAPI->CreateHandler<NullResultCallback, NullResult, void, services::NullDto>(
        Callback, nullptr, web::EResponseCodes::ResponseNoContent);

    static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesIdDelete(PrototypeId, ResponseHandler);
}

void AssetSystem::DeleteAssetById(const csp::common::String& AsseCollectiontId, const csp::common::String& AssetId, NullResultCallback Callback)
{
    services::ResponseHandlerPtr ResponseHandler = AssetDetailAPI->CreateHandler<NullResultCallback, NullResult, void, services::NullDto>(
        Callback, nullptr, web::EResponseCodes::ResponseNoContent);

    static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
        ->apiV1PrototypesPrototypeIdAssetDetailsAssetDetailIdDelete(AsseCollectiontId, AssetId, ResponseHandler);
}

void AssetSystem::CreateAssetCollection(const Optional<String>& InSpaceId, const Optional<String>& ParentAssetCollectionId,
    const String& AssetCollectionName, const Optional<Map<String, String>>& Metadata, EAssetCollectionType Type, const Optional<Array<String>>& Tags,
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
            Callback, nullptr, web::EResponseCodes::ResponseCreated);

    static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesPost(PrototypeInfo, ResponseHandler);
}

void AssetSystem::DeleteAssetCollection(const AssetCollection& AssetCollection, NullResultCallback Callback)
{
    DeleteAssetCollectionById(AssetCollection.Id, Callback);
}

void AssetSystem::CopyAssetCollectionsToSpace(csp::common::Array<AssetCollection>& SourceAssetCollections, const csp::common::String& DestSpaceId,
    bool CopyAsync, AssetCollectionsResultCallback Callback)
{
    if (SourceAssetCollections.Size() == 0)
    {
        CSP_LOG_MSG(LogLevel::Error, "No source asset collections were provided whilst attempting to perform a copy to another space.");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<AssetCollectionsResult>());
        return;
    }

    csp::common::String SourceSpaceId = SourceAssetCollections[0].SpaceId;
    std::vector<csp::common::String> AssetCollectionIds = { SourceAssetCollections[0].Id };

    bool AssetCollectionsBelongToSameSpace = true;

    for (size_t i = 1; i < SourceAssetCollections.Size(); ++i)
    {
        AssetCollectionsBelongToSameSpace &= SourceAssetCollections[i].SpaceId == SourceSpaceId;
        AssetCollectionIds.emplace_back(SourceAssetCollections[i].Id);
    }

    // Verify we have a valid space ID to copy from.
    if (SourceSpaceId.IsEmpty())
    {
        CSP_LOG_MSG(LogLevel::Error,
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
            Callback, nullptr);

    // Use `GET /api/v1/prototypes` and only pass asset collection IDs
    static_cast<chs::PrototypeApi*>(PrototypeAPI)
        ->apiV1PrototypesGroupOwnedOriginalGroupIdDuplicateNewGroupIdPost(std::nullopt, // Tags
            std::nullopt, // ExcludedTags
            std::nullopt, // TagsAll
            AssetCollectionIds, // const std::optional<std::vector<utility::string_t>>& Ids
            std::nullopt, // Names
            std::nullopt, // PartialNames
            std::nullopt, // ExcludedIds
            std::nullopt, // PointOfInterestIds
            std::nullopt, // ParentId
            std::nullopt, // GroupIds
            std::nullopt, // Types
            true, // HasGroup
            std::nullopt, // CreatedBy
            std::nullopt, // CreatedAfter
            std::nullopt, // PrototypeOwnerIds
            std::nullopt, // ReadAccessFilters
            std::nullopt, // WriteAccessFilters
            std::nullopt, // OrganizationIds
            SourceSpaceId, // originalGroupId
            DestSpaceId, // newGroupId
            std::nullopt, // shallowCopy
            CopyAsync, // asyncCall
            std::nullopt, // onBehalfOf
            ResponseHandler, // ResponseHandler
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

void AssetSystem::FindAssetCollections(const Optional<Array<String>>& Ids, const Optional<String>& ParentId, const Optional<Array<String>>& Names,
    const Optional<Array<EAssetCollectionType>>& Types, const Optional<Array<String>>& Tags, const Optional<Array<String>>& SpaceIds,
    const Optional<int>& ResultsSkipNumber, const Optional<int>& ResultsMaxNumber, AssetCollectionsResultCallback Callback)
{
    typedef std::optional<std::vector<String>> StringVec;

    StringVec PrototypeIds;

    if (Ids.HasValue())
    {
        std::vector<String> Vals;

        for (size_t i = 0; i < Ids->Size(); ++i)
        {
            Vals.push_back(Ids->operator[](i));
        }

        PrototypeIds = std::move(Vals);
    }

    std::optional<String> ParentPrototypeId;

    if (ParentId.HasValue())
    {
        ParentPrototypeId = *ParentId;
    }

    StringVec PrototypeNames;

    if (Names.HasValue())
    {
        std::vector<String> Vals;

        for (size_t i = 0; i < Names->Size(); ++i)
        {
            Vals.push_back(Names->operator[](i));
        }

        PrototypeNames = std::move(Vals);
    }

    StringVec PrototypeTypes;

    if (Types.HasValue())
    {
        std::vector<String> Vals;

        for (size_t i = 0; i < Types->Size(); ++i)
        {
            Vals.push_back(ConvertAssetCollectionTypeToString(Types->operator[](i)));
        }

        PrototypeTypes = std::move(Vals);
    }

    StringVec PrototypeTags;

    if (Tags.HasValue())
    {
        std::vector<String> Vals;

        for (size_t i = 0; i < Tags->Size(); ++i)
        {
            Vals.push_back(Tags->operator[](i));
        }

        PrototypeTags = std::move(Vals);
    }

    StringVec GroupIds;

    if (SpaceIds.HasValue())
    {
        std::vector<String> Vals;

        for (size_t i = 0; i < SpaceIds->Size(); ++i)
        {
            Vals.push_back(SpaceIds->operator[](i));
        }

        GroupIds = std::move(Vals);
    }

    int32_t Skip = ResultsSkipNumber.HasValue() ? *ResultsSkipNumber : DEFAULT_SKIP_NUMBER;
    int32_t Limit = ResultsMaxNumber.HasValue() ? *ResultsMaxNumber : DEFAULT_RESULT_MAX_NUMBER;

    services::ResponseHandlerPtr ResponseHandler
        = PrototypeAPI->CreateHandler<AssetCollectionsResultCallback, AssetCollectionsResult, void, services::DtoArray<chs::PrototypeDto>>(
            Callback, nullptr);

    static_cast<chs::PrototypeApi*>(PrototypeAPI)
        ->apiV1PrototypesGet(PrototypeTags, // Tags
            std::nullopt, // ExcludedTags
            std::nullopt, // TagsAll
            PrototypeIds, // Ids
            PrototypeNames, // Names
            std::nullopt, // PartialNames
            std::nullopt, // ExcludedIds
            std::nullopt, // PointOfInterestIds
            ParentPrototypeId, // ParentId
            GroupIds, // GroupIds
            PrototypeTypes, // Types
            std::nullopt, // HasGroup
            std::nullopt, // CreatedBy
            std::nullopt, // CreatedAfter
            std::nullopt, // PrototypeOwnerIds
            std::nullopt, // ReadAccessFilters
            std::nullopt, // WriteAccessFilters
            std::nullopt, // OrganizationIds
            Skip, // Skip
            Limit, // Limit
            std::nullopt, // SortBy
            std::nullopt, // SortDirection
            ResponseHandler);
}

void AssetSystem::UpdateAssetCollectionMetadata(const AssetCollection& AssetCollection, const Map<String, String>& NewMetadata,
    const Optional<Array<String>>& Tags, AssetCollectionResultCallback Callback)
{
    auto PrototypeInfo = CreatePrototypeDto(AssetCollection.SpaceId, AssetCollection.ParentId, AssetCollection.Name, NewMetadata,
        AssetCollection.Type, Tags.HasValue() ? Tags : AssetCollection.Tags);

    services::ResponseHandlerPtr ResponseHandler
        = PrototypeAPI->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(Callback, nullptr);

    static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesIdPut(AssetCollection.Id, PrototypeInfo, ResponseHandler);
}

void AssetSystem::CreateAsset(const AssetCollection& AssetCollection, const String& Name, const Optional<String>& ThirdPartyPackagedAssetIdentifier,
    const Optional<EThirdPartyPlatform>& ThirdPartyPlatform, EAssetType Type, AssetResultCallback Callback)
{
    auto AssetInfo = std::make_shared<chs::AssetDetailDto>();
    AssetInfo->SetName(Name);
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

    services::ResponseHandlerPtr ResponseHandler = AssetDetailAPI->CreateHandler<AssetResultCallback, AssetResult, void, chs::AssetDetailDto>(
        Callback, nullptr, web::EResponseCodes::ResponseCreated);

    static_cast<chs::AssetDetailApi*>(AssetDetailAPI)->apiV1PrototypesPrototypeIdAssetDetailsPost(AssetCollection.Id, AssetInfo, ResponseHandler);
}

void AssetSystem::UpdateAsset(const Asset& Asset, AssetResultCallback Callback)
{
    auto AssetInfo = std::make_shared<chs::AssetDetailDto>();
    AssetInfo->SetName(Asset.Name);
    AssetInfo->SetLanguageCode(Asset.LanguageCode);
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
        StringFormat("%s|%d", Asset.ThirdPartyPackagedAssetIdentifier.c_str(), static_cast<int>(Asset.ThirdPartyPlatformType)));

    AssetInfo->SetAssetType(ConvertAssetTypeToString(Asset.Type));
    services::ResponseHandlerPtr ResponseHandler = AssetDetailAPI->CreateHandler<AssetResultCallback, AssetResult, void, chs::AssetDetailDto>(
        Callback, nullptr, web::EResponseCodes::ResponseCreated);
    static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
        ->apiV1PrototypesPrototypeIdAssetDetailsAssetDetailIdPut(Asset.AssetCollectionId, Asset.Id, AssetInfo, ResponseHandler);
}

void AssetSystem::DeleteAsset(const AssetCollection& AssetCollection, const Asset& Asset, NullResultCallback Callback)
{
    DeleteAssetById(AssetCollection.Id, Asset.Id, Callback);
}

void AssetSystem::GetAssetsInCollection(const AssetCollection& AssetCollection, AssetsResultCallback Callback)
{
    std::vector<String> PrototypeIds = { AssetCollection.Id };

    services::ResponseHandlerPtr ResponseHandler
        = AssetDetailAPI->CreateHandler<AssetsResultCallback, AssetsResult, void, services::DtoArray<chs::AssetDetailDto>>(Callback, nullptr);

    static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
        ->apiV1PrototypesAssetDetailsGet(std::nullopt, // Ids
            std::nullopt, // SupportedPlatforms
            std::nullopt, // AssetTypes
            std::nullopt, // Styles
            std::nullopt, // Names
            std::nullopt, // CreatedAfter
            PrototypeIds, // PrototypeIds
            std::nullopt, // PrototypeNames
            std::nullopt, // PrototypeParentNames
            std::nullopt, // Tags
            std::nullopt, // ExcludedTags
            std::nullopt, // TagsAll
            ResponseHandler);
}

void AssetSystem::GetAssetById(const String& AssetCollectionId, const String& AssetId, AssetResultCallback Callback)
{
    services::ResponseHandlerPtr ResponseHandler
        = AssetDetailAPI->CreateHandler<AssetResultCallback, AssetResult, void, chs::AssetDetailDto>(Callback, nullptr);

    static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
        ->apiV1PrototypesPrototypeIdAssetDetailsAssetDetailIdGet(AssetCollectionId, AssetId, ResponseHandler);
}

void AssetSystem::GetAssetsByCriteria(const Array<String>& AssetCollectionIds, const Optional<Array<String>>& AssetIds,
    const Optional<Array<String>>& AssetNames, const Optional<Array<EAssetType>>& AssetTypes, AssetsResultCallback Callback)
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
            AssetDetailIds->push_back({ (*AssetIds)[idx] });
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
        ->apiV1PrototypesAssetDetailsGet(AssetDetailIds, // Ids
            std::nullopt, // SupportedPlatforms
            AssetDetailTypes, // AssetTypes
            std::nullopt, // Styles
            AssetDetailNames, // Names
            std::nullopt, // CreatedAfter
            PrototypeIds, // PrototypeIds
            std::nullopt, // PrototypeNames
            std::nullopt, // PrototypeParentNames
            std::nullopt, // Tags
            std::nullopt, // ExcludedTags
            std::nullopt, // TagsAll
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
        ->apiV1PrototypesAssetDetailsGet(std::nullopt, // Ids
            std::nullopt, // SupportedPlatforms
            std::nullopt, // AssetTypes
            std::nullopt, // Styles
            std::nullopt, // Names
            std::nullopt, // CreatedAfter
            Ids, // PrototypeIds
            std::nullopt, // PrototypeNames
            std::nullopt, // PrototypeParentNames
            std::nullopt, // Tags
            std::nullopt, // ExcludedTags
            std::nullopt, // TagsAll
            ResponseHandler);
}

void AssetSystem::UploadAssetData(
    const AssetCollection& AssetCollection, const Asset& Asset, const AssetDataSource& AssetDataSource, UriResultCallback Callback)
{
    UploadAssetDataEx(AssetCollection, Asset, AssetDataSource, CancellationToken::Dummy(), Callback);
}

void AssetSystem::UploadAssetDataEx(const AssetCollection& AssetCollection, const Asset& Asset, const AssetDataSource& AssetDataSource,
    CancellationToken& CancellationToken, UriResultCallback Callback)
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

    services::ResponseHandlerPtr ResponseHandler = AssetDetailAPI->CreateHandler<UriResultCallback, UriResult, void, services::NullDto>(
        InternalCallback, nullptr, web::EResponseCodes::ResponseOK);

    static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
        ->apiV1PrototypesPrototypeIdAssetDetailsAssetDetailIdBlobPost(
            AssetCollection.Id, Asset.Id, std::nullopt, FormFile, ResponseHandler, CancellationToken);
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
            auto& Headers = Result.GetValue();
            auto& ContentLength = Headers["content-length"];
            auto Value = std::strtoull(ContentLength.c_str(), nullptr, 10);
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

    GetAssetsByCriteria({ AssetCollection.Id }, nullptr, nullptr, Array<EAssetType> { EAssetType::MODEL }, GetAssetsCallback);
}

CSP_ASYNC_RESULT_WITH_PROGRESS void AssetSystem::RegisterAssetToLODChain(
    const AssetCollection& AssetCollection, const Asset& InAsset, int LODLevel, AssetResultCallback Callback)
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
        LODChain Chain = CreateLODChainFromAssets(Assets, AssetCollection.Id);

        if (!ValidateNewLODLevelForChain(Chain, LODLevel))
        {
            CSP_LOG_MSG(LogLevel::Error, "LOD level already exists in chain");

            INVOKE_IF_NOT_NULL(Callback, Result);

            return;
        }

        // UpdateAsset
        auto UpdateAssetCallback = [this, AssetCollection, Callback, Assets](const AssetResult& Result) { INVOKE_IF_NOT_NULL(Callback, Result); };

        // Add new LOD style
        Asset NewAsset = InAsset;
        Array<String> NewStyles(NewAsset.Styles.Size() + 1);

        for (int i = 0; i < NewAsset.Styles.Size(); ++i)
        {
            NewStyles[i] = NewAsset.Styles[i];
        }

        NewStyles[NewAsset.Styles.Size()] = CreateLODStyleVar(LODLevel);
        NewAsset.Styles = std::move(NewStyles);

        UpdateAsset(NewAsset, UpdateAssetCallback);
    };

    GetAssetsByCriteria({ InAsset.AssetCollectionId }, nullptr, nullptr, Array<EAssetType> { EAssetType::MODEL }, GetAssetsCallback);
}

void AssetSystem::CreateMaterial(const csp::common::String& Name, const csp::common::String& SpaceId, GLTFMaterialResultCallback Callback)
{
    // 1. Create asset collection
    auto CreateAssetCollectionCB = [this, Callback, Name, SpaceId](const AssetCollectionResult& CreateAssetCollectionResult)
    {
        if (CreateAssetCollectionResult.GetResultCode() != EResultCode::Success)
        {
            Callback(GLTFMaterialResult(CreateAssetCollectionResult.GetResultCode(), CreateAssetCollectionResult.GetHttpResultCode()));
            return;
        }

        // 2. Create asset
        const AssetCollection& CreatedAssetCollection = CreateAssetCollectionResult.GetAssetCollection();

        auto CreateAssetCB = [this, Callback, CreatedAssetCollection, SpaceId, Name](const AssetResult& CreateAssetResult)
        {
            if (CreateAssetResult.GetResultCode() != EResultCode::Success)
            {
                Callback(GLTFMaterialResult(CreateAssetResult.GetResultCode(), CreateAssetResult.GetHttpResultCode()));
                return;
            }

            // 3. Upload default material
            Asset CreatedAsset = CreateAssetResult.GetAsset();

            GLTFMaterial NewMaterial(Name, CreatedAssetCollection.Id, CreatedAsset.Id);
            csp::common::String MaterialJson = csp::json::JsonSerializer::Serialize(NewMaterial);

            auto UploadMaterialCallback = [this, Callback, NewMaterial, SpaceId, Name](const UriResult& UploadResult)
            {
                if (UploadResult.GetResultCode() != EResultCode::Success)
                {
                    Callback(GLTFMaterialResult(UploadResult.GetResultCode(), UploadResult.GetHttpResultCode()));
                    return;
                }

                // 4. Return created material
                GLTFMaterialResult FinalResult(UploadResult.GetResultCode(), UploadResult.GetHttpResultCode());
                FinalResult.SetGLTFMaterial(NewMaterial);

                Callback(FinalResult);
            };

            CreatedAsset.FileName = CreateUniqueMaterialFileName(Name, SpaceId);

            // Create a new string to prevent const casting
            std::string Buffer(MaterialJson.c_str());

            BufferAssetDataSource AssetData;
            AssetData.SetMimeType("application/json");
            AssetData.Buffer = Buffer.data();
            AssetData.BufferLength = MaterialJson.Length();

            UploadAssetData(CreatedAssetCollection, CreatedAsset, AssetData, UploadMaterialCallback);
        };

        const csp::common::String MaterialAssetName = CreateUniqueMaterialAssetName(Name, SpaceId);

        CreateAsset(CreateAssetCollectionResult.GetAssetCollection(), MaterialAssetName, nullptr, nullptr, EAssetType::MATERIAL, CreateAssetCB);
    };

    const csp::common::String MaterialCollectionName = CreateUniqueMaterialAssetCollectionName(Name, SpaceId);

    CreateAssetCollection(SpaceId, nullptr, MaterialCollectionName, nullptr, EAssetCollectionType::DEFAULT, nullptr, CreateAssetCollectionCB);
}

void AssetSystem::UpdateMaterial(const GLTFMaterial& Material, NullResultCallback Callback)
{
    // 1. Get asset collection
    auto GetAssetCollectionCB = [this, Material, Callback](const AssetCollectionResult& CreateAssetCollectionResult)
    {
        if (CreateAssetCollectionResult.GetResultCode() != EResultCode::Success)
        {
            Callback(NullResult(CreateAssetCollectionResult.GetResultCode(), CreateAssetCollectionResult.GetHttpResultCode()));
            return;
        }

        // 2. Get asset
        const AssetCollection& CreatedAssetCollection = CreateAssetCollectionResult.GetAssetCollection();

        auto GetAssetCB = [this, Callback, Material, CreatedAssetCollection](const AssetResult& CreateAssetResult)
        {
            if (CreateAssetResult.GetResultCode() != EResultCode::Success)
            {
                Callback(NullResult(CreateAssetResult.GetResultCode(), CreateAssetResult.GetHttpResultCode()));
                return;
            }

            // 3. Upload material
            auto UploadMaterialCallback = [this, Callback, Material](const UriResult& UploadResult)
            { Callback(NullResult(UploadResult.GetResultCode(), UploadResult.GetHttpResultCode())); };

            csp::common::String MaterialJson = json::JsonSerializer::Serialize(Material);
            const Asset& CreatedAsset = CreateAssetResult.GetAsset();

            // Create a new string to prevent const casting
            std::string Buffer(MaterialJson.c_str());

            BufferAssetDataSource AssetData;
            AssetData.SetMimeType("application/json");
            AssetData.Buffer = Buffer.data();
            AssetData.BufferLength = MaterialJson.Length();

            UploadAssetData(CreatedAssetCollection, CreatedAsset, AssetData, UploadMaterialCallback);
        };

        GetAssetById(Material.GetMaterialCollectionId(), Material.GetMaterialId(), GetAssetCB);
    };

    GetAssetCollectionById(Material.GetMaterialCollectionId(), GetAssetCollectionCB);
}

void AssetSystem::DeleteMaterial(const GLTFMaterial& Material, NullResultCallback Callback)
{
    // 1. Delete asset
    auto DeleteAssetCB = [this, Callback, &Material](const NullResult& DeleteAssetResult)
    {
        if (DeleteAssetResult.GetResultCode() != EResultCode::Success)
        {
            Callback(NullResult(DeleteAssetResult.GetResultCode(), DeleteAssetResult.GetHttpResultCode()));
            return;
        }

        // 2. Delete asset collection
        auto DeleteAssetCollectionCB = [this, Callback](const NullResult& DeleteAssetCollectionResult) { Callback(DeleteAssetCollectionResult); };

        DeleteAssetCollectionById(Material.GetMaterialCollectionId(), DeleteAssetCollectionCB);
    };

    DeleteAssetById(Material.GetMaterialCollectionId(), Material.GetMaterialId(), DeleteAssetCB);
}

void AssetSystem::GetMaterials(const csp::common::String& SpaceId, GLTFMaterialsResultCallback Callback)
{
    // 1. find asset collection for space
    auto FindAssetCollectionsCB = [this, Callback](const AssetCollectionsResult& FindAssetCollectionsResult)
    {
        if (FindAssetCollectionsResult.GetResultCode() != EResultCode::Success)
        {
            Callback(GLTFMaterialsResult(FindAssetCollectionsResult.GetResultCode(), FindAssetCollectionsResult.GetHttpResultCode()));
            return;
        }

        const auto& AssetCollections = FindAssetCollectionsResult.GetAssetCollections();

        if (AssetCollections.Size() == 0)
        {
            // There are no asset collections for this space
            Callback(GLTFMaterialsResult(FindAssetCollectionsResult.GetResultCode(), FindAssetCollectionsResult.GetHttpResultCode()));
            return;
        }

        // 2. Find material assets in collections
        csp::common::Array<csp::common::String> AssetCollectionIds(AssetCollections.Size());

        for (size_t i = 0; i < AssetCollections.Size(); ++i)
        {
            AssetCollectionIds[i] = AssetCollections[i].Id;
        }

        auto GetAssetsCB = [this, Callback](const AssetsResult& GetAssetsResult)
        {
            const auto& Assets = GetAssetsResult.GetAssets();
            const size_t AssetsToDownload = Assets.Size();

            if (AssetsToDownload == 0)
            {
                // There are no material assets in this space
                Callback(GLTFMaterialsResult(GetAssetsResult.GetResultCode(), GetAssetsResult.GetHttpResultCode()));
                return;
            }

            // These are shared references to prevent going out of scope between callbacks
            // Note: The callbacks ARE called on the main thread
            auto DownloadedMaterials = std::make_shared<csp::common::Array<GLTFMaterial>>(AssetsToDownload);
            auto AssetsDownloaded = std::make_shared<size_t>();
            auto Failed = std::make_shared<bool>();

            // 3. Download asset data for each material asset
            for (size_t i = 0; i < Assets.Size(); ++i)
            {
                csp::common::String AssetCollectionId = Assets[i].AssetCollectionId;
                csp::common::String AssetId = Assets[i].Id;

                auto DownloadMaterialCallback = [this, Callback, AssetsToDownload, i, AssetCollectionId, AssetId, DownloadedMaterials,
                                                    AssetsDownloaded, Failed](const AssetDataResult& DownloadResult)
                {
                    // Return early as one of the calls has already failed
                    if (*Failed)
                    {
                        return;
                    }

                    if (DownloadResult.GetResultCode() != EResultCode::Success)
                    {
                        if (DownloadResult.GetResultCode() == EResultCode::Failed)
                        {
                            *Failed = true;
                        }

                        Callback(GLTFMaterialsResult(DownloadResult.GetResultCode(), DownloadResult.GetHttpResultCode()));
                        return;
                    }

                    // Convert material json to material
                    const char* MaterialData = static_cast<const char*>(DownloadResult.GetData());

                    GLTFMaterial FoundMaterial("", AssetCollectionId, AssetId);
                    bool Deserialized = json::JsonDeserializer::Deserialize(MaterialData, FoundMaterial);

                    (*AssetsDownloaded)++;

                    if (Deserialized == false)
                    {
                        CSP_LOG_ERROR_MSG("Failed to deserialize material");
                        return;
                    }

                    (*DownloadedMaterials)[i] = FoundMaterial;

                    if ((*AssetsDownloaded) >= AssetsToDownload)
                    {
                        // Finish
                        GLTFMaterialsResult Result(DownloadResult.GetResultCode(), DownloadResult.GetHttpResultCode());
                        Result.SetGLTFMaterials((*DownloadedMaterials));

                        Callback(Result);
                    }
                };

                DownloadAssetData(Assets[i], DownloadMaterialCallback);
            }
        };

        GetAssetsByCriteria(AssetCollectionIds, nullptr, nullptr, csp::common::Array { EAssetType::MATERIAL }, GetAssetsCB);
    };

    FindAssetCollections(
        nullptr, nullptr, nullptr, nullptr, nullptr, csp::common::Array<csp::common::String> { SpaceId }, nullptr, nullptr, FindAssetCollectionsCB);
}

void AssetSystem::GetMaterial(const csp::common::String& AssetCollectionId, const csp::common::String& AssetId, GLTFMaterialResultCallback Callback)
{
    // 1. Get asset collection
    auto GetAssetCollectionCB = [this, AssetCollectionId, AssetId, Callback](const AssetCollectionResult& CreateAssetCollectionResult)
    {
        if (CreateAssetCollectionResult.GetResultCode() != EResultCode::Success)
        {
            Callback(GLTFMaterialResult(CreateAssetCollectionResult.GetResultCode(), CreateAssetCollectionResult.GetHttpResultCode()));
            return;
        }

        // 2. Get asset
        const AssetCollection& FoundAssetCollection = CreateAssetCollectionResult.GetAssetCollection();

        auto GetAssetCB = [this, Callback, FoundAssetCollection](const AssetResult& CreateAssetResult)
        {
            if (CreateAssetResult.GetResultCode() != EResultCode::Success)
            {
                Callback(GLTFMaterialResult(CreateAssetResult.GetResultCode(), CreateAssetResult.GetHttpResultCode()));
                return;
            }

            // 3. Download material
            const Asset& FoundAsset = CreateAssetResult.GetAsset();

            auto DownloadMaterialCallback = [this, Callback, FoundAssetCollection, FoundAsset](const AssetDataResult& DownloadResult)
            {
                if (DownloadResult.GetResultCode() != EResultCode::Success)
                {
                    Callback(GLTFMaterialResult(DownloadResult.GetResultCode(), DownloadResult.GetHttpResultCode()));
                    return;
                }

                const char* MaterialData = static_cast<const char*>(DownloadResult.GetData());

                // Convert material json to material
                GLTFMaterial FoundMaterial("", FoundAssetCollection.Id, FoundAsset.Id);
                bool Deserialized = csp::json::JsonDeserializer::Deserialize(MaterialData, FoundMaterial);

                if (Deserialized == false)
                {
                    CSP_LOG_ERROR_MSG("Failed to deserialize material");
                }

                GLTFMaterialResult Result(DownloadResult.GetResultCode(), DownloadResult.GetHttpResultCode());
                Result.SetGLTFMaterial(FoundMaterial);

                Callback(Result);
            };

            DownloadAssetData(FoundAsset, DownloadMaterialCallback);
        };

        GetAssetById(AssetCollectionId, AssetId, GetAssetCB);
    };

    GetAssetCollectionById(AssetCollectionId, GetAssetCollectionCB);
}

CSP_EVENT void AssetSystem::SetAssetDetailBlobChangedCallback(AssetDetailBlobChangedCallbackHandler Callback)
{
    AssetDetailBlobChangedCallback = Callback;

    // If MaterialChangedCallback hasn't been registered, we need to register
    if (!MaterialChangedCallback)
    {
        RegisterSystemCallback();
    }
}

void AssetSystem::SetMaterialChangedCallback(MaterialChangedCallbackHandler Callback)
{
    MaterialChangedCallback = Callback;

    // If AssetDetailBlobChangedCallback hasn't been registered, we need to register
    if (!AssetDetailBlobChangedCallback)
    {
        RegisterSystemCallback();
    }
}

void AssetSystem::RegisterSystemCallback() { EventBusPtr->ListenNetworkEvent("AssetDetailBlobChanged", this); }

void AssetSystem::DeregisterSystemCallback()
{
    if (EventBusPtr)
    {
        EventBusPtr->StopListenNetworkEvent("AssetDetailBlobChanged");
    }
}

void AssetSystem::OnEvent(const std::vector<signalr::value>& EventValues)
{
    if (!AssetDetailBlobChangedCallback && !MaterialChangedCallback)
    {
        return;
    }

    csp::multiplayer::AssetChangedEventDeserialiser Deserialiser;
    Deserialiser.Parse(EventValues);

    const csp::multiplayer::AssetDetailBlobParams& AssetParams = Deserialiser.GetEventParams();

    if (AssetDetailBlobChangedCallback)
    {
        AssetDetailBlobChangedCallback(AssetParams);
    }

    if (AssetParams.AssetType == systems::EAssetType::MATERIAL && MaterialChangedCallback)
    {
        csp::multiplayer::MaterialChangedParams MaterialParams;
        MaterialParams.ChangeType = AssetParams.ChangeType;
        MaterialParams.MaterialCollectionId = AssetParams.AssetCollectionId;
        MaterialParams.MaterialId = AssetParams.AssetId;

        MaterialChangedCallback(MaterialParams);
    }
}

} // namespace csp::systems
