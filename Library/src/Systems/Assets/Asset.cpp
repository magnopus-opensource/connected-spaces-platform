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
#include "CSP/Systems/Assets/Asset.h"

#include "CSP/Common/StringFormat.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "Common/Convert.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/PrototypeService/AssetFileDto.h"
#include "Services/PrototypeService/Dto.h"

namespace chs = csp::services::generated::prototypeservice;

namespace csp::systems
{

csp::systems::EAssetType ConvertDTOAssetDetailType(const csp::common::String& dtoAssetDetailType)
{
    if (dtoAssetDetailType == "Image")
        return csp::systems::EAssetType::IMAGE;
    else if (dtoAssetDetailType == "Thumbnail")
        return csp::systems::EAssetType::THUMBNAIL;
    else if (dtoAssetDetailType == "Simulation")
        return csp::systems::EAssetType::SIMULATION;
    else if (dtoAssetDetailType == "Model")
        return csp::systems::EAssetType::MODEL;
    else if (dtoAssetDetailType == "Video")
        return csp::systems::EAssetType::VIDEO;
    else if (dtoAssetDetailType == "ScriptLibrary")
        return csp::systems::EAssetType::SCRIPT_LIBRARY;
    else if (dtoAssetDetailType == "HolocapVideo")
        return csp::systems::EAssetType::HOLOCAP_VIDEO;
    else if (dtoAssetDetailType == "HolocapAudio")
        return csp::systems::EAssetType::HOLOCAP_AUDIO;
    else if (dtoAssetDetailType == "Audio")
        return csp::systems::EAssetType::AUDIO;
    else if (dtoAssetDetailType == "GaussianSplat")
        return csp::systems::EAssetType::GAUSSIAN_SPLAT;
    else if (dtoAssetDetailType == "Material")
        return csp::systems::EAssetType::MATERIAL;
    else if (dtoAssetDetailType == "Annotation")
        return csp::systems::EAssetType::ANNOTATION;
    else if (dtoAssetDetailType == "AnnotationThumbnail")
        return csp::systems::EAssetType::ANNOTATION_THUMBNAIL;
    else if (dtoAssetDetailType == "Text")
        return csp::systems::EAssetType::TEXT;
    else
    {
        CSP_LOG_MSG(common::LogLevel::Error, "Unsupported Asset Type!");
        return csp::systems::EAssetType::IMAGE;
    }
}

csp::systems::EAssetPlatform ConvertStringToAssetPlatform(const csp::common::String& platform)
{
    if (platform == "Default")
    {
        return EAssetPlatform::DEFAULT;
    }
    else
    {
        CSP_LOG_MSG(common::LogLevel::Error, "Unsupported Asset Platform!");
        return EAssetPlatform::DEFAULT;
    }
}

csp::common::String ConvertAssetPlatformToString(EAssetPlatform platform)
{
    switch (platform)
    {
    case EAssetPlatform::DEFAULT:
        return "Default";
    }

    CSP_LOG_MSG(common::LogLevel::Error, "Unsupported Asset Platform!");
    return "Default";
}

} // namespace csp::systems

namespace csp::systems
{

void AssetDetailDtoToAsset(const chs::AssetDetailDto& dto, csp::systems::Asset& asset)
{
    if (dto.HasPrototypeId())
    {
        asset.AssetCollectionId = dto.GetPrototypeId();
    }

    if (dto.HasId())
    {
        asset.Id = dto.GetId();
    }

    if (dto.HasFileName())
    {
        asset.FileName = dto.GetFileName();
    }

    if (dto.HasName())
    {
        asset.Name = dto.GetName();
    }

    if (dto.HasLanguageCode())
    {
        asset.LanguageCode = dto.GetLanguageCode();
    }

    if (dto.HasAssetType())
    {
        asset.Type = ConvertDTOAssetDetailType(dto.GetAssetType());
    }

    if (dto.HasSupportedPlatforms())
    {
        const auto& platforms = dto.GetSupportedPlatforms();
        asset.Platforms = csp::common::Array<csp::systems::EAssetPlatform>(platforms.size());

        for (size_t i = 0; i < platforms.size(); ++i)
        {
            // TODO Move this to a separate function when we have some different values than DEFAULT
            asset.Platforms[i] = ConvertStringToAssetPlatform(platforms[i]);
        }
    }

    if (dto.HasStyle())
    {
        asset.Styles = csp::common::Convert(dto.GetStyle());
    }

    if (dto.HasAddressableId())
    {
        // TODO CHS naming refactor planned for AssetDetailDto.m_AddressableId, becoming AssetDetailDto.m_ThirdPartyReferenceId
        const auto& inAddressableId = dto.GetAddressableId().Split('|');
        if (inAddressableId.Size() == 2)
        {
            asset.ThirdPartyPlatformType = static_cast<EThirdPartyPlatform>(std::stoi(inAddressableId[1].c_str()));
            asset.ThirdPartyPackagedAssetIdentifier = inAddressableId[0];
        }
        else
        {
            asset.ThirdPartyPackagedAssetIdentifier = dto.GetAddressableId();
            asset.ThirdPartyPlatformType = EThirdPartyPlatform::None;
        }
    }
    else
    {
        asset.ThirdPartyPackagedAssetIdentifier = "";
        asset.ThirdPartyPlatformType = EThirdPartyPlatform::None;
    }

    if (dto.HasUri())
    {
        asset.Uri = dto.GetUri();
    }

    if (dto.HasChecksum())
    {
        asset.Checksum = dto.GetChecksum();
    }

    if (dto.HasVersion())
    {
        asset.Version = std::stoi(dto.GetVersion().c_str());
    }

    if (dto.HasMimeType())
    {
        asset.MimeType = dto.GetMimeType();
    }
}

Asset::Asset()
    : Type(EAssetType::MODEL)
    , Version(0)
    , ThirdPartyPackagedAssetIdentifier("")
    , ThirdPartyPlatformType(EThirdPartyPlatform::None)
{
}

bool Asset::operator==(const Asset& other) const
{
    return AssetCollectionId == other.AssetCollectionId && Id == other.Id && FileName == other.FileName && Name == other.Name
        && LanguageCode == other.LanguageCode && Type == other.Type && Platforms == other.Platforms && Styles == other.Styles
        && ExternalUri == other.ExternalUri && Uri == other.Uri && Checksum == other.Checksum && Version == other.Version
        && MimeType == other.MimeType && ExternalMimeType == other.ExternalMimeType
        && ThirdPartyPackagedAssetIdentifier == other.ThirdPartyPackagedAssetIdentifier && ThirdPartyPlatformType == other.ThirdPartyPlatformType;
}

bool Asset::operator!=(const Asset& other) const { return !(*this == other); }

const csp::common::String& FileAssetDataSource::GetMimeType() const { return m_mimeType; }

void FileAssetDataSource::SetMimeType(const csp::common::String& inMimeType) { m_mimeType = inMimeType; }

void FileAssetDataSource::SetUploadContent(
    csp::web::WebClient* inWebClient, csp::web::HttpPayload* inPayload, const csp::systems::Asset& inAsset) const
{
    assert(!FilePath.IsEmpty());

    auto version = std::to_string(std::abs(inAsset.Version) /* Version should not be negative */);
    inWebClient->SetFileUploadContentFromFile(inPayload, FilePath, version.c_str(), m_mimeType);
}

BufferAssetDataSource::BufferAssetDataSource()
    : Buffer(nullptr)
    , BufferLength(0)
{
}

const csp::common::String& BufferAssetDataSource::GetMimeType() const { return m_mimeType; }

void BufferAssetDataSource::SetMimeType(const csp::common::String& inMimeType) { m_mimeType = inMimeType; }

void BufferAssetDataSource::SetUploadContent(
    csp::web::WebClient* inWebClient, csp::web::HttpPayload* inPayload, const csp::systems::Asset& inAsset) const
{
    assert(Buffer != nullptr);
    assert(BufferLength > 0);
    assert(!inAsset.FileName.IsEmpty());

    auto version = std::to_string(std::abs(inAsset.Version) /* Version should not be negative */);
    inWebClient->SetFileUploadContentFromBuffer(
        inPayload, reinterpret_cast<const char*>(Buffer), BufferLength, inAsset.FileName, version.c_str(), m_mimeType);
}

Asset& AssetResult::GetAsset() { return m_asset; }

const Asset& AssetResult::GetAsset() const { return m_asset; }

void AssetResult::SetAsset(const csp::systems::Asset& setAsset) { this->m_asset = setAsset; }

void AssetResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* assetDetailResponse = static_cast<chs::AssetDetailDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        assetDetailResponse->FromJson(response->GetPayload().GetContent());

        AssetDetailDtoToAsset(*assetDetailResponse, m_asset);
    }
}

csp::common::Array<Asset>& AssetsResult::GetAssets() { return m_assets; }

const csp::common::Array<Asset>& AssetsResult::GetAssets() const { return m_assets; }

void AssetsResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* assetsResponse = static_cast<csp::services::DtoArray<chs::AssetDetailDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        assetsResponse->FromJson(response->GetPayload().GetContent());

        std::vector<chs::AssetDetailDto>& detailsArray = assetsResponse->GetArray();
        m_assets = csp::common::Array<csp::systems::Asset>(detailsArray.size());

        // Extract data from response in our Projects array
        for (size_t i = 0; i < detailsArray.size(); ++i)
        {
            AssetDetailDtoToAsset(detailsArray[i], m_assets[i]);
        }
    }
}

const csp::common::String& UriResult::GetUri() const { return m_uri; }

void UriResult::SetUri(const csp::common::String& value) { m_uri = value; }

UriResult::UriResult(const csp::common::String uri)
    : m_uri(uri)
{
    SetResult(csp::systems::EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));
}

void UriResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    const auto* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        m_uri = response->GetPayload().GetContent();
    }
}

void UriResult::SetResponseBody(const csp::common::String& contents) { m_responseBody = contents; }

AssetDataResult::AssetDataResult(void*) { }

AssetDataResult::AssetDataResult(const AssetDataResult& other)
    : ResultBase(other)
{
}

AssetDataResult::~AssetDataResult() { }

const void* AssetDataResult::GetData() const { return m_responseBody.c_str(); }

size_t AssetDataResult::GetDataLength() const { return m_responseBody.Length(); }

void AssetDataResult::OnResponse(const csp::services::ApiResponseBase* apiResponse) { ResultBase::OnResponse(apiResponse); }

} // namespace csp::systems
