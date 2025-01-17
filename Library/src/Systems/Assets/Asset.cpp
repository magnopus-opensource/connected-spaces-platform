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
#include "Memory/Memory.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/PrototypeService/AssetFileDto.h"
#include "Services/PrototypeService/Dto.h"

namespace chs = csp::services::generated::prototypeservice;

namespace csp::systems
{

csp::systems::EAssetType ConvertDTOAssetDetailType(const csp::common::String& DTOAssetDetailType)
{
    if (DTOAssetDetailType == "Image")
        return csp::systems::EAssetType::IMAGE;
    else if (DTOAssetDetailType == "Thumbnail")
        return csp::systems::EAssetType::THUMBNAIL;
    else if (DTOAssetDetailType == "Simulation")
        return csp::systems::EAssetType::SIMULATION;
    else if (DTOAssetDetailType == "Model")
        return csp::systems::EAssetType::MODEL;
    else if (DTOAssetDetailType == "Video")
        return csp::systems::EAssetType::VIDEO;
    else if (DTOAssetDetailType == "ScriptLibrary")
        return csp::systems::EAssetType::SCRIPT_LIBRARY;
    else if (DTOAssetDetailType == "HolocapVideo")
        return csp::systems::EAssetType::HOLOCAP_VIDEO;
    else if (DTOAssetDetailType == "HolocapAudio")
        return csp::systems::EAssetType::HOLOCAP_AUDIO;
    else if (DTOAssetDetailType == "Audio")
        return csp::systems::EAssetType::AUDIO;
    else if (DTOAssetDetailType == "GaussianSplat")
        return csp::systems::EAssetType::GAUSSIAN_SPLAT;
    else if (DTOAssetDetailType == "Material")
        return csp::systems::EAssetType::MATERIAL;
    else
    {
        CSP_LOG_MSG(LogLevel::Error, "Unsupported Asset Type!");
        return csp::systems::EAssetType::IMAGE;
    }
}

csp::systems::EAssetPlatform ConvertStringToAssetPlatform(const csp::common::String& Platform)
{
    if (Platform == "Default")
    {
        return EAssetPlatform::DEFAULT;
    }
    else
    {
        CSP_LOG_MSG(LogLevel::Error, "Unsupported Asset Platform!");
        return EAssetPlatform::DEFAULT;
    }
}

csp::common::String ConvertAssetPlatformToString(EAssetPlatform Platform)
{
    switch (Platform)
    {
    case EAssetPlatform::DEFAULT:
        return "Default";
    }

    CSP_LOG_MSG(LogLevel::Error, "Unsupported Asset Platform!");
    return "Default";
}

void AssetDetailDtoToAsset(const chs::AssetDetailDto& Dto, csp::systems::Asset& Asset)
{
    if (Dto.HasPrototypeId())
    {
        Asset.AssetCollectionId = Dto.GetPrototypeId();
    }

    if (Dto.HasId())
    {
        Asset.Id = Dto.GetId();
    }

    if (Dto.HasFileName())
    {
        Asset.FileName = Dto.GetFileName();
    }

    if (Dto.HasName())
    {
        Asset.Name = Dto.GetName();
    }

    if (Dto.HasLanguageCode())
    {
        Asset.LanguageCode = Dto.GetLanguageCode();
    }

    if (Dto.HasAssetType())
    {
        Asset.Type = ConvertDTOAssetDetailType(Dto.GetAssetType());
    }

    if (Dto.HasSupportedPlatforms())
    {
        const auto& Platforms = Dto.GetSupportedPlatforms();
        Asset.Platforms = csp::common::Array<csp::systems::EAssetPlatform>(Platforms.size());

        for (size_t i = 0; i < Platforms.size(); ++i)
        {
            // TODO Move this to a separate function when we have some different values than DEFAULT
            Asset.Platforms[i] = ConvertStringToAssetPlatform(Platforms[i]);
        }
    }

    if (Dto.HasStyle())
    {
        Asset.Styles = csp::common::Convert(Dto.GetStyle());
    }

    if (Dto.HasAddressableId())
    {
        // TODO CHS naming refactor planned for AssetDetailDto.m_AddressableId, becoming AssetDetailDto.m_ThirdPartyReferenceId
        const auto& InAddressableId = Dto.GetAddressableId().Split('|');
        if (InAddressableId.Size() == 2)
        {
            Asset.ThirdPartyPlatformType = static_cast<EThirdPartyPlatform>(std::stoi(InAddressableId[1].c_str()));
            Asset.ThirdPartyPackagedAssetIdentifier = InAddressableId[0];
        }
        else
        {
            Asset.ThirdPartyPackagedAssetIdentifier = Dto.GetAddressableId();
            Asset.ThirdPartyPlatformType = EThirdPartyPlatform::NONE;
        }
    }
    else
    {
        Asset.ThirdPartyPackagedAssetIdentifier = "";
        Asset.ThirdPartyPlatformType = EThirdPartyPlatform::NONE;
    }

    if (Dto.HasUri())
    {
        Asset.Uri = Dto.GetUri();
    }

    if (Dto.HasChecksum())
    {
        Asset.Checksum = Dto.GetChecksum();
    }

    if (Dto.HasVersion())
    {
        Asset.Version = std::stoi(Dto.GetVersion().c_str());
    }

    if (Dto.HasMimeType())
    {
        Asset.MimeType = Dto.GetMimeType();
    }
}
} // namespace csp::systems

namespace csp::systems
{

Asset::Asset()
    : Type(EAssetType::MODEL)
    , Version(0)
    , ThirdPartyPackagedAssetIdentifier("")
    , ThirdPartyPlatformType(EThirdPartyPlatform::NONE)
{
}

const csp::common::String& FileAssetDataSource::GetMimeType() const { return MimeType; }

void FileAssetDataSource::SetMimeType(const csp::common::String& InMimeType) { MimeType = InMimeType; }

void FileAssetDataSource::SetUploadContent(
    csp::web::WebClient* InWebClient, csp::web::HttpPayload* InPayload, const csp::systems::Asset& InAsset) const
{
    assert(!FilePath.IsEmpty());

    auto Version = std::to_string(std::abs(InAsset.Version) /* Version should not be negative */);
    InWebClient->SetFileUploadContentFromFile(InPayload, FilePath, Version.c_str(), MimeType);
}

BufferAssetDataSource::BufferAssetDataSource()
    : Buffer(nullptr)
    , BufferLength(0)
{
}

const csp::common::String& BufferAssetDataSource::GetMimeType() const { return MimeType; }

void BufferAssetDataSource::SetMimeType(const csp::common::String& InMimeType) { MimeType = InMimeType; }

void BufferAssetDataSource::SetUploadContent(
    csp::web::WebClient* InWebClient, csp::web::HttpPayload* InPayload, const csp::systems::Asset& InAsset) const
{
    assert(Buffer != nullptr);
    assert(BufferLength > 0);
    assert(!InAsset.FileName.IsEmpty());

    auto Version = std::to_string(std::abs(InAsset.Version) /* Version should not be negative */);
    InWebClient->SetFileUploadContentFromBuffer(
        InPayload, reinterpret_cast<const char*>(Buffer), BufferLength, InAsset.FileName, Version.c_str(), MimeType);
}

Asset& AssetResult::GetAsset() { return Asset; }

const Asset& AssetResult::GetAsset() const { return Asset; }

void AssetResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* AssetDetailResponse = static_cast<chs::AssetDetailDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        AssetDetailResponse->FromJson(Response->GetPayload().GetContent());

        AssetDetailDtoToAsset(*AssetDetailResponse, Asset);
    }
}

csp::common::Array<Asset>& AssetsResult::GetAssets() { return Assets; }

const csp::common::Array<Asset>& AssetsResult::GetAssets() const { return Assets; }

void AssetsResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* AssetsResponse = static_cast<csp::services::DtoArray<chs::AssetDetailDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        AssetsResponse->FromJson(Response->GetPayload().GetContent());

        std::vector<chs::AssetDetailDto>& DetailsArray = AssetsResponse->GetArray();
        Assets = csp::common::Array<csp::systems::Asset>(DetailsArray.size());

        // Extract data from response in our Projects array
        for (size_t i = 0; i < DetailsArray.size(); ++i)
        {
            AssetDetailDtoToAsset(DetailsArray[i], Assets[i]);
        }
    }
}

csp::common::String& UriResult::GetUri() { return Uri; }

const csp::common::String& UriResult::GetUri() const { return Uri; }

void UriResult::SetUri(const csp::common::String& Value) { Uri = Value; }

UriResult::UriResult(const csp::common::String Uri)
    : Uri(Uri)
{
    SetResult(csp::systems::EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));
}

void UriResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    const auto* Response = ApiResponse->GetResponse();
    const auto& Headers = Response->GetPayload().GetHeaders();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        Uri = Response->GetPayload().GetContent();
    }
}

void UriResult::SetResponseBody(const csp::common::String& Contents) { ResponseBody = Contents; }

AssetDataResult::AssetDataResult(void*) { }

AssetDataResult::AssetDataResult(const AssetDataResult& Other)
    : ResultBase(Other)
{
}

AssetDataResult::~AssetDataResult() { }

const void* AssetDataResult::GetData() const { return ResponseBody.c_str(); }

size_t AssetDataResult::GetDataLength() const { return ResponseBody.Length(); }

void AssetDataResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse) { ResultBase::OnResponse(ApiResponse); }

} // namespace csp::systems
