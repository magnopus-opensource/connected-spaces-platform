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

#include "CSP/Common/CSPAsyncScheduler.h"
#include "CSP/Common/ContinuationUtils.h"
#include "CSP/Common/Interfaces/IAuthContext.h"
#include "CallHelpers.h"
#include "LODHelpers.h"
#include "Multiplayer/NetworkEventSerialisation.h"
#include "Services/PrototypeService/Api.h"
#include "Systems/ResultHelpers.h"
#include "Web/RemoteFileManager.h"

#include "Json/JsonSerializer.h"

// StringFormat needs to be here due to clashing headers
#include "CSP/Common/StringFormat.h"
#include "Common/Convert.h"

#include <fmt/format.h>

#include <algorithm>

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

String ConvertAssetCollectionTypeToString(systems::EAssetCollectionType assetCollectionType)
{
    switch (assetCollectionType)
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

String ConvertAssetTypeToString(systems::EAssetType assetType)
{
    switch (assetType)
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
    case systems::EAssetType::TEXT:
        return "Text";
    default:
        assert(false && "Unsupported Asset Type!");
        return "Image";
    }
}

std::optional<String> ConvertShaderTypeToString(systems::EShaderType shaderType)
{
    std::optional<String> result = {};

    switch (shaderType)
    {
    case systems::EShaderType::Standard:
    {
        result = "Standard";
        break;
    }
    case systems::EShaderType::AlphaVideo:
    {
        result = "AlphaVideo";
        break;
    }
    default:
    {
        // If the shader type is nonsense
        result = {};
        break;
    }
    }

    return result;
}

std::optional<systems::EShaderType> ConvertStringToShaderType(const csp::common::String& shaderType)
{
    std::optional<systems::EShaderType> result = {};

    if (shaderType == "AlphaVideo")
    {
        result = systems::EShaderType::AlphaVideo;
    }
    else if (shaderType == "Standard")
    {
        result = systems::EShaderType::Standard;
    }
    else
    {
        // If there's nonsense data in the shader type.
        result = {};
    }

    return result;
}

std::shared_ptr<chs::PrototypeDto> CreatePrototypeDto(const Optional<String>& spaceId, const Optional<String>& parentAssetCollectionId,
    const String& assetCollectionName, const Optional<Map<String, String>>& metadata, systems::EAssetCollectionType type,
    const Optional<Array<String>>& tags)
{
    auto prototypeInfo = std::make_shared<chs::PrototypeDto>();
    prototypeInfo->SetName(assetCollectionName);

    prototypeInfo->SetType(ConvertAssetCollectionTypeToString(type));

    if (spaceId.HasValue())
    {
        const std::vector<String> groupIds = { *spaceId };
        prototypeInfo->SetGroupIds(groupIds);
    }

    if (parentAssetCollectionId.HasValue())
    {
        prototypeInfo->SetParentId(*parentAssetCollectionId);
    }

    if (metadata.HasValue())
    {
        std::map<String, String> dtoMetadata;

        auto* keys = metadata->Keys();

        for (size_t idx = 0; idx < keys->Size(); ++idx)
        {
            auto key = keys->operator[](idx);
            auto value = metadata->operator[](key);
            dtoMetadata.insert(std::pair<String, String>(key, value));
        }

        prototypeInfo->SetMetadata(dtoMetadata);
    }

    if (tags.HasValue())
    {
        std::vector<String> tagsVector;
        tagsVector.reserve(tags->Size());

        for (size_t idx = 0; idx < tags->Size(); ++idx)
        {
            tagsVector.push_back((*tags)[idx]);
        }

        prototypeInfo->SetTags(tagsVector);
    }

    return prototypeInfo;
}

csp::common::String CreateUniqueMaterialAssetCollectionName(const csp::common::String& name, const csp::common::String& spaceId)
{
    return MATERIAL_ASSET_COLLECTION_NAME_PREFIX + spaceId + "_" + name;
}

csp::common::String CreateUniqueMaterialAssetName(const csp::common::String& name, const csp::common::String& spaceId)
{
    return MATERIAL_ASSET_NAME_PREFIX + spaceId + "_" + name;
}

csp::common::String CreateUniqueMaterialFileName(const csp::common::String& name, const csp::common::String& spaceId)
{
    return MATERIAL_FILE_NAME_PREFIX + spaceId + "_" + name + ".json";
}

} // namespace

namespace csp::systems
{

Material* InstantiateMaterialOfType(csp::systems::EShaderType shaderType, const csp::common::String& name,
    const csp::common::String& assetCollectionId, const csp::common::String& assetId)
{
    Material* newMaterial = nullptr;

    switch (shaderType)
    {
    case EShaderType::Standard:
    {
        GLTFMaterial* newGltfMaterial = new GLTFMaterial(name, assetCollectionId, assetId);
        newMaterial = static_cast<Material*>(newGltfMaterial);
        break;
    }
    case EShaderType::AlphaVideo:
    {
        AlphaVideoMaterial* newAlphaVideoMaterial = new AlphaVideoMaterial(name, assetCollectionId, assetId);
        newMaterial = static_cast<Material*>(newAlphaVideoMaterial);
        break;
    }
    default:
    {
        assert(false && "Unable to instantiate material. Unsupported Shader Type.");
        break;
    }
    }

    return newMaterial;
}

std::optional<Material*> DeserializeIntoMaterialOfType(
    const char* materialData, csp::systems::EShaderType shaderType, Material* materialToDeserialize)
{
    std::optional<Material*> result = {};

    switch (shaderType)
    {
    case csp::systems::EShaderType::Standard:
    {
        GLTFMaterial* newGltfMaterial = static_cast<GLTFMaterial*>(materialToDeserialize);
        const bool success = csp::json::JsonDeserializer::Deserialize(materialData, *newGltfMaterial);
        if (success)
        {
            result = materialToDeserialize;
        }
        break;
    }
    case csp::systems::EShaderType::AlphaVideo:
    {
        AlphaVideoMaterial* newAlphaVideoMaterial = static_cast<AlphaVideoMaterial*>(materialToDeserialize);
        const bool success = csp::json::JsonDeserializer::Deserialize(materialData, *newAlphaVideoMaterial);
        if (success)
        {
            result = materialToDeserialize;
        }
        break;
    }
    default:
    {
        assert(false && "Unable to deserialize material. Unsupported Shader Type.");
        break;
    }
    }

    return result;
}

void SerializeMaterialOfType(EShaderType shaderType, const Material* material, csp::common::String& outMaterialJson)
{
    switch (shaderType)
    {
    case csp::systems::EShaderType::Standard:
    {
        const GLTFMaterial* newGltfMaterial = static_cast<const GLTFMaterial*>(material);
        outMaterialJson = json::JsonSerializer::Serialize(*newGltfMaterial);
        break;
    }
    case csp::systems::EShaderType::AlphaVideo:
    {
        const AlphaVideoMaterial* newAlphaVideoMaterial = static_cast<const AlphaVideoMaterial*>(material);
        outMaterialJson = json::JsonSerializer::Serialize(*newAlphaVideoMaterial);
        break;
    }
    default:
    {
        assert(false && "Unable to serialize material. Unsupported Shader Type.");
        break;
    }
    }
}

std::optional<EShaderType> GetShaderTypeFromMaterialCollection(const csp::systems::AssetCollection& assetCollection)
{
    std::optional<EShaderType> result = {};

    const auto metadata = assetCollection.GetMetadataImmutable();
    if (metadata.HasKey(MATERIAL_SHADERTYPE_METADATA_KEY))
    {
        result = ConvertStringToShaderType(metadata.operator[](MATERIAL_SHADERTYPE_METADATA_KEY));
    }
    else
    {
        // Please note: Older Spaces built when only GLTF materials were supported will not have stored
        // shader type metadata in the Material Asset Collection. We therefore default to a standard shader type.
        CSP_LOG_MSG(LogLevel::Verbose, "Shader type metadata missing from Material Asset Collection, defaulting to Standard shader type.");
        result = EShaderType::Standard;
    }

    return result;
}

AssetSystem::AssetSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , m_prototypeApi(nullptr)
    , m_assetDetailApi(nullptr)
    , m_fileManager(nullptr)
{
}

AssetSystem::AssetSystem(
    web::WebClient* webClient, multiplayer::NetworkEventBus& eventBus, const csp::common::IAuthContext& inAuthContext, common::LogSystem& logSystem)
    : SystemBase(webClient, &eventBus, &logSystem)
{
    m_prototypeApi = new chs::PrototypeApi(webClient);
    m_assetDetailApi = new chs::AssetDetailApi(webClient);

    m_fileManager = new web::RemoteFileManager(webClient, inAuthContext);

    RegisterSystemCallback();
}

AssetSystem::~AssetSystem()
{
    delete (m_fileManager);

    delete (m_assetDetailApi);
    delete (m_prototypeApi);
}

void AssetSystem::DeleteAssetCollectionById(const csp::common::String& assetCollectionId, NullResultCallback callback)
{
    const String prototypeId = assetCollectionId;

    if (prototypeId.IsEmpty())
    {
        CSP_LOG_MSG(LogLevel::Error, "A delete of an asset collection was issued without an ID. You have to provide an asset collection ID.");

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<NullResult>());

        return;
    }

    services::ResponseHandlerPtr responseHandler = m_prototypeApi->CreateHandler<NullResultCallback, NullResult, void, services::NullDto>(
        callback, nullptr, web::EResponseCodes::ResponseNoContent);

    static_cast<chs::PrototypeApi*>(m_prototypeApi)->prototypesIdDelete({ prototypeId }, responseHandler);
}

void AssetSystem::DeleteAssetById(const csp::common::String& asseCollectiontId, const csp::common::String& assetId, NullResultCallback callback)
{
    services::ResponseHandlerPtr responseHandler = m_assetDetailApi->CreateHandler<NullResultCallback, NullResult, void, services::NullDto>(
        callback, nullptr, web::EResponseCodes::ResponseNoContent);

    static_cast<chs::AssetDetailApi*>(m_assetDetailApi)
        ->prototypesPrototypeIdAsset_detailsAssetDetailIdDelete({ asseCollectiontId, assetId }, responseHandler);
}

void AssetSystem::CreateAssetCollection(const Optional<String>& inSpaceId, const Optional<String>& parentAssetCollectionId,
    const String& assetCollectionName, const Optional<Map<String, String>>& metadata, EAssetCollectionType type, const Optional<Array<String>>& tags,
    AssetCollectionResultCallback callback)
{
    Optional<String> spaceId;

    if (inSpaceId.HasValue())
    {
        spaceId = *inSpaceId;
    }

    const auto prototypeInfo = CreatePrototypeDto(spaceId, parentAssetCollectionId, assetCollectionName, metadata, type, tags);

    const services::ResponseHandlerPtr responseHandler
        = m_prototypeApi->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(
            callback, nullptr, web::EResponseCodes::ResponseCreated);

    static_cast<chs::PrototypeApi*>(m_prototypeApi)->prototypesPost({ prototypeInfo }, responseHandler);
}

async::task<AssetCollectionResult> AssetSystem::CreateAssetCollection(const csp::common::Optional<csp::common::String>& inSpaceId,
    const csp::common::Optional<csp::common::String>& parentAssetCollectionId, const csp::common::String& assetCollectionName,
    const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& metadata, const EAssetCollectionType type,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags)
{
    async::event_task<AssetCollectionResult> onCompleteEvent;
    async::task<AssetCollectionResult> onCompleteTask = onCompleteEvent.get_task();

    Optional<String> spaceId;

    if (inSpaceId.HasValue())
    {
        spaceId = *inSpaceId;
    }

    const auto prototypeInfo = CreatePrototypeDto(spaceId, parentAssetCollectionId, assetCollectionName, metadata, type, tags);

    const services::ResponseHandlerPtr responseHandler
        = m_prototypeApi->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(
            [](const AssetCollectionResult&) {}, nullptr, web::EResponseCodes::ResponseCreated, std::move(onCompleteEvent));

    static_cast<chs::PrototypeApi*>(m_prototypeApi)->prototypesPost({ prototypeInfo }, responseHandler);

    return onCompleteTask;
}

void AssetSystem::DeleteAssetCollection(const AssetCollection& assetCollection, NullResultCallback callback)
{
    const String prototypeId = assetCollection.Id;

    if (prototypeId.IsEmpty())
    {
        CSP_LOG_MSG(LogLevel::Error, "A delete of an asset collection was issued without an ID. You have to provide an asset collection ID.");

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<NullResult>());

        return;
    }

    services::ResponseHandlerPtr responseHandler = m_prototypeApi->CreateHandler<NullResultCallback, NullResult, void, services::NullDto>(
        callback, nullptr, web::EResponseCodes::ResponseNoContent);

    static_cast<chs::PrototypeApi*>(m_prototypeApi)->prototypesIdDelete({ prototypeId }, responseHandler);
}

async::task<NullResult> AssetSystem::DeleteAssetCollection(const AssetCollection& assetCollection)
{
    async::event_task<NullResult> onCompleteEvent;
    async::task<NullResult> onCompleteTask = onCompleteEvent.get_task();

    const String prototypeId = assetCollection.Id;

    if (prototypeId.IsEmpty())
    {
        onCompleteEvent.set_exception(std::make_exception_ptr(csp::common::continuations::ResultException(
            "A delete of an asset collection was issued without an ID. You have to provide an asset collection ID.", MakeInvalid<NullResult>())));

        return onCompleteTask;
    }

    services::ResponseHandlerPtr responseHandler = m_prototypeApi->CreateHandler<NullResultCallback, NullResult, void, services::NullDto>(
        [](const NullResult& /*s*/) {}, nullptr, web::EResponseCodes::ResponseNoContent, std::move(onCompleteEvent));

    static_cast<chs::PrototypeApi*>(m_prototypeApi)->prototypesIdDelete({ prototypeId }, responseHandler);

    return onCompleteTask;
}

void AssetSystem::DeleteMultipleAssetCollections(csp::common::Array<AssetCollection>& sourceAssetCollectionIDs, NullResultCallback callback)
{
    if (sourceAssetCollectionIDs.Size() == 0)
    {
        CSP_LOG_MSG(LogLevel::Error, "No source asset collections were provided whilst attempting to delete.");

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<NullResult>());
        return;
    }

    std::vector<csp::common::String> assetCollectionIds;

    for (size_t i = 0; i < sourceAssetCollectionIDs.Size(); ++i)
    {
        assetCollectionIds.emplace_back(sourceAssetCollectionIDs[i].Id);
    }

    if (assetCollectionIds.size() == 0)
    {
        CSP_LOG_MSG(LogLevel::Error, "No asset collections could be converted to to required format to delete.");

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<NullResult>());
        return;
    }

    csp::services::ResponseHandlerPtr responseHandler
        = m_prototypeApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::DtoArray<chs::PrototypeDto>>(callback, nullptr);

    static_cast<chs::PrototypeApi*>(m_prototypeApi)->prototypesDelete({ assetCollectionIds }, responseHandler, csp::common::CancellationToken::Dummy());
}

void AssetSystem::CopyAssetCollectionsToSpace(csp::common::Array<AssetCollection>& sourceAssetCollections, const csp::common::String& destSpaceId,
    bool copyAsync, AssetCollectionsResultCallback callback)
{
    if (sourceAssetCollections.Size() == 0)
    {
        CSP_LOG_MSG(LogLevel::Error, "No source asset collections were provided whilst attempting to perform a copy to another space.");

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<AssetCollectionsResult>());
        return;
    }

    csp::common::String sourceSpaceId = sourceAssetCollections[0].SpaceId;
    std::vector<csp::common::String> assetCollectionIds = { sourceAssetCollections[0].Id };

    bool assetCollectionsBelongToSameSpace = true;

    for (size_t i = 1; i < sourceAssetCollections.Size(); ++i)
    {
        assetCollectionsBelongToSameSpace &= sourceAssetCollections[i].SpaceId == sourceSpaceId;
        assetCollectionIds.emplace_back(sourceAssetCollections[i].Id);
    }

    // Verify we have a valid space ID to copy from.
    if (sourceSpaceId.IsEmpty())
    {
        CSP_LOG_MSG(LogLevel::Error,
            "An asset with no space ID was provided whilst attempting to perform a copy to another space. All assets must have a valid space ID.");

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<AssetCollectionsResult>());
        return;
    }

    // Verify that all source asset collections belong to the same space. If not, this qualifies as an unsupported operation.
    if (!assetCollectionsBelongToSameSpace)
    {
        CSP_LOG_MSG(LogLevel::Error, "All asset collections must belong to the same space for a copy operation.");

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<AssetCollectionsResult>());
        return;
    }

    csp::services::ResponseHandlerPtr responseHandler
        = m_prototypeApi->CreateHandler<AssetCollectionsResultCallback, AssetCollectionsCopyResult, void, chs::CopyPrototypesResult>(callback, nullptr);

    auto prototypeFilters = std::shared_ptr<chs::PrototypeFilters>(new chs::PrototypeFilters);
    prototypeFilters->SetIds(assetCollectionIds);
    prototypeFilters->SetHasGroup(true);

    auto duplicateGroupPrototypesOptions = std::shared_ptr<chs::DuplicateGroupPrototypesOptions>(new chs::DuplicateGroupPrototypesOptions);
    duplicateGroupPrototypesOptions->SetOriginalGroupId(sourceSpaceId);
    duplicateGroupPrototypesOptions->SetNewGroupId(destSpaceId);
    duplicateGroupPrototypesOptions->SetAdditionalFilters(prototypeFilters);
    duplicateGroupPrototypesOptions->SetAsyncCall(copyAsync);
    duplicateGroupPrototypesOptions->SetIncludeMusubiGeneratedAssets(true);

    // Use `GET /api/v1/prototypes` and only pass asset collection IDs
    static_cast<chs::PrototypeApi*>(m_prototypeApi)
        ->prototypesGroup_ownedOriginalGroupIdDuplicateNewGroupIdPost(
            {
                sourceSpaceId, // originalGroupId
                destSpaceId, // newGroupId
                duplicateGroupPrototypesOptions // RequestBody
            },
            responseHandler, // ResponseHandler
            csp::common::CancellationToken::Dummy() // CancellationToken
        );
}

void AssetSystem::GetAssetCollectionById(const String& assetCollectionId, AssetCollectionResultCallback callback)
{
    services::ResponseHandlerPtr responseHandler
        = m_prototypeApi->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(callback, nullptr);

    static_cast<chs::PrototypeApi*>(m_prototypeApi)->prototypesIdGet({ assetCollectionId }, responseHandler);
}

async::task<AssetCollectionResult> AssetSystem::GetAssetCollectionById(const csp::common::String& assetCollectionId)
{
    async::event_task<AssetCollectionResult> onCompleteEvent;
    async::task<AssetCollectionResult> onCompleteTask = onCompleteEvent.get_task();

    services::ResponseHandlerPtr responseHandler
        = m_prototypeApi->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(
            [](const AssetCollectionResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(onCompleteEvent));

    static_cast<chs::PrototypeApi*>(m_prototypeApi)->prototypesIdGet({ assetCollectionId }, responseHandler);

    return onCompleteTask;
}

void AssetSystem::GetAssetCollectionByName(const String& assetCollectionName, AssetCollectionResultCallback callback)
{
    services::ResponseHandlerPtr responseHandler
        = m_prototypeApi->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(callback, nullptr);

    static_cast<chs::PrototypeApi*>(m_prototypeApi)->prototypesNameNameGet({ assetCollectionName }, responseHandler);
}

void AssetSystem::FindAssetCollections(const Optional<Array<String>>& ids, const Optional<String>& parentId, const Optional<Array<String>>& names,
    const Optional<Array<EAssetCollectionType>>& types, const Optional<Array<String>>& tags, const Optional<Array<String>>& spaceIds,
    const Optional<int>& resultsSkipNumber, const Optional<int>& resultsMaxNumber, AssetCollectionsResultCallback callback)
{
    typedef std::optional<std::vector<String>> StringVec;

    StringVec prototypeIds = Convert(ids);
    std::optional<String> parentPrototypeId = Convert(parentId);
    StringVec prototypeNames = Convert(names);

    StringVec prototypeTypes;

    if (types.HasValue())
    {
        std::vector<String> vals;

        for (size_t i = 0; i < types->Size(); ++i)
        {
            vals.push_back(ConvertAssetCollectionTypeToString(types->operator[](i)));
        }

        prototypeTypes = std::move(vals);
    }

    StringVec prototypeTags = Convert(tags);
    StringVec groupIds = Convert(spaceIds);

    int32_t skip = resultsSkipNumber.HasValue() ? *resultsSkipNumber : DEFAULT_SKIP_NUMBER;
    int32_t limit = resultsMaxNumber.HasValue() ? *resultsMaxNumber : DEFAULT_RESULT_MAX_NUMBER;

    services::ResponseHandlerPtr responseHandler
        = m_prototypeApi->CreateHandler<AssetCollectionsResultCallback, AssetCollectionsResult, void, services::DtoArray<chs::PrototypeDto>>(
            callback, nullptr);

    static_cast<chs::PrototypeApi*>(m_prototypeApi)
        ->prototypesGet(
            {
                prototypeTags, // Tags
                std::nullopt, // ExcludedTags
                std::nullopt, // TagsAll
                prototypeIds, // Ids
                prototypeNames, // Names
                std::nullopt, // PartialNames
                std::nullopt, // ExcludedIds
                std::nullopt, // PointOfInterestIds
                parentPrototypeId, // ParentId
                groupIds, // GroupIds
                prototypeTypes, // Types
                std::nullopt, // HasGroup
                std::nullopt, // CreatedBy
                std::nullopt, // CreatedAfter
                std::nullopt, // PrototypeOwnerIds
                std::nullopt, // ReadAccessFilters
                std::nullopt, // WriteAccessFilters
                std::nullopt, // OrganizationIds (no longer used)
                std::nullopt, // ExcludedTypes
                skip, // Skip
                limit, // Limit
                std::nullopt, // SortBy
                std::nullopt // SortDirection
            },
            responseHandler);
}

async::task<AssetCollectionsResult> AssetSystem::FindAssetCollections(const csp::common::Optional<csp::common::Array<csp::common::String>>& ids,
    const csp::common::Optional<csp::common::String>& parentId, const csp::common::Optional<csp::common::Array<csp::common::String>>& names,
    const csp::common::Optional<csp::common::Array<EAssetCollectionType>>& types,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& spaceIds, const csp::common::Optional<int>& resultsSkipNumber,
    const csp::common::Optional<int>& resultsMaxNumber)
{
    async::event_task<AssetCollectionsResult> onCompleteEvent;
    async::task<AssetCollectionsResult> onCompleteTask = onCompleteEvent.get_task();

    typedef std::optional<std::vector<String>> StringVec;

    StringVec prototypeIds = Convert(ids);
    std::optional<String> parentPrototypeId = Convert(parentId);
    StringVec prototypeNames = Convert(names);

    StringVec prototypeTypes;

    if (types.HasValue())
    {
        std::vector<String> vals;

        for (size_t i = 0; i < types->Size(); ++i)
        {
            vals.push_back(ConvertAssetCollectionTypeToString(types->operator[](i)));
        }

        prototypeTypes = std::move(vals);
    }

    StringVec prototypeTags = Convert(tags);
    StringVec groupIds = Convert(spaceIds);

    int32_t skip = resultsSkipNumber.HasValue() ? *resultsSkipNumber : DEFAULT_SKIP_NUMBER;
    int32_t limit = resultsMaxNumber.HasValue() ? *resultsMaxNumber : DEFAULT_RESULT_MAX_NUMBER;

    services::ResponseHandlerPtr responseHandler
        = m_prototypeApi->CreateHandler<AssetCollectionsResultCallback, AssetCollectionsResult, void, services::DtoArray<chs::PrototypeDto>>(
            [](const AssetCollectionsResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(onCompleteEvent));

    static_cast<chs::PrototypeApi*>(m_prototypeApi)
        ->prototypesGet(
            {
                prototypeTags, // Tags
                std::nullopt, // ExcludedTags
                std::nullopt, // TagsAll
                prototypeIds, // Ids
                prototypeNames, // Names
                std::nullopt, // PartialNames
                std::nullopt, // ExcludedIds
                std::nullopt, // PointOfInterestIds
                parentPrototypeId, // ParentId
                groupIds, // GroupIds
                prototypeTypes, // Types
                std::nullopt, // HasGroup
                std::nullopt, // CreatedBy
                std::nullopt, // CreatedAfter
                std::nullopt, // PrototypeOwnerIds
                std::nullopt, // ReadAccessFilters
                std::nullopt, // WriteAccessFilters
                std::nullopt, // OrganizationIds (no longer used)
                std::nullopt, // ExcludedTypes
                skip, // Skip
                limit, // Limit
                std::nullopt, // SortBy
                std::nullopt // SortDirection
            },
            responseHandler);

    return onCompleteTask;
}

void AssetSystem::UpdateAssetCollectionMetadata(const AssetCollection& assetCollection, const Map<String, String>& newMetadata,
    const Optional<Array<String>>& tags, AssetCollectionResultCallback callback)
{
    auto prototypeInfo = CreatePrototypeDto(assetCollection.SpaceId, assetCollection.ParentId, assetCollection.Name, newMetadata,
        assetCollection.Type, tags.HasValue() ? tags : assetCollection.Tags);

    services::ResponseHandlerPtr responseHandler
        = m_prototypeApi->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(callback, nullptr);

    static_cast<chs::PrototypeApi*>(m_prototypeApi)->prototypesIdPut({ assetCollection.Id, prototypeInfo }, responseHandler);
}

async::task<AssetCollectionResult> AssetSystem::UpdateAssetCollectionMetadata(const AssetCollection& assetCollection,
    const csp::common::Map<csp::common::String, csp::common::String>& newMetadata,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags)
{
    async::event_task<AssetCollectionResult> onCompleteEvent;
    async::task<AssetCollectionResult> onCompleteTask = onCompleteEvent.get_task();

    auto prototypeInfo = CreatePrototypeDto(assetCollection.SpaceId, assetCollection.ParentId, assetCollection.Name, newMetadata,
        assetCollection.Type, tags.HasValue() ? tags : assetCollection.Tags);

    services::ResponseHandlerPtr responseHandler
        = m_prototypeApi->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(
            [](const AssetCollectionResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(onCompleteEvent));

    static_cast<chs::PrototypeApi*>(m_prototypeApi)->prototypesIdPut({ assetCollection.Id, prototypeInfo }, responseHandler);

    return onCompleteTask;
}

void AssetSystem::GetAssetCollectionCount(const csp::common::Optional<csp::common::Array<csp::common::String>>& ids,
    const csp::common::Optional<csp::common::String>& parentId, const csp::common::Optional<csp::common::Array<csp::common::String>>& names,
    const csp::common::Optional<csp::common::Array<EAssetCollectionType>>& types,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& spaceIds, csp::systems::AssetCollectionCountResultCallback callback)
{
    std::optional<std::vector<String>> prototypeIds = Convert(ids);
    std::optional<String> parentPrototypeId = Convert(parentId);
    std::optional<std::vector<String>> prototypeNames = Convert(names);
    std::optional<std::vector<String>> prototypeTypes;

    if (types.HasValue())
    {
        std::vector<String> vals;

        for (size_t i = 0; i < types->Size(); ++i)
        {
            vals.push_back(ConvertAssetCollectionTypeToString(types->operator[](i)));
        }

        prototypeTypes = std::move(vals);
    }

    std::optional<std::vector<String>> prototypeTags = Convert(tags);
    std::optional<std::vector<String>> groupIds = Convert(spaceIds);

    services::ResponseHandlerPtr responseHandler = m_prototypeApi->CreateHandler<csp::systems::AssetCollectionCountResultCallback,
        csp::systems::AssetCollectionCountResult, void, services::DtoArray<chs::PrototypeDto>>(callback, nullptr);

    static_cast<chs::PrototypeApi*>(m_prototypeApi)
        ->prototypesCountGet(
            {
                prototypeTags, // Tags
                std::nullopt, // ExcludedTags
                std::nullopt, // TagsAll
                prototypeIds, // Ids
                prototypeNames, // Names
                std::nullopt, // PartialNames
                std::nullopt, // ExcludedIds
                std::nullopt, // PointOfInterestIds
                parentPrototypeId, // ParentId
                groupIds, // GroupIds
                prototypeTypes, // Types
                std::nullopt, // HasGroup
                std::nullopt, // CreatedBy
                std::nullopt, // CreatedAfter
                std::nullopt, // PrototypeOwnerIds
                std::nullopt, // ReadAccessFilters
                std::nullopt, // WriteAccessFilters
                std::nullopt, // OrganizationIds
                std::nullopt // ExcludedTypes
            },
            responseHandler);
}

void AssetSystem::CreateAsset(const AssetCollection& assetCollection, const String& name, const Optional<String>& thirdPartyPackagedAssetIdentifier,
    const Optional<EThirdPartyPlatform>& thirdPartyPlatform, EAssetType type, AssetResultCallback callback)
{
    auto assetInfo = std::make_shared<chs::AssetDetailDto>();
    assetInfo->SetName(name);
    String inAddressableId;

    if (thirdPartyPackagedAssetIdentifier.HasValue() || thirdPartyPlatform.HasValue())
    {
        if (thirdPartyPackagedAssetIdentifier.HasValue() && thirdPartyPlatform.HasValue())
        {
            inAddressableId = StringFormat("%s|%d", thirdPartyPackagedAssetIdentifier->c_str(), static_cast<int>(*thirdPartyPlatform));
        }
        else if (thirdPartyPackagedAssetIdentifier.HasValue())
        {
            inAddressableId = StringFormat("%s|%d", thirdPartyPackagedAssetIdentifier->c_str(), static_cast<int>(EThirdPartyPlatform::None));
        }
        else if (thirdPartyPlatform.HasValue())
        {
            inAddressableId = StringFormat("%s|%d", "", static_cast<int>(*thirdPartyPlatform));
        }

        // TODO: CHS naming refactor planned for AssetDetailDto.m_AddressableId, becoming AssetDetailDto.m_ThirdPartyReferenceId
        assetInfo->SetAddressableId(inAddressableId);
    }

    assetInfo->SetAssetType(ConvertAssetTypeToString(type));

    // TODO: Move this to a separate function when we have some different values than DEFAULT
    std::vector<String> styles;
    const auto defaultStyle = "Default";
    styles.push_back(defaultStyle);
    assetInfo->SetStyle(styles);

    // TODO: Move this to a separate function when we have some different values than DEFAULT
    std::vector<String> platform;
    const auto defaultPlatform = "Default";
    platform.push_back(defaultPlatform);
    assetInfo->SetSupportedPlatforms(platform);

    services::ResponseHandlerPtr responseHandler = m_assetDetailApi->CreateHandler<AssetResultCallback, AssetResult, void, chs::AssetDetailDto>(
        callback, nullptr, web::EResponseCodes::ResponseCreated);

    static_cast<chs::AssetDetailApi*>(m_assetDetailApi)->prototypesPrototypeIdAsset_detailsPost({ assetCollection.Id, assetInfo }, responseHandler);
}

async::task<AssetResult> AssetSystem::CreateAsset(const AssetCollection& assetCollection, const csp::common::String& name,
    const csp::common::Optional<csp::common::String>& thirdPartyPackagedAssetIdentifier,
    const csp::common::Optional<csp::systems::EThirdPartyPlatform>& thirdPartyPlatform, EAssetType type)
{
    async::event_task<AssetResult> onCompleteEvent;
    async::task<AssetResult> onCompleteTask = onCompleteEvent.get_task();

    auto assetInfo = std::make_shared<chs::AssetDetailDto>();
    assetInfo->SetName(name);
    String inAddressableId;

    if (thirdPartyPackagedAssetIdentifier.HasValue() || thirdPartyPlatform.HasValue())
    {
        if (thirdPartyPackagedAssetIdentifier.HasValue() && thirdPartyPlatform.HasValue())
        {
            inAddressableId = StringFormat("%s|%d", thirdPartyPackagedAssetIdentifier->c_str(), static_cast<int>(*thirdPartyPlatform));
        }
        else if (thirdPartyPackagedAssetIdentifier.HasValue())
        {
            inAddressableId = StringFormat("%s|%d", thirdPartyPackagedAssetIdentifier->c_str(), static_cast<int>(EThirdPartyPlatform::None));
        }
        else if (thirdPartyPlatform.HasValue())
        {
            inAddressableId = StringFormat("%s|%d", "", static_cast<int>(*thirdPartyPlatform));
        }

        // TODO: CHS naming refactor planned for AssetDetailDto.m_AddressableId, becoming AssetDetailDto.m_ThirdPartyReferenceId
        assetInfo->SetAddressableId(inAddressableId);
    }

    assetInfo->SetAssetType(ConvertAssetTypeToString(type));

    // TODO: Move this to a separate function when we have some different values than DEFAULT
    std::vector<String> styles;
    const auto defaultStyle = "Default";
    styles.push_back(defaultStyle);
    assetInfo->SetStyle(styles);

    // TODO: Move this to a separate function when we have some different values than DEFAULT
    std::vector<String> platform;
    const auto defaultPlatform = "Default";
    platform.push_back(defaultPlatform);
    assetInfo->SetSupportedPlatforms(platform);

    services::ResponseHandlerPtr responseHandler = m_assetDetailApi->CreateHandler<AssetResultCallback, AssetResult, void, chs::AssetDetailDto>(
        [](const AssetResult&) {}, nullptr, web::EResponseCodes::ResponseCreated, std::move(onCompleteEvent));

    static_cast<chs::AssetDetailApi*>(m_assetDetailApi)->prototypesPrototypeIdAsset_detailsPost({ assetCollection.Id, assetInfo }, responseHandler);

    return onCompleteTask;
}

void AssetSystem::UpdateAsset(const Asset& asset, AssetResultCallback callback)
{
    auto assetInfo = std::make_shared<chs::AssetDetailDto>();
    assetInfo->SetName(asset.Name);
    assetInfo->SetLanguageCode(asset.LanguageCode);
    assetInfo->SetStyle(Convert(asset.Styles));

    // TODO: Move this to a separate function when we have some different values than DEFAULT
    std::vector<String> platform;
    const auto defaultPlatform = "Default";
    platform.push_back(defaultPlatform);
    assetInfo->SetSupportedPlatforms(platform);

    if (!asset.ExternalUri.IsEmpty() && !asset.ExternalMimeType.IsEmpty())
    {
        assetInfo->SetExternalUri(asset.ExternalUri);
        assetInfo->SetExternalMimeType(asset.ExternalMimeType);
    }

    assetInfo->SetAddressableId(
        StringFormat("%s|%d", asset.ThirdPartyPackagedAssetIdentifier.c_str(), static_cast<int>(asset.ThirdPartyPlatformType)));

    assetInfo->SetAssetType(ConvertAssetTypeToString(asset.Type));
    services::ResponseHandlerPtr responseHandler = m_assetDetailApi->CreateHandler<AssetResultCallback, AssetResult, void, chs::AssetDetailDto>(
        callback, nullptr, web::EResponseCodes::ResponseCreated);
    static_cast<chs::AssetDetailApi*>(m_assetDetailApi)
        ->prototypesPrototypeIdAsset_detailsAssetDetailIdPut({ asset.AssetCollectionId, asset.Id, assetInfo }, responseHandler);
}

void AssetSystem::DeleteAsset(const AssetCollection& assetCollection, const Asset& asset, NullResultCallback callback)
{
    DeleteAssetById(assetCollection.Id, asset.Id, callback);
}

async::task<NullResult> AssetSystem::DeleteAsset(const AssetCollection& assetCollection, const Asset& asset)
{
    std::shared_ptr<async::event_task<NullResult>> onCompleteEvent = std::make_shared<async::event_task<NullResult>>();
    async::task<NullResult> onCompleteTask = onCompleteEvent->get_task();

    DeleteAssetById(assetCollection.Id, asset.Id,
        [onCompleteEvent](const NullResult& result)
        {
            if (result.GetResultCode() == EResultCode::Success || result.GetResultCode() == EResultCode::Failed)
            {
                onCompleteEvent->set(result);
            }
        });

    return onCompleteTask;
}

void AssetSystem::GetAssetsInCollection(const AssetCollection& assetCollection, AssetsResultCallback callback)
{
    std::vector<String> prototypeIds = { assetCollection.Id };

    services::ResponseHandlerPtr responseHandler
        = m_assetDetailApi->CreateHandler<AssetsResultCallback, AssetsResult, void, services::DtoArray<chs::AssetDetailDto>>(callback, nullptr);

    static_cast<chs::AssetDetailApi*>(m_assetDetailApi)
        ->prototypesAsset_detailsGet(
            {
                std::nullopt, // Ids
                std::nullopt, // SupportedPlatforms
                std::nullopt, // AssetTypes
                std::nullopt, // Styles
                std::nullopt, // Names
                std::nullopt, // CreatedAfter
                prototypeIds, // PrototypeIds
                std::nullopt, // PrototypeNames
                std::nullopt, // PrototypeParentNames
                std::nullopt, // Tags
                std::nullopt, // ExcludedTags
                std::nullopt // TagsAll
            },
            responseHandler);
}

void AssetSystem::GetAssetById(const String& assetCollectionId, const String& assetId, AssetResultCallback callback)
{
    services::ResponseHandlerPtr responseHandler
        = m_assetDetailApi->CreateHandler<AssetResultCallback, AssetResult, void, chs::AssetDetailDto>(callback, nullptr);

    static_cast<chs::AssetDetailApi*>(m_assetDetailApi)
        ->prototypesPrototypeIdAsset_detailsAssetDetailIdGet({ assetCollectionId, assetId }, responseHandler);
}

void AssetSystem::GetAssetsByCriteria(const Array<String>& assetCollectionIds, const Optional<Array<String>>& assetIds,
    const Optional<Array<String>>& assetNames, const Optional<Array<EAssetType>>& assetTypes, AssetsResultCallback callback)
{
    if (assetCollectionIds.IsEmpty())
    {
        CSP_LOG_MSG(LogLevel::Error, "You have to provide at least one AssetCollectionId");

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<AssetsResult>());

        return;
    }

    std::vector<String> prototypeIds;
    prototypeIds.reserve(assetCollectionIds.Size());

    for (size_t idx = 0; idx < assetCollectionIds.Size(); ++idx)
    {
        prototypeIds.push_back(assetCollectionIds[idx]);
    }

    std::optional<std::vector<String>> assetDetailIds;

    if (assetIds.HasValue())
    {
        assetDetailIds.emplace(std::vector<String>());
        assetDetailIds->reserve(assetIds->Size());

        for (size_t idx = 0; idx < assetIds->Size(); ++idx)
        {
            assetDetailIds->push_back({ (*assetIds)[idx] });
        }
    }

    std::optional<std::vector<String>> assetDetailNames;

    if (assetNames.HasValue())
    {
        assetDetailNames.emplace(std::vector<String>());
        assetDetailNames->reserve(assetNames->Size());

        for (size_t idx = 0; idx < assetNames->Size(); ++idx)
        {
            assetDetailNames->push_back((*assetNames)[idx]);
        }
    }

    std::optional<std::vector<String>> assetDetailTypes;

    if (assetTypes.HasValue())
    {
        assetDetailTypes.emplace(std::vector<String>());
        assetDetailTypes->reserve(assetTypes->Size());

        for (size_t idx = 0; idx < assetTypes->Size(); ++idx)
        {
            assetDetailTypes->push_back(ConvertAssetTypeToString((*assetTypes)[idx]));
        }
    }

    services::ResponseHandlerPtr responseHandler
        = m_assetDetailApi->CreateHandler<AssetsResultCallback, AssetsResult, void, services::DtoArray<chs::AssetDetailDto>>(callback, nullptr);

    static_cast<chs::AssetDetailApi*>(m_assetDetailApi)
        ->prototypesAsset_detailsGet(
            {
                assetDetailIds, // Ids
                std::nullopt, // SupportedPlatforms
                assetDetailTypes, // AssetTypes
                std::nullopt, // Styles
                assetDetailNames, // Names
                std::nullopt, // CreatedAfter
                prototypeIds, // PrototypeIds
                std::nullopt, // PrototypeNames
                std::nullopt, // PrototypeParentNames
                std::nullopt, // Tags
                std::nullopt, // ExcludedTags
                std::nullopt // TagsAll
            },
            responseHandler);
}

async::task<AssetsResult> AssetSystem::GetAssetsByCriteria(const csp::common::Array<csp::common::String>& assetCollectionIds,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& assetIds,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& assetNames,
    const csp::common::Optional<csp::common::Array<EAssetType>>& assetTypes)
{
    async::event_task<AssetsResult> onCompleteEvent;
    async::task<AssetsResult> onCompleteTask = onCompleteEvent.get_task();

    if (assetCollectionIds.IsEmpty())
    {
        onCompleteEvent.set_exception(std::make_exception_ptr(
            csp::common::continuations::ResultException("You have to provide at least one AssetCollectionId", MakeInvalid<AssetsResult>())));

        return onCompleteTask;
    }

    std::vector<String> prototypeIds;
    prototypeIds.reserve(assetCollectionIds.Size());

    for (size_t idx = 0; idx < assetCollectionIds.Size(); ++idx)
    {
        prototypeIds.push_back(assetCollectionIds[idx]);
    }

    std::optional<std::vector<String>> assetDetailIds;

    if (assetIds.HasValue())
    {
        assetDetailIds.emplace(std::vector<String>());
        assetDetailIds->reserve(assetIds->Size());

        for (size_t idx = 0; idx < assetIds->Size(); ++idx)
        {
            assetDetailIds->push_back({ (*assetIds)[idx] });
        }
    }

    std::optional<std::vector<String>> assetDetailNames;

    if (assetNames.HasValue())
    {
        assetDetailNames.emplace(std::vector<String>());
        assetDetailNames->reserve(assetNames->Size());

        for (size_t idx = 0; idx < assetNames->Size(); ++idx)
        {
            assetDetailNames->push_back((*assetNames)[idx]);
        }
    }

    std::optional<std::vector<String>> assetDetailTypes;

    if (assetTypes.HasValue())
    {
        assetDetailTypes.emplace(std::vector<String>());
        assetDetailTypes->reserve(assetTypes->Size());

        for (size_t idx = 0; idx < assetTypes->Size(); ++idx)
        {
            assetDetailTypes->push_back(ConvertAssetTypeToString((*assetTypes)[idx]));
        }
    }

    services::ResponseHandlerPtr responseHandler
        = m_assetDetailApi->CreateHandler<AssetsResultCallback, AssetsResult, void, services::DtoArray<chs::AssetDetailDto>>(
            [](const AssetsResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(onCompleteEvent));

    static_cast<chs::AssetDetailApi*>(m_assetDetailApi)
        ->prototypesAsset_detailsGet(
            {
                assetDetailIds, // Ids
                std::nullopt, // SupportedPlatforms
                assetDetailTypes, // AssetTypes
                std::nullopt, // Styles
                assetDetailNames, // Names
                std::nullopt, // CreatedAfter
                prototypeIds, // PrototypeIds
                std::nullopt, // PrototypeNames
                std::nullopt, // PrototypeParentNames
                std::nullopt, // Tags
                std::nullopt, // ExcludedTags
                std::nullopt // TagsAll
            },
            responseHandler);

    return onCompleteTask;
}

void AssetSystem::GetAssetsByCollectionIds(const Array<String>& assetCollectionIds, AssetsResultCallback callback)
{
    if (assetCollectionIds.IsEmpty())
    {
        CSP_LOG_MSG(LogLevel::Error, "You have to provide at least one AssetCollectionId");

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<AssetsResult>());

        return;
    }

    std::vector<String> ids;

    for (size_t i = 0; i < assetCollectionIds.Size(); ++i)
    {
        ids.push_back(assetCollectionIds[i]);
    }

    services::ResponseHandlerPtr responseHandler
        = m_assetDetailApi->CreateHandler<AssetsResultCallback, AssetsResult, void, services::DtoArray<chs::AssetDetailDto>>(callback, nullptr);

    // Use `GET /api/v1/prototypes/asset-details` and only pass asset collection IDs
    static_cast<chs::AssetDetailApi*>(m_assetDetailApi)
        ->prototypesAsset_detailsGet(
            {
                std::nullopt, // Ids
                std::nullopt, // SupportedPlatforms
                std::nullopt, // AssetTypes
                std::nullopt, // Styles
                std::nullopt, // Names
                std::nullopt, // CreatedAfter
                ids, // PrototypeIds
                std::nullopt, // PrototypeNames
                std::nullopt, // PrototypeParentNames
                std::nullopt, // Tags
                std::nullopt, // ExcludedTags
                std::nullopt // TagsAll
            },
            responseHandler);
}

void AssetSystem::UploadAssetData(
    const AssetCollection& assetCollection, const Asset& asset, const AssetDataSource& assetDataSource, UriResultCallback callback)
{
    UploadAssetDataEx(assetCollection, asset, assetDataSource, CancellationToken::Dummy(), callback);
}

void AssetSystem::UploadAssetDataEx(const AssetCollection& assetCollection, const Asset& asset, const AssetDataSource& assetDataSource,
    CancellationToken& cancellationToken, UriResultCallback callback)
{
    if (asset.Name.IsEmpty())
    {
        INVOKE_IF_NOT_NULL(callback, MakeInvalid<UriResult>());

        return;
    }

    auto formFile = std::make_shared<web::HttpPayload>();
    assetDataSource.SetUploadContent(m_webClient, formFile.get(), asset);

    UriResultCallback internalCallback = [callback, asset](const UriResult& result)
    {
        if (result.GetFailureReason() != ERequestFailureReason::None)
        {
            CSP_LOG_ERROR_MSG(String("Asset with Id %s has failed to upload").c_str());
        }

        INVOKE_IF_NOT_NULL(callback, result);
    };

    services::ResponseHandlerPtr responseHandler = m_assetDetailApi->CreateHandler<UriResultCallback, UriResult, void, services::NullDto>(
        internalCallback, nullptr, web::EResponseCodes::ResponseOK);

    static_cast<chs::AssetDetailApi*>(m_assetDetailApi)
        ->prototypesPrototypeIdAsset_detailsAssetDetailIdBlobPost(
            { assetCollection.Id, asset.Id, std::nullopt, formFile }, responseHandler, cancellationToken);
}

async::task<UriResult> AssetSystem::UploadAssetDataEx(const AssetCollection& assetCollection, const Asset& asset,
    const AssetDataSource& assetDataSource, csp::common::CancellationToken& cancellationToken)
{
    async::event_task<UriResult> onCompleteEvent;
    async::task<UriResult> onCompleteTask = onCompleteEvent.get_task();

    if (asset.Name.IsEmpty())
    {
        onCompleteEvent.set_exception(
            std::make_exception_ptr(csp::common::continuations::ResultException("Asset name cannot be empty", MakeInvalid<UriResult>())));

        return onCompleteTask;
    }

    auto formFile = std::make_shared<web::HttpPayload>();
    assetDataSource.SetUploadContent(m_webClient, formFile.get(), asset);

    services::ResponseHandlerPtr responseHandler = m_assetDetailApi->CreateHandler<UriResultCallback, UriResult, void, services::NullDto>(
        [](const UriResult&) {}, nullptr, web::EResponseCodes::ResponseOK, std::move(onCompleteEvent));

    static_cast<chs::AssetDetailApi*>(m_assetDetailApi)
        ->prototypesPrototypeIdAsset_detailsAssetDetailIdBlobPost(
            { assetCollection.Id, asset.Id, std::nullopt, formFile }, responseHandler, cancellationToken);

    return onCompleteTask;
}

void AssetSystem::DownloadAssetData(const Asset& asset, AssetDataResultCallback callback)
{
    DownloadAssetDataEx(asset, CancellationToken::Dummy(), callback);
}

void AssetSystem::DownloadAssetDataEx(const Asset& asset, CancellationToken& cancellationToken, AssetDataResultCallback callback)
{
    services::ResponseHandlerPtr responseHandler
        = m_assetDetailApi->CreateHandler<AssetDataResultCallback, AssetDataResult, void, services::AssetFileDto>(callback, nullptr);

    m_fileManager->GetFile(asset.Uri, responseHandler, cancellationToken);
}

void AssetSystem::GetAssetDataSize(const Asset& asset, UInt64ResultCallback callback)
{
    HTTPHeadersResultCallback internalCallback = [callback](const HTTPHeadersResult& result)
    {
        UInt64Result internalResult(result.GetResultCode(), result.GetHttpResultCode());

        if (result.GetResultCode() == EResultCode::Success)
        {
            auto& headers = result.GetValue();
            auto& contentLength = headers["content-length"];
            auto value = std::strtoull(contentLength.c_str(), nullptr, 10);
            internalResult.SetValue(value);
        }

        INVOKE_IF_NOT_NULL(callback, internalResult);
    };

    services::ResponseHandlerPtr responseHandler
        = m_assetDetailApi->CreateHandler<HTTPHeadersResultCallback, HTTPHeadersResult, void, services::NullDto>(internalCallback, nullptr);

    m_fileManager->GetResponseHeaders(asset.Uri, responseHandler);
}

CSP_ASYNC_RESULT void AssetSystem::GetLODChain(const AssetCollection& assetCollection, LODChainResultCallback callback)
{
    auto getAssetsCallback = [assetCollection, callback](const AssetsResult& result)
    {
        LODChainResult lodResult(result.GetResultCode(), result.GetHttpResultCode());

        if (result.GetResultCode() == EResultCode::Success)
        {
            LODChain chain = CreateLODChainFromAssets(result.GetAssets(), assetCollection.Id);
            lodResult.SetLODChain(std::move(chain));
        }

        INVOKE_IF_NOT_NULL(callback, lodResult);
    };

    GetAssetsByCriteria({ assetCollection.Id }, nullptr, nullptr, Array<EAssetType> { EAssetType::MODEL }, getAssetsCallback);
}

CSP_ASYNC_RESULT_WITH_PROGRESS void AssetSystem::RegisterAssetToLODChain(
    const AssetCollection& assetCollection, const Asset& inAsset, int lodLevel, AssetResultCallback callback)
{
    // GetAssetsByCriteria
    auto getAssetsCallback = [this, assetCollection, inAsset, lodLevel, callback](const AssetsResult& result)
    {
        if (result.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (result.GetResultCode() == EResultCode::Failed)
        {
            INVOKE_IF_NOT_NULL(callback, result);

            return;
        }

        const Array<Asset>& assets = result.GetAssets();
        LODChain chain = CreateLODChainFromAssets(assets, assetCollection.Id);

        if (!ValidateNewLODLevelForChain(chain, lodLevel))
        {
            CSP_LOG_MSG(LogLevel::Error, "LOD level already exists in chain");

            INVOKE_IF_NOT_NULL(callback, result);

            return;
        }

        // UpdateAsset
        auto updateAssetCallback = [assetCollection, callback, assets](const AssetResult& result) { INVOKE_IF_NOT_NULL(callback, result); };

        // Add new LOD style
        Asset newAsset = inAsset;
        Array<String> newStyles(newAsset.Styles.Size() + 1);

        for (size_t i = 0; i < newAsset.Styles.Size(); ++i)
        {
            newStyles[i] = newAsset.Styles[i];
        }

        newStyles[newAsset.Styles.Size()] = CreateLODStyleVar(lodLevel);
        newAsset.Styles = std::move(newStyles);

        UpdateAsset(newAsset, updateAssetCallback);
    };

    GetAssetsByCriteria({ inAsset.AssetCollectionId }, nullptr, nullptr, Array<EAssetType> { EAssetType::MODEL }, getAssetsCallback);
}

void AssetSystem::CreateMaterial(const csp::common::String& name, const csp::systems::EShaderType shaderType, const csp::common::String& spaceId,
    csp::common::Map<csp::common::String, csp::common::String>& metadata, const csp::common::Array<csp::common::String>& assetTags,
    MaterialResultCallback callback)
{
    // 1. Create asset collection
    auto createAssetCollectionCb = [this, callback, name, spaceId, shaderType](const AssetCollectionResult& createAssetCollectionResult)
    {
        if (createAssetCollectionResult.GetResultCode() != EResultCode::Success)
        {
            callback(MaterialResult(createAssetCollectionResult.GetResultCode(), createAssetCollectionResult.GetHttpResultCode()));
            return;
        }

        // 2. Create asset
        const AssetCollection& createdAssetCollection = createAssetCollectionResult.GetAssetCollection();

        auto createAssetCb = [this, callback, createdAssetCollection, spaceId, name, shaderType](const AssetResult& createAssetResult)
        {
            if (createAssetResult.GetResultCode() != EResultCode::Success)
            {
                callback(MaterialResult(createAssetResult.GetResultCode(), createAssetResult.GetHttpResultCode()));
                return;
            }

            // 3. Upload default material
            Asset createdAsset = createAssetResult.GetAsset();
            csp::common::String materialJson;

            // Create material of the specific derived type.
            Material* newlyCreatedMaterial = InstantiateMaterialOfType(shaderType, name, createdAssetCollection.Id, createdAsset.Id);

            // Serialse material data.
            SerializeMaterialOfType(shaderType, newlyCreatedMaterial, materialJson);

            auto uploadMaterialCallback = [callback, newlyCreatedMaterial, spaceId, name](const UriResult& uploadResult)
            {
                if (uploadResult.GetResultCode() == EResultCode::InProgress)
                {
                    return;
                }

                // 4. Return created material
                MaterialResult finalResult(uploadResult.GetResultCode(), uploadResult.GetHttpResultCode());
                finalResult.SetMaterial(newlyCreatedMaterial);

                callback(finalResult);
            };

            createdAsset.FileName = CreateUniqueMaterialFileName(name, spaceId);

            // Create a new string to prevent const casting
            std::string buffer(materialJson.c_str());

            BufferAssetDataSource assetData;
            assetData.SetMimeType("application/json");
            assetData.Buffer = buffer.data();
            assetData.BufferLength = materialJson.Length();

            UploadAssetData(createdAssetCollection, createdAsset, assetData, uploadMaterialCallback);
        };

        const csp::common::String materialAssetName = CreateUniqueMaterialAssetName(name, spaceId);

        CreateAsset(createAssetCollectionResult.GetAssetCollection(), materialAssetName, nullptr, nullptr, EAssetType::MATERIAL, createAssetCb);
    };

    const csp::common::String materialCollectionName = CreateUniqueMaterialAssetCollectionName(name, spaceId);

    std::optional<String> convertedShaderType = ConvertShaderTypeToString(shaderType);
    if (convertedShaderType.has_value())
    {
        // Set the shader type in the material collection metadata - this is required to deserialize a material to the correct type.
        metadata[MATERIAL_SHADERTYPE_METADATA_KEY] = convertedShaderType.value();
    }
    else
    {
        CSP_LOG_MSG(LogLevel::Error, "Specified shader type invalid.");
        INVOKE_IF_NOT_NULL(callback, MakeInvalid<MaterialResult>());
        return;
    }

    CreateAssetCollection(spaceId, nullptr, materialCollectionName, metadata, EAssetCollectionType::DEFAULT, assetTags, createAssetCollectionCb);
}

void AssetSystem::UpdateMaterial(const Material& material, NullResultCallback callback)
{
    // 1. Get asset collection
    auto getAssetCollectionCb = [this, &material, callback](const AssetCollectionResult& createAssetCollectionResult)
    {
        if (createAssetCollectionResult.GetResultCode() != EResultCode::Success)
        {
            callback(NullResult(createAssetCollectionResult.GetResultCode(), createAssetCollectionResult.GetHttpResultCode()));
            return;
        }

        // 2. Get asset
        const AssetCollection& createdAssetCollection = createAssetCollectionResult.GetAssetCollection();

        auto getAssetCb = [this, callback, &material, createdAssetCollection](const AssetResult& createAssetResult)
        {
            if (createAssetResult.GetResultCode() != EResultCode::Success)
            {
                callback(NullResult(createAssetResult.GetResultCode(), createAssetResult.GetHttpResultCode()));
                return;
            }

            // 3. Upload material
            auto uploadMaterialCallback
                = [callback](const UriResult& uploadResult) { callback(NullResult(uploadResult.GetResultCode(), uploadResult.GetHttpResultCode())); };

            csp::common::String materialJson;

            // Serialse material data.
            SerializeMaterialOfType(material.GetShaderType(), &material, materialJson);

            const Asset& createdAsset = createAssetResult.GetAsset();

            // Create a new string to prevent const casting
            std::string buffer(materialJson.c_str());

            BufferAssetDataSource assetData;
            assetData.SetMimeType("application/json");
            assetData.Buffer = buffer.data();
            assetData.BufferLength = materialJson.Length();

            UploadAssetData(createdAssetCollection, createdAsset, assetData, uploadMaterialCallback);
        };
        GetAssetById(material.GetMaterialCollectionId(), material.GetMaterialId(), getAssetCb);
    };

    GetAssetCollectionById(material.GetMaterialCollectionId(), getAssetCollectionCb);
}

void AssetSystem::DeleteMaterial(const Material& material, NullResultCallback callback)
{
    // 1. Delete asset
    auto deleteAssetCb = [this, callback, &material](const NullResult& deleteAssetResult)
    {
        if (deleteAssetResult.GetResultCode() != EResultCode::Success)
        {
            callback(NullResult(deleteAssetResult.GetResultCode(), deleteAssetResult.GetHttpResultCode()));
            return;
        }

        // 2. Delete asset collection
        auto deleteAssetCollectionCb = [callback](const NullResult& deleteAssetCollectionResult) { callback(deleteAssetCollectionResult); };

        DeleteAssetCollectionById(material.GetMaterialCollectionId(), deleteAssetCollectionCb);
    };

    DeleteAssetById(material.GetMaterialCollectionId(), material.GetMaterialId(), deleteAssetCb);
}

async::task<MaterialResult> AssetSystem::DownloadMaterial(
    const AssetCollection& assetCollection, const csp::common::String& assetId, const csp::common::String& uri)
{
    auto onCompleteEvent = std::make_shared<async::event_task<MaterialResult>>();
    auto onCompleteTask = onCompleteEvent->get_task();

    GetMaterialFromUri(assetCollection, assetId, uri,
        [onCompleteEvent, assetId](const auto& result)
        {
            if (result.GetResultCode() == EResultCode::Failed)
            {
                onCompleteEvent->set_exception(
                    std::make_exception_ptr(std::runtime_error(fmt::format("Failed to download Material: {}", assetId.c_str()))));
                return;
            }

            if (result.GetResultCode() == EResultCode::Success)
            {
                onCompleteEvent->set(result);
                return;
            }
        });

    return onCompleteTask;
}

std::function<async::task<MaterialsResult>(const AssetsResult&)> AssetSystem::DownloadAllMaterials(
    const csp::common::Array<AssetCollection>& assetCollections)
{
    return [this, assetCollections](const AssetsResult& getAssetsResult) -> async::task<MaterialsResult>
    {
        const auto& assets = getAssetsResult.GetAssets();

        if (assets.IsEmpty())
        {
            // There are no material assets in this space
            return async::make_task(MaterialsResult(getAssetsResult.GetResultCode(), getAssetsResult.GetHttpResultCode()));
        }

        auto downloadTasks = std::vector<async::task<MaterialResult>>();
        downloadTasks.reserve(assets.Size());

        for (const auto& asset : assets)
        {
            if (const auto assetCollection = std::find_if(std::begin(assetCollections), std::end(assetCollections),
                    [&](const auto& collection) { return collection.Id == asset.AssetCollectionId; });
                assetCollection != std::end(assetCollections))
            {
                downloadTasks.push_back(DownloadMaterial(*assetCollection, asset.Id, asset.Uri));
            }
            else
            {
                CSP_LOG_ERROR_MSG("A Material Collection with the specified Id was not found.");
            }
        }

        return async::when_all(downloadTasks)
            .then(
                [](std::vector<async::task<MaterialResult>> downloadTasks) -> MaterialsResult
                {
                    auto downloadedMaterials = std::vector<Material*>();
                    downloadedMaterials.reserve(downloadTasks.size());

                    for (auto& task : downloadTasks)
                    {
                        try
                        {
                            if (auto result = task.get().GetMaterial())
                            {
                                downloadedMaterials.push_back(result);
                            }
                        }
                        catch (const std::exception& exception)
                        {
                            CSP_LOG_ERROR_FORMAT("AssetSystem::GetMaterials: %s", exception.what());
                        }
                    }

                    if (downloadedMaterials.empty())
                    {
                        return MakeInvalid<MaterialsResult>();
                    }

                    auto materials = csp::common::Array<Material*>(downloadedMaterials.size());

                    for (size_t i = 0; i < downloadedMaterials.size(); ++i)
                    {
                        materials[i] = downloadedMaterials[i];
                    }

                    auto result = MaterialsResult(EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));
                    result.SetMaterials(materials);

                    return result;
                });
    };
}

void AssetSystem::GetMaterials(const csp::common::String& spaceId, MaterialsResultCallback callback)
{
    auto fetchMaterials = [this](const AssetCollectionsResult& result) -> async::task<MaterialsResult>
    {
        if (result.GetResultCode() != EResultCode::Success)
        {
            throw std::runtime_error(fmt::format("FindAssetCollections returned failure response code {}", result.GetHttpResultCode()));
        }

        const auto& assetCollections = result.GetAssetCollections();

        if (assetCollections.IsEmpty())
        {
            return async::make_task(MaterialsResult(result.GetResultCode(), result.GetHttpResultCode()));
        }

        auto assetCollectionIds = csp::common::Array<csp::common::String>(assetCollections.Size());

        for (size_t i = 0; i < assetCollections.Size(); ++i)
        {
            assetCollectionIds[i] = assetCollections[i].Id;
        }

        return GetAssetsByCriteria(assetCollectionIds, nullptr, nullptr, csp::common::Array { EAssetType::MATERIAL })
            .then(DownloadAllMaterials(assetCollections));
    };

    FindAssetCollections(nullptr, nullptr, nullptr, nullptr, nullptr, csp::common::Array<csp::common::String> { spaceId }, nullptr, nullptr)
        .then(std::move(fetchMaterials))
        .then(
            [callback](async::task<MaterialsResult> result)
            {
                try
                {
                    callback(result.get());
                }
                catch (const std::exception& exception)
                {
                    CSP_LOG_ERROR_FORMAT("AssetSystem::GetMaterials failed: %s", exception.what());
                    callback(MakeInvalid<MaterialsResult>());
                }
                catch (...)
                {
                    callback(MakeInvalid<MaterialsResult>());
                }
            });
}

void AssetSystem::GetMaterial(const csp::common::String& assetCollectionId, const csp::common::String& assetId, MaterialResultCallback callback)
{
    // 1. Get asset collection
    auto getAssetCollectionCb = [this, assetCollectionId, assetId, callback](const AssetCollectionResult& createAssetCollectionResult)
    {
        if (createAssetCollectionResult.GetResultCode() != EResultCode::Success)
        {
            callback(MaterialResult(createAssetCollectionResult.GetResultCode(), createAssetCollectionResult.GetHttpResultCode()));
            return;
        }

        // 2. Get asset
        const AssetCollection& foundAssetCollection = createAssetCollectionResult.GetAssetCollection();

        auto getAssetCb = [this, callback, foundAssetCollection](const AssetResult& createAssetResult)
        {
            if (createAssetResult.GetResultCode() != EResultCode::Success)
            {
                callback(MaterialResult(createAssetResult.GetResultCode(), createAssetResult.GetHttpResultCode()));
                return;
            }

            // 3. Download material
            const Asset& foundAsset = createAssetResult.GetAsset();
            GetMaterialFromUri(foundAssetCollection, foundAsset.Id, foundAsset.Uri, callback);
        };

        GetAssetById(assetCollectionId, assetId, getAssetCb);
    };

    GetAssetCollectionById(assetCollectionId, getAssetCollectionCb);
}

void AssetSystem::GetMaterialFromUri(const csp::systems::AssetCollection& assetCollection, const csp::common::String& assetId,
    const csp::common::String& uri, MaterialResultCallback callback)
{
    std::optional<csp::systems::EShaderType> shaderType = GetShaderTypeFromMaterialCollection(assetCollection);
    if (!shaderType.has_value())
    {
        CSP_LOG_ERROR_MSG("Error: Material contains an invalid shader type.");
        INVOKE_IF_NOT_NULL(callback, MakeInvalid<MaterialResult>());
        return;
    }

    auto downloadMaterialCallback
        = [callback, assetId, assetCollectionId = assetCollection.Id, shaderType = *shaderType](const AssetDataResult& downloadResult)
    {
        if (downloadResult.GetResultCode() != EResultCode::Success)
        {
            callback(MaterialResult(downloadResult.GetResultCode(), downloadResult.GetHttpResultCode()));
            return;
        }

        const char* materialData = static_cast<const char*>(downloadResult.GetData());

        // Create material of the specific derived type.
        Material* foundMaterial = InstantiateMaterialOfType(shaderType, "", assetCollectionId, assetId);

        // Deserialse material data.
        auto deserializationResult = DeserializeIntoMaterialOfType(materialData, shaderType, foundMaterial);

        if (!deserializationResult.has_value())
        {
            CSP_LOG_ERROR_MSG("Failed to deserialize material");

            INVOKE_IF_NOT_NULL(callback, MakeInvalid<MaterialResult>());

            return;
        }

        MaterialResult result(downloadResult.GetResultCode(), downloadResult.GetHttpResultCode());
        result.SetMaterial(deserializationResult.value());

        callback(result);
    };

    Asset asset;
    asset.Uri = uri;

    DownloadAssetData(asset, downloadMaterialCallback);
}

void AssetSystem::SetAssetDetailBlobChangedCallback(AssetDetailBlobChangedCallbackHandler callback)
{
    m_assetDetailBlobChangedCallback = callback;

    RegisterSystemCallback();
}

void AssetSystem::SetMaterialChangedCallback(MaterialChangedCallbackHandler callback)
{
    m_materialChangedCallback = callback;

    RegisterSystemCallback();
}

void AssetSystem::RegisterSystemCallback()
{
    if (!m_eventBusPtr)
    {
        CSP_LOG_ERROR_MSG("Error: Failed to register AssetSystem. NetworkEventBus must be instantiated in the MultiplayerConnection first.");
        return;
    }

    if (!m_materialChangedCallback && !m_assetDetailBlobChangedCallback)
    {
        CSP_LOG_ERROR_MSG("Error: Neither MaterialChangedCallback nor AssetDetailBlobChangedCallback were set, not registering AssetSystem to "
                          "AssetDetailBlobChanged.\nPlease set either callback before registering.");
        return;
    }

    m_eventBusPtr->ListenNetworkEvent(
        csp::multiplayer::NetworkEventRegistration("CSPInternal::AssetSystem",
            csp::multiplayer::NetworkEventBus::StringFromNetworkEvent(csp::multiplayer::NetworkEventBus::NetworkEvent::AssetDetailBlobChanged)),
        [this](const csp::common::NetworkEventData& networkEventData) { this->OnAssetDetailBlobChangedEvent(networkEventData); });
}

void AssetSystem::OnAssetDetailBlobChangedEvent(const csp::common::NetworkEventData& networkEventData)
{
    if (!m_assetDetailBlobChangedCallback && !m_materialChangedCallback)
    {
        return;
    }

    const csp::common::AssetDetailBlobChangedNetworkEventData& assetDetailBlobChangedNetworkEventData
        = static_cast<const csp::common::AssetDetailBlobChangedNetworkEventData&>(networkEventData);

    if (m_assetDetailBlobChangedCallback)
    {
        m_assetDetailBlobChangedCallback(assetDetailBlobChangedNetworkEventData);
    }

    if (assetDetailBlobChangedNetworkEventData.AssetType == systems::EAssetType::MATERIAL && m_materialChangedCallback)
    {
        csp::common::MaterialChangedParams materialParams;
        materialParams.ChangeType = assetDetailBlobChangedNetworkEventData.ChangeType;
        materialParams.MaterialCollectionId = assetDetailBlobChangedNetworkEventData.AssetCollectionId;
        materialParams.MaterialId = assetDetailBlobChangedNetworkEventData.AssetId;

        m_materialChangedCallback(materialParams);
    }
}

} // namespace csp::systems
