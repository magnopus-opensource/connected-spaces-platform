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
#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/WebService.h"
#include "CSP/ThirdPartyPlatforms.h"

#include <functional>

namespace csp::web
{

struct IWebClient;
class HttpPayload;
class WebClient;

} // namespace csp::web

namespace csp::services
{

class ApiResponseBase;

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

/// @brief Asset type enum, defines the allowed and implemented types of assets.
enum class EAssetType
{
    IMAGE,
    THUMBNAIL,
    SIMULATION,
    MODEL,
    VIDEO,
    SCRIPT_LIBRARY,
    HOLOCAP_VIDEO,
    HOLOCAP_AUDIO,
    AUDIO,
    GAUSSIAN_SPLAT,
    MATERIAL
};

enum class EAssetPlatform
{
    DEFAULT
};

/// @brief Converts a received DTO type into a Connected Spaces Platform enum EAssetType.
/// @param DTOAssetDetailType : The string defining the asset type given via the DTO.
/// @returns the converted EAssetType value.
EAssetType ConvertDTOAssetDetailType(const csp::common::String& DTOAssetDetailType);

/// @brief Converts a string platform definition to an EAssetPlatform value.
EAssetPlatform ConvertStringToAssetPlatform(const csp::common::String& Platform);

/// @brief Converts the EAssetPlatform enum into a string value.
csp::common::String ConvertAssetPlatformToString(EAssetPlatform Platform);

/// @ingroup Asset System
/// @brief Data representation of an asset which maps to a PrototypeService::AssetDetail.
class CSP_API Asset
{
public:
    Asset();
    Asset(const Asset& Other) = default;
    csp::common::String AssetCollectionId;
    csp::common::String Id;
    csp::common::String FileName;
    csp::common::String Name;
    csp::common::String LanguageCode;
    EAssetType Type;
    csp::common::Array<EAssetPlatform> Platforms;
    csp::common::Array<csp::common::String> Styles;
    csp::common::String ExternalUri;
    /// @brief S3 blob URI for Download
    csp::common::String Uri;
    csp::common::String Checksum;
    int Version;
    csp::common::String MimeType;
    csp::common::String ExternalMimeType;
    csp::common::String ThirdPartyPackagedAssetIdentifier;
    EThirdPartyPlatform ThirdPartyPlatformType;
};

/// @brief Defines a base data source for an Asset, attributing a mime type and providing functionality for uploading the data.
CSP_INTERFACE class CSP_API AssetDataSource
{
public:
    /// @brief Gets the mime type of this data source
    /// @return returns a string representing the mime type set for this data source.
    virtual const csp::common::String& GetMimeType() const = 0;

    /// @brief Sets the mime type for this data source
    /// @param InMimeType The mime type to set.
    virtual void SetMimeType(const csp::common::String& InMimeType) = 0;

    CSP_NO_EXPORT virtual void SetUploadContent(
        csp::web::WebClient* InWebClient, csp::web::HttpPayload* InPayload, const csp::systems::Asset& InAsset) const
        = 0;

protected:
    virtual ~AssetDataSource() = default;
};

/// @ingroup Asset System
/// @brief A file based data source for Assets, handles uploading a file based on a file path.
class CSP_API FileAssetDataSource : public AssetDataSource
{
public:
    /** @name Data Values
     *
     *   @{ */
    csp::common::String FilePath;
    /** @} */

    /// @brief Gets the mime type of this data source
    /// @return returns a string representing the mime type set for this data source.
    const csp::common::String& GetMimeType() const override;

    /// @brief Sets the mime type for this data source
    /// @param InMimeType The mime type to set.
    void SetMimeType(const csp::common::String& InMimeType) override;

private:
    void SetUploadContent(csp::web::WebClient* InWebClient, csp::web::HttpPayload* InPayload, const csp::systems::Asset& InAsset) const override;

    csp::common::String MimeType = "application/octet-stream";
};

/// @ingroup Asset System
/// @brief A buffer based data source for Assets, handles uploading a file based on a given buffer.
class CSP_API BufferAssetDataSource : public AssetDataSource
{
public:
    BufferAssetDataSource();

    /** @name Data Values
     *
     *   @{ */
    void* Buffer;
    size_t BufferLength;
    /** @} */

    /// @brief Gets the mime type of this data source
    /// @return returns a string representing the mime type set for this data source.
    const csp::common::String& GetMimeType() const override;

    /// @brief Sets the mime type for this data source
    /// @param InMimeType The mime type to set.
    void SetMimeType(const csp::common::String& InMimeType) override;

private:
    void SetUploadContent(csp::web::WebClient* InWebClient, csp::web::HttpPayload* InPayload, const csp::systems::Asset& InAsset) const override;
    csp::common::String MimeType = "application/octet-stream";
};

/// @ingroup Asset System
/// @brief Data class used to contain information when creating an asset.
class CSP_API AssetResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the asset result.
    /// @return Asset : const ref of asset class.
    Asset& GetAsset();

    /// @brief Retrieves the asset result.
    /// @return Asset : const ref of asset class.
    const Asset& GetAsset() const;

protected:
    AssetResult() = delete;
    AssetResult(void*) {};

private:
    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    CSP_NO_EXPORT AssetResult(const csp::systems::ResultBase& InResult)
        : csp::systems::ResultBase(InResult.GetResultCode(), InResult.GetHttpResultCode()) {};

    Asset Asset;
};

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to get an array of assets.
class CSP_API AssetsResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the asset array being stored as a pointer.
    /// @return csp::common::Array<Asset> : pointer to asset array being stored.
    csp::common::Array<Asset>& GetAssets();

    /// @brief Retrieves the asset array being stored as a pointer.
    /// @return csp::common::Array<Asset> : pointer to asset array being stored.
    const csp::common::Array<Asset>& GetAssets() const;

    CSP_NO_EXPORT AssetsResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

protected:
    AssetsResult() = delete;
    AssetsResult(void*) {};

private:
    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<Asset> Assets;
};

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to upload an asset.
class CSP_API UriResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    friend class SpaceSystem;
    friend class SettingsSystem;
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the uri for the asset uploaded.
    /// @return csp::common::String : uri of the uploaded asset.
    csp::common::String& GetUri();

    /// @brief Retrieves the uri for the asset uploaded.
    /// @return csp::common::String : uri of the uploaded asset.
    const csp::common::String& GetUri() const;

    void SetUri(const csp::common::String& Value);

    CSP_NO_EXPORT UriResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

protected:
    UriResult() = delete;
    UriResult(void*) {};

private:
    UriResult(const csp::common::String Uri);
    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    void SetResponseBody(const csp::common::String& Contents);

    csp::common::String Uri;
};

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to download asset data.
class CSP_API AssetDataResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    AssetDataResult(const AssetDataResult& Other);
    ~AssetDataResult();

    /// @brief Retrieves the data from the result.
    const void* GetData() const;

    /// @brief Gets the length of data returned.
    size_t GetDataLength() const;

protected:
    AssetDataResult() = delete;
    AssetDataResult(void*);

private:
    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;
};

/// @brief Callback containing asset.
/// @param Result CreateAssetResult : result class
typedef std::function<void(const AssetResult& Result)> AssetResultCallback;

/// @brief Callback containing array of assets.
/// @param Result DeleteAssetResult : result class
typedef std::function<void(const AssetsResult& Result)> AssetsResultCallback;

/// @brief Callback containing asset data uri.
/// @param Result GetAssetsResult : result class
typedef std::function<void(const UriResult& Result)> UriResultCallback;

/// @brief Callback containing asset data.
/// @param Result GetAssetsResult : result class
typedef std::function<void(const AssetDataResult& Result)> AssetDataResultCallback;

} // namespace csp::systems
