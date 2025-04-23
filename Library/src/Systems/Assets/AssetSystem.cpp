/*
 * Copyright 2025 Magnopus LLC

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

#include <async++.h>

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

constexpr const char* MATERIAL_SHADERTYPE_METADATA_KEY = "ShaderType";

String ConvertAssetCollectionTypeToString(systems::EAssetCollectionType AssetCollectionType)
{
    switch (AssetCollectionType)
    {
    case systems::EAssetCollectionType::DEFAULT:
        return "Default";
    case systems::EAssetCollectionType::FOUNDATION_INTERNAL:
        return "FoundationInternal";
    case systems::EAssetCollectionType::COMMENT_CONTAINER:
        return "CommentContainer";
    case systems::EAssetCollectionType::COMMENT:
        return "Comment";
    case systems::EAssetCollectionType::SPACE_THUMBNAIL:
        return "SpaceThumbnail";
    default:
    {
        assert(false && "Unsupported AssetCollection Type!");
        return "Default";
    }
    }
}

String ConvertAssetTypeToString(systems::EAssetType AssetType)
{
    switch (AssetType)
    {
    case systems::EAssetType::IMAGE:
        return "Image";
    case systems::EAssetType::THUMBNAIL:
        return "Thumbnail";
    case systems::EAssetType::SIMULATION:
        return "Simulation";
    case systems::EAssetType::MODEL:
        return "Model";
    case systems::EAssetType::VIDEO:
        return "Video";
    case systems::EAssetType::SCRIPT_LIBRARY:
        return "ScriptLibrary";
    case systems::EAssetType::HOLOCAP_VIDEO:
        return "HolocapVideo";
    case systems::EAssetType::HOLOCAP_AUDIO:
        return "HolocapAudio";
    case systems::EAssetType::AUDIO:
        return "Audio";
    case systems::EAssetType::GAUSSIAN_SPLAT:
        return "GaussianSplat";
    case systems::EAssetType::MATERIAL:
        return "Material";
    case systems::EAssetType::ANNOTATION:
        return "Annotation";
    case systems::EAssetType::ANNOTATION_THUMBNAIL:
        return "AnnotationThumbnail";
    default:
        assert(false && "Unsupported Asset Type!");
        return "Image";
    }
}

std::optional<String> ConvertShaderTypeToString(systems::EShaderType ShaderType)
{
    std::optional<String> Result = {};

    switch (ShaderType)
    {
    case systems::EShaderType::Standard:
    {
        Result = "Standard";
        break;
    }
    case systems::EShaderType::AlphaVideo:
    {
        Result = "AlphaVideo";
        break;
    }
    default:
    {
        // If the shader type is nonsense
        Result = {};
        break;
    }
    }

    return Result;
}

std::optional<systems::EShaderType> ConvertStringToShaderType(const csp::common::String& ShaderType)
{
    std::optional<systems::EShaderType> Result = {};

    if (ShaderType == "AlphaVideo")
    {
        Result = systems::EShaderType::AlphaVideo;
    }
    else if (ShaderType == "Standard")
    {
        Result = systems::EShaderType::Standard;
    }
    else
    {
        // If there's nonsense data in the shader type.
        Result = {};
    }

    return Result;
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

        for (size_t idx = 0; idx < Keys->Size(); ++idx)
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

Material* InstantiateMaterialOfType(csp::systems::EShaderType ShaderType, const csp::common::String& Name,
    const csp::common::String& AssetCollectionId, const csp::common::String& AssetId)
{
    Material* NewMaterial = nullptr;

    switch (ShaderType)
    {
    case EShaderType::Standard:
    {
        GLTFMaterial* NewGLTFMaterial = CSP_NEW GLTFMaterial(Name, AssetCollectionId, AssetId);
        NewMaterial = static_cast<Material*>(NewGLTFMaterial);
        break;
    }
    case EShaderType::AlphaVideo:
    {
        AlphaVideoMaterial* NewAlphaVideoMaterial = CSP_NEW AlphaVideoMaterial(Name, AssetCollectionId, AssetId);
        NewMaterial = static_cast<Material*>(NewAlphaVideoMaterial);
        break;
    }
    default:
    {
        assert(false && "Unable to instantiate material. Unsupported Shader Type.");
        break;
    }
    }

    return NewMaterial;
}

std::optional<Material*> DeserializeIntoMaterialOfType(
    const char* MaterialData, csp::systems::EShaderType ShaderType, Material* MaterialToDeserialize)
{
    std::optional<Material*> Result = {};

    switch (ShaderType)
    {
    case csp::systems::EShaderType::Standard:
    {
        GLTFMaterial* NewGLTFMaterial = static_cast<GLTFMaterial*>(MaterialToDeserialize);
        const bool Success = csp::json::JsonDeserializer::Deserialize(MaterialData, *NewGLTFMaterial);
        if (Success)
        {
            Result = MaterialToDeserialize;
        }
        break;
    }
    case csp::systems::EShaderType::AlphaVideo:
    {
        AlphaVideoMaterial* NewAlphaVideoMaterial = static_cast<AlphaVideoMaterial*>(MaterialToDeserialize);
        const bool Success = csp::json::JsonDeserializer::Deserialize(MaterialData, *NewAlphaVideoMaterial);
        if (Success)
        {
            Result = MaterialToDeserialize;
        }
        break;
    }
    default:
    {
        assert(false && "Unable to deserialize material. Unsupported Shader Type.");
        break;
    }
    }

    return Result;
}

void SerializeMaterialOfType(EShaderType ShaderType, const Material* Material, csp::common::String& OutMaterialJson)
{
    switch (ShaderType)
    {
    case csp::systems::EShaderType::Standard:
    {
        const GLTFMaterial* NewGLTFMaterial = static_cast<const GLTFMaterial*>(Material);
        OutMaterialJson = json::JsonSerializer::Serialize(*NewGLTFMaterial);
        break;
    }
    case csp::systems::EShaderType::AlphaVideo:
    {
        const AlphaVideoMaterial* NewAlphaVideoMaterial = static_cast<const AlphaVideoMaterial*>(Material);
        OutMaterialJson = json::JsonSerializer::Serialize(*NewAlphaVideoMaterial);
        break;
    }
    default:
    {
        assert(false && "Unable to serialize material. Unsupported Shader Type.");
        break;
    }
    }
}

std::optional<EShaderType> GetShaderTypeFromMaterialCollection(const csp::systems::AssetCollection& AssetCollection)
{
    std::optional<EShaderType> Result = {};

    const auto Metadata = AssetCollection.GetMetadataImmutable();
    if (Metadata.HasKey(MATERIAL_SHADERTYPE_METADATA_KEY))
    {
        Result = ConvertStringToShaderType(Metadata.operator[](MATERIAL_SHADERTYPE_METADATA_KEY));
    }
    else
    {
        // Please note: Older Spaces built when only GLTF materials were supported will not have stored
        // shader type metadata in the Material Asset Collection. We therefore default to a standard shader type.
        CSP_LOG_MSG(LogLevel::Verbose, "Shader type metadata missing from Material Asset Collection, defaulting to Standard shader type.");
        Result = EShaderType::Standard;
    }

    return Result;
}

std::optional<EShaderType> GetShaderTypeFromMaterialCollectionArray(
    std::shared_ptr<csp::common::Array<csp::systems::AssetCollection>> AssetCollections, const csp::common::String& AssetCollectionId)
{
    for (size_t i = 0; i < (*AssetCollections).Size(); ++i)
    {
        if ((*AssetCollections)[i].Id == AssetCollectionId)
        {
            return GetShaderTypeFromMaterialCollection((*AssetCollections)[i]);
        }
    }

    CSP_LOG_ERROR_MSG("A Material Collection with the specified Id was not found.");
    return {};
}

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

async::task<AssetCollectionResult> AssetSystem::CreateAssetCollection(const csp::common::Optional<csp::common::String>& InSpaceId,
    const csp::common::Optional<csp::common::String>& ParentAssetCollectionId, const csp::common::String& AssetCollectionName,
    const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& Metadata, const EAssetCollectionType Type,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags)
{
    async::event_task<AssetCollectionResult> OnCompleteEvent;
    async::task<AssetCollectionResult> OnCompleteTask = OnCompleteEvent.get_task();

    Optional<String> SpaceId;

    if (InSpaceId.HasValue())
    {
        SpaceId = *InSpaceId;
    }

    const auto PrototypeInfo = CreatePrototypeDto(SpaceId, ParentAssetCollectionId, AssetCollectionName, Metadata, Type, Tags);

    const services::ResponseHandlerPtr ResponseHandler
        = PrototypeAPI->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(
            [](const AssetCollectionResult&) {}, nullptr, web::EResponseCodes::ResponseCreated, std::move(OnCompleteEvent));

    static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesPost(PrototypeInfo, ResponseHandler);

    return OnCompleteTask;
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

    services::ResponseHandlerPtr ResponseHandler = PrototypeAPI->CreateHandler<NullResultCallback, NullResult, void, services::NullDto>(
        Callback, nullptr, web::EResponseCodes::ResponseNoContent);

    static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesIdDelete(PrototypeId, ResponseHandler);
}

async::task<NullResult> AssetSystem::DeleteAssetCollection(const AssetCollection& AssetCollection)
{
    async::event_task<NullResult> OnCompleteEvent;
    async::task<NullResult> OnCompleteTask = OnCompleteEvent.get_task();

    const String PrototypeId = AssetCollection.Id;

    if (PrototypeId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("A delete of an asset collection was issued without an ID. You have to provide an asset collection ID.");
        OnCompleteEvent.set_exception(std::make_exception_ptr(std::exception()));

        return OnCompleteTask;
    }

    services::ResponseHandlerPtr ResponseHandler = PrototypeAPI->CreateHandler<NullResultCallback, NullResult, void, services::NullDto>(
        [](const NullResult& s) {}, nullptr, web::EResponseCodes::ResponseNoContent, std::move(OnCompleteEvent));

    static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesIdDelete(PrototypeId, ResponseHandler);

    return OnCompleteTask;
}

void AssetSystem::DeleteMultipleAssetCollections(csp::common::Array<AssetCollection>& SourceAssetCollectionIDs, NullResultCallback Callback)
{
    if (SourceAssetCollectionIDs.Size() == 0)
    {
        CSP_LOG_MSG(LogLevel::Error, "No source asset collections were provided whilst attempting to delete.");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());
        return;
    }

    std::vector<csp::common::String> AssetCollectionIds;

    for (size_t i = 0; i < SourceAssetCollectionIDs.Size(); ++i)
    {
        AssetCollectionIds.emplace_back(SourceAssetCollectionIDs[i].Id);
    }

    if (AssetCollectionIds.size() == 0)
    {
        CSP_LOG_MSG(LogLevel::Error, "No asset collections could be converted to to required format to delete.");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());
        return;
    }

    csp::services::ResponseHandlerPtr ResponseHandler
        = PrototypeAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::DtoArray<chs::PrototypeDto>>(Callback, nullptr);

    static_cast<chs::PrototypeApi*>(PrototypeAPI)
        ->apiV1PrototypesDelete(AssetCollectionIds, ResponseHandler, csp::common::CancellationToken::Dummy());
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
            std::nullopt, // OrganizationIds (no longer used)
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

async::task<AssetCollectionResult> AssetSystem::GetAssetCollectionById(const csp::common::String& AssetCollectionId)
{
    async::event_task<AssetCollectionResult> OnCompleteEvent;
    async::task<AssetCollectionResult> OnCompleteTask = OnCompleteEvent.get_task();

    services::ResponseHandlerPtr ResponseHandler
        = PrototypeAPI->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(
            [](const AssetCollectionResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(OnCompleteEvent));

    static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesIdGet(AssetCollectionId, ResponseHandler);

    return OnCompleteTask;
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

    StringVec PrototypeIds = Convert(Ids);
    std::optional<String> ParentPrototypeId = Convert(ParentId);
    StringVec PrototypeNames = Convert(Names);

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

    StringVec PrototypeTags = Convert(Tags);
    StringVec GroupIds = Convert(SpaceIds);

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
            std::nullopt, // OrganizationIds (no longer used)
            Skip, // Skip
            Limit, // Limit
            std::nullopt, // SortBy
            std::nullopt, // SortDirection
            ResponseHandler);
}

async::task<AssetCollectionsResult> AssetSystem::FindAssetCollections(const csp::common::Optional<csp::common::Array<csp::common::String>>& Ids,
    const csp::common::Optional<csp::common::String>& ParentId, const csp::common::Optional<csp::common::Array<csp::common::String>>& Names,
    const csp::common::Optional<csp::common::Array<EAssetCollectionType>>& Types,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& SpaceIds, const csp::common::Optional<int>& ResultsSkipNumber,
    const csp::common::Optional<int>& ResultsMaxNumber)
{
    async::event_task<AssetCollectionsResult> OnCompleteEvent;
    async::task<AssetCollectionsResult> OnCompleteTask = OnCompleteEvent.get_task();

    typedef std::optional<std::vector<String>> StringVec;

    StringVec PrototypeIds = Convert(Ids);
    std::optional<String> ParentPrototypeId = Convert(ParentId);
    StringVec PrototypeNames = Convert(Names);

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

    StringVec PrototypeTags = Convert(Tags);
    StringVec GroupIds = Convert(SpaceIds);

    int32_t Skip = ResultsSkipNumber.HasValue() ? *ResultsSkipNumber : DEFAULT_SKIP_NUMBER;
    int32_t Limit = ResultsMaxNumber.HasValue() ? *ResultsMaxNumber : DEFAULT_RESULT_MAX_NUMBER;

    services::ResponseHandlerPtr ResponseHandler
        = PrototypeAPI->CreateHandler<AssetCollectionsResultCallback, AssetCollectionsResult, void, services::DtoArray<chs::PrototypeDto>>(
            [](const AssetCollectionsResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(OnCompleteEvent));

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
            std::nullopt, // OrganizationIds (no longer used)
            Skip, // Skip
            Limit, // Limit
            std::nullopt, // SortBy
            std::nullopt, // SortDirection
            ResponseHandler);

    return OnCompleteTask;
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

async::task<AssetCollectionResult> AssetSystem::UpdateAssetCollectionMetadata(const AssetCollection& AssetCollection,
    const csp::common::Map<csp::common::String, csp::common::String>& NewMetadata,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags)
{
    async::event_task<AssetCollectionResult> OnCompleteEvent;
    async::task<AssetCollectionResult> OnCompleteTask = OnCompleteEvent.get_task();

    auto PrototypeInfo = CreatePrototypeDto(AssetCollection.SpaceId, AssetCollection.ParentId, AssetCollection.Name, NewMetadata,
        AssetCollection.Type, Tags.HasValue() ? Tags : AssetCollection.Tags);

    services::ResponseHandlerPtr ResponseHandler
        = PrototypeAPI->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(
            [](const AssetCollectionResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(OnCompleteEvent));

    static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesIdPut(AssetCollection.Id, PrototypeInfo, ResponseHandler);

    return OnCompleteTask;
}

void AssetSystem::GetAssetCollectionCount(const csp::common::Optional<csp::common::Array<csp::common::String>>& Ids,
    const csp::common::Optional<csp::common::String>& ParentId, const csp::common::Optional<csp::common::Array<csp::common::String>>& Names,
    const csp::common::Optional<csp::common::Array<EAssetCollectionType>>& Types,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& SpaceIds, csp::systems::AssetCollectionCountResultCallback Callback)
{
    std::optional<std::vector<String>> PrototypeIds = Convert(Ids);
    std::optional<String> ParentPrototypeId = Convert(ParentId);
    std::optional<std::vector<String>> PrototypeNames = Convert(Names);
    std::optional<std::vector<String>> PrototypeTypes;

    if (Types.HasValue())
    {
        std::vector<String> Vals;

        for (size_t i = 0; i < Types->Size(); ++i)
        {
            Vals.push_back(ConvertAssetCollectionTypeToString(Types->operator[](i)));
        }

        PrototypeTypes = std::move(Vals);
    }

    std::optional<std::vector<String>> PrototypeTags = Convert(Tags);
    std::optional<std::vector<String>> GroupIds = Convert(SpaceIds);

    services::ResponseHandlerPtr ResponseHandler = PrototypeAPI->CreateHandler<csp::systems::AssetCollectionCountResultCallback,
        csp::systems::AssetCollectionCountResult, void, services::DtoArray<chs::PrototypeDto>>(Callback, nullptr);

    static_cast<chs::PrototypeApi*>(PrototypeAPI)
        ->apiV1PrototypesCountGet(PrototypeTags, // Tags
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
            ResponseHandler);
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

async::task<AssetResult> AssetSystem::CreateAsset(const AssetCollection& AssetCollection, const csp::common::String& Name,
    const csp::common::Optional<csp::common::String>& ThirdPartyPackagedAssetIdentifier,
    const csp::common::Optional<csp::systems::EThirdPartyPlatform>& ThirdPartyPlatform, EAssetType Type)
{
    async::event_task<AssetResult> OnCompleteEvent;
    async::task<AssetResult> OnCompleteTask = OnCompleteEvent.get_task();

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
        [](const AssetResult&) {}, nullptr, web::EResponseCodes::ResponseCreated, std::move(OnCompleteEvent));

    static_cast<chs::AssetDetailApi*>(AssetDetailAPI)->apiV1PrototypesPrototypeIdAssetDetailsPost(AssetCollection.Id, AssetInfo, ResponseHandler);

    return OnCompleteTask;
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

async::task<NullResult> AssetSystem::DeleteAsset(const AssetCollection& AssetCollection, const Asset& Asset)
{
    std::shared_ptr<async::event_task<NullResult>> OnCompleteEvent = std::make_shared<async::event_task<NullResult>>();
    async::task<NullResult> OnCompleteTask = OnCompleteEvent->get_task();

    DeleteAssetById(AssetCollection.Id, Asset.Id,
        [OnCompleteEvent](const NullResult& Result)
        {
            if (Result.GetResultCode() == EResultCode::Success || Result.GetResultCode() == EResultCode::Failed)
            {
                OnCompleteEvent->set(Result);
            }
        });

    return OnCompleteTask;
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

async::task<AssetsResult> AssetSystem::GetAssetsByCriteria(const csp::common::Array<csp::common::String>& AssetCollectionIds,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& AssetIds,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& AssetNames,
    const csp::common::Optional<csp::common::Array<EAssetType>>& AssetTypes)
{
    async::event_task<AssetsResult> OnCompleteEvent;
    async::task<AssetsResult> OnCompleteTask = OnCompleteEvent.get_task();

    if (AssetCollectionIds.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("You have to provide at least one AssetCollectionId");
        OnCompleteEvent.set_exception(std::make_exception_ptr(std::exception()));

        return OnCompleteTask;
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
        = AssetDetailAPI->CreateHandler<AssetsResultCallback, AssetsResult, void, services::DtoArray<chs::AssetDetailDto>>(
            [](const AssetsResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(OnCompleteEvent));

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

    return OnCompleteTask;
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

    for (size_t i = 0; i < AssetCollectionIds.Size(); ++i)
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

async::task<UriResult> AssetSystem::UploadAssetDataEx(const AssetCollection& AssetCollection, const Asset& Asset,
    const AssetDataSource& AssetDataSource, csp::common::CancellationToken& CancellationToken)
{
    async::event_task<UriResult> OnCompleteEvent;
    async::task<UriResult> OnCompleteTask = OnCompleteEvent.get_task();

    if (Asset.Name.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("Asset name cannot be empty");
        OnCompleteEvent.set_exception(std::make_exception_ptr(std::exception()));

        return OnCompleteTask;
    }

    auto FormFile = std::make_shared<web::HttpPayload>();
    AssetDataSource.SetUploadContent(WebClient, FormFile.get(), Asset);

    services::ResponseHandlerPtr ResponseHandler = AssetDetailAPI->CreateHandler<UriResultCallback, UriResult, void, services::NullDto>(
        [](const UriResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(OnCompleteEvent));

    static_cast<chs::AssetDetailApi*>(AssetDetailAPI)
        ->apiV1PrototypesPrototypeIdAssetDetailsAssetDetailIdBlobPost(
            AssetCollection.Id, Asset.Id, std::nullopt, FormFile, ResponseHandler, CancellationToken);

    return OnCompleteTask;
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

        for (size_t i = 0; i < NewAsset.Styles.Size(); ++i)
        {
            NewStyles[i] = NewAsset.Styles[i];
        }

        NewStyles[NewAsset.Styles.Size()] = CreateLODStyleVar(LODLevel);
        NewAsset.Styles = std::move(NewStyles);

        UpdateAsset(NewAsset, UpdateAssetCallback);
    };

    GetAssetsByCriteria({ InAsset.AssetCollectionId }, nullptr, nullptr, Array<EAssetType> { EAssetType::MODEL }, GetAssetsCallback);
}

void AssetSystem::CreateMaterial(const csp::common::String& Name, const csp::systems::EShaderType ShaderType, const csp::common::String& SpaceId,
    csp::common::Map<csp::common::String, csp::common::String>& Metadata, const csp::common::Array<csp::common::String>& AssetTags,
    MaterialResultCallback Callback)
{
    // 1. Create asset collection
    auto CreateAssetCollectionCB = [this, Callback, Name, SpaceId, ShaderType](const AssetCollectionResult& CreateAssetCollectionResult)
    {
        if (CreateAssetCollectionResult.GetResultCode() != EResultCode::Success)
        {
            Callback(MaterialResult(CreateAssetCollectionResult.GetResultCode(), CreateAssetCollectionResult.GetHttpResultCode()));
            return;
        }

        // 2. Create asset
        const AssetCollection& CreatedAssetCollection = CreateAssetCollectionResult.GetAssetCollection();

        auto CreateAssetCB = [this, Callback, CreatedAssetCollection, SpaceId, Name, ShaderType](const AssetResult& CreateAssetResult)
        {
            if (CreateAssetResult.GetResultCode() != EResultCode::Success)
            {
                Callback(MaterialResult(CreateAssetResult.GetResultCode(), CreateAssetResult.GetHttpResultCode()));
                return;
            }

            // 3. Upload default material
            Asset CreatedAsset = CreateAssetResult.GetAsset();
            csp::common::String MaterialJson;

            // Create material of the specific derived type.
            Material* NewlyCreatedMaterial = InstantiateMaterialOfType(ShaderType, Name, CreatedAssetCollection.Id, CreatedAsset.Id);

            // Serialse material data.
            SerializeMaterialOfType(ShaderType, NewlyCreatedMaterial, MaterialJson);

            auto UploadMaterialCallback = [this, Callback, NewlyCreatedMaterial, SpaceId, Name](const UriResult& UploadResult)
            {
                if (UploadResult.GetResultCode() == EResultCode::InProgress)
                {
                    return;
                }

                // 4. Return created material
                MaterialResult FinalResult(UploadResult.GetResultCode(), UploadResult.GetHttpResultCode());
                FinalResult.SetMaterial(NewlyCreatedMaterial);

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

    std::optional<String> ConvertedShaderType = ConvertShaderTypeToString(ShaderType);
    if (ConvertedShaderType.has_value())
    {
        // Set the shader type in the material collection metadata - this is required to deserialize a material to the correct type.
        Metadata[MATERIAL_SHADERTYPE_METADATA_KEY] = ConvertedShaderType.value();
    }
    else
    {
        CSP_LOG_MSG(LogLevel::Error, "Specified shader type invalid.");
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MaterialResult>());
        return;
    }

    CreateAssetCollection(SpaceId, nullptr, MaterialCollectionName, Metadata, EAssetCollectionType::DEFAULT, AssetTags, CreateAssetCollectionCB);
}

void AssetSystem::UpdateMaterial(const Material& Material, NullResultCallback Callback)
{
    // 1. Get asset collection
    auto GetAssetCollectionCB = [this, &Material, Callback](const AssetCollectionResult& CreateAssetCollectionResult)
    {
        if (CreateAssetCollectionResult.GetResultCode() != EResultCode::Success)
        {
            Callback(NullResult(CreateAssetCollectionResult.GetResultCode(), CreateAssetCollectionResult.GetHttpResultCode()));
            return;
        }

        // 2. Get asset
        const AssetCollection& CreatedAssetCollection = CreateAssetCollectionResult.GetAssetCollection();

        auto GetAssetCB = [this, Callback, &Material, CreatedAssetCollection](const AssetResult& CreateAssetResult)
        {
            if (CreateAssetResult.GetResultCode() != EResultCode::Success)
            {
                Callback(NullResult(CreateAssetResult.GetResultCode(), CreateAssetResult.GetHttpResultCode()));
                return;
            }

            // 3. Upload material
            auto UploadMaterialCallback = [this, Callback, &Material](const UriResult& UploadResult)
            { Callback(NullResult(UploadResult.GetResultCode(), UploadResult.GetHttpResultCode())); };

            csp::common::String MaterialJson;

            // Serialse material data.
            SerializeMaterialOfType(Material.GetShaderType(), &Material, MaterialJson);

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

void AssetSystem::DeleteMaterial(const Material& Material, NullResultCallback Callback)
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

void AssetSystem::GetMaterials(const csp::common::String& SpaceId, MaterialsResultCallback Callback)
{
    // 1. find asset collection for space
    auto FindAssetCollectionsCB = [this, Callback](const AssetCollectionsResult& FindAssetCollectionsResult)
    {
        if (FindAssetCollectionsResult.GetResultCode() != EResultCode::Success)
        {
            Callback(MaterialsResult(FindAssetCollectionsResult.GetResultCode(), FindAssetCollectionsResult.GetHttpResultCode()));
            return;
        }

        // Create a shared reference to prevent it going out of scope between callbacks
        auto AssetCollections = std::make_shared<csp::common::Array<csp::systems::AssetCollection>>(FindAssetCollectionsResult.GetAssetCollections());

        if ((*AssetCollections).Size() == 0)
        {
            // There are no asset collections for this space
            Callback(MaterialsResult(FindAssetCollectionsResult.GetResultCode(), FindAssetCollectionsResult.GetHttpResultCode()));
            return;
        }

        // 2. Find material assets in collections
        csp::common::Array<csp::common::String> AssetCollectionIds((*AssetCollections).Size());

        for (size_t i = 0; i < (*AssetCollections).Size(); ++i)
        {
            AssetCollectionIds[i] = (*AssetCollections)[i].Id;
        }

        auto GetAssetsCB = [this, AssetCollections, Callback](const AssetsResult& GetAssetsResult)
        {
            const auto& Assets = GetAssetsResult.GetAssets();
            const size_t AssetsToDownload = Assets.Size();

            if (AssetsToDownload == 0)
            {
                // There are no material assets in this space
                Callback(MaterialsResult(GetAssetsResult.GetResultCode(), GetAssetsResult.GetHttpResultCode()));
                return;
            }

            // These are shared references to prevent going out of scope between callbacks
            // Note: The callbacks ARE called on the main thread
            auto DownloadedMaterials = std::make_shared<csp::common::Array<Material*>>(AssetsToDownload);
            auto AssetsDownloaded = std::make_shared<size_t>();
            auto Failed = std::make_shared<bool>();

            // 3. Download asset data for each material asset
            for (size_t i = 0; i < Assets.Size(); ++i)
            {
                csp::common::String AssetCollectionId = Assets[i].AssetCollectionId;
                csp::common::String AssetId = Assets[i].Id;

                std::optional<EShaderType> ShaderType = GetShaderTypeFromMaterialCollectionArray(AssetCollections, AssetCollectionId);

                if (!ShaderType.has_value())
                {
                    INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MaterialsResult>());
                    return;
                }

                auto DownloadMaterialCallback = [this, Callback, AssetsToDownload, i, AssetCollectionId, AssetId, ShaderType, DownloadedMaterials,
                                                    AssetsDownloaded, Failed](const AssetDataResult& DownloadResult)
                {
                    // Return early as one of the calls has already failed
                    if (*Failed)
                    {
                        return;
                    }

                    if (DownloadResult.GetResultCode() == EResultCode::InProgress)
                    {
                        return;
                    }

                    if (DownloadResult.GetResultCode() == EResultCode::Failed)
                    {
                        *Failed = true;

                        Callback(MaterialsResult(DownloadResult.GetResultCode(), DownloadResult.GetHttpResultCode()));
                        return;
                    }

                    const char* MaterialData = static_cast<const char*>(DownloadResult.GetData());

                    // Create material of the specific derived type.
                    Material* FoundMaterial = InstantiateMaterialOfType(ShaderType.value(), "", AssetCollectionId, AssetId);

                    // Deserialize material data.
                    auto DeserializationResult = DeserializeIntoMaterialOfType(MaterialData, ShaderType.value(), FoundMaterial);

                    if (!DeserializationResult.has_value())
                    {
                        CSP_LOG_ERROR_MSG("Failed to deserialize material");

                        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MaterialsResult>());

                        return;
                    }

                    (*DownloadedMaterials)[i] = DeserializationResult.value();

                    (*AssetsDownloaded)++;

                    if ((*AssetsDownloaded) >= AssetsToDownload)
                    {
                        // Finish
                        MaterialsResult Result(DownloadResult.GetResultCode(), DownloadResult.GetHttpResultCode());
                        Result.SetMaterials(*DownloadedMaterials);

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

void AssetSystem::GetMaterial(const csp::common::String& AssetCollectionId, const csp::common::String& AssetId, MaterialResultCallback Callback)
{
    // 1. Get asset collection
    auto GetAssetCollectionCB = [this, AssetCollectionId, AssetId, Callback](const AssetCollectionResult& CreateAssetCollectionResult)
    {
        if (CreateAssetCollectionResult.GetResultCode() != EResultCode::Success)
        {
            Callback(MaterialResult(CreateAssetCollectionResult.GetResultCode(), CreateAssetCollectionResult.GetHttpResultCode()));
            return;
        }

        // 2. Get asset
        const AssetCollection& FoundAssetCollection = CreateAssetCollectionResult.GetAssetCollection();

        auto GetAssetCB = [this, Callback, FoundAssetCollection](const AssetResult& CreateAssetResult)
        {
            if (CreateAssetResult.GetResultCode() != EResultCode::Success)
            {
                Callback(MaterialResult(CreateAssetResult.GetResultCode(), CreateAssetResult.GetHttpResultCode()));
                return;
            }

            // 3. Download material
            const Asset& FoundAsset = CreateAssetResult.GetAsset();

            auto DownloadMaterialCallback = [this, Callback, FoundAssetCollection, FoundAsset](const AssetDataResult& DownloadResult)
            {
                if (DownloadResult.GetResultCode() != EResultCode::Success)
                {
                    Callback(MaterialResult(DownloadResult.GetResultCode(), DownloadResult.GetHttpResultCode()));
                    return;
                }

                const char* MaterialData = static_cast<const char*>(DownloadResult.GetData());

                std::optional<csp::systems::EShaderType> ShaderType = GetShaderTypeFromMaterialCollection(FoundAssetCollection);
                if (!ShaderType.has_value())
                {
                    INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MaterialResult>());
                    return;
                }

                // Create material of the specific derived type.
                Material* FoundMaterial = InstantiateMaterialOfType(ShaderType.value(), "", FoundAssetCollection.Id, FoundAsset.Id);

                // Deserialse material data.
                auto DeserializationResult = DeserializeIntoMaterialOfType(MaterialData, ShaderType.value(), FoundMaterial);

                if (!DeserializationResult.has_value())
                {
                    CSP_LOG_ERROR_MSG("Failed to deserialize material");

                    INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MaterialResult>());

                    return;
                }

                MaterialResult Result(DownloadResult.GetResultCode(), DownloadResult.GetHttpResultCode());
                Result.SetMaterial(DeserializationResult.value());

                Callback(Result);
            };

            DownloadAssetData(FoundAsset, DownloadMaterialCallback);
        };

        GetAssetById(AssetCollectionId, AssetId, GetAssetCB);
    };

    GetAssetCollectionById(AssetCollectionId, GetAssetCollectionCB);
}

void AssetSystem::SetAssetDetailBlobChangedCallback(AssetDetailBlobChangedCallbackHandler Callback)
{
    AssetDetailBlobChangedCallback = Callback;

    RegisterSystemCallback();
}

void AssetSystem::SetMaterialChangedCallback(MaterialChangedCallbackHandler Callback)
{
    MaterialChangedCallback = Callback;

    RegisterSystemCallback();
}

void AssetSystem::RegisterSystemCallback()
{
    if (!EventBusPtr)
    {
        CSP_LOG_ERROR_MSG("Error: Failed to register AssetSystem. EventBus must be instantiated in the MultiplayerConnection first.");
        return;
    }

    if (!MaterialChangedCallback && !AssetDetailBlobChangedCallback)
    {
        CSP_LOG_ERROR_MSG("Error: Neither MaterialChangedCallback nor AssetDetailBlobChangedCallback were set, not registering AssetSystem to "
                          "AssetDetailBlobChanged.\nPlease set either callback before registering.");
        return;
    }

    EventBusPtr->ListenNetworkEvent("AssetDetailBlobChanged", this);
}

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
