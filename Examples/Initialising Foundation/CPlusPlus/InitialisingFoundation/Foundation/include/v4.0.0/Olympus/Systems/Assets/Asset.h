#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Services/WebService.h"
#include "Olympus/ThirdPartyPlatforms.h"

#include <functional>

namespace oly_web
{

struct IWebClient;
class HttpPayload;
class WebClient;

} // namespace oly_web

namespace oly_services
{

class ApiResponseBase;

OLY_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
OLY_END_IGNORE

} // namespace oly_services

namespace oly_systems
{

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
    AUDIO
};

enum class EAssetPlatform
{
    DEFAULT
};

EAssetType ConvertDTOAssetDetailType(const oly_common::String& DTOAssetDetailType);
EAssetPlatform ConvertStringToAssetPlatform(const oly_common::String& Platform);

oly_common::String ConvertAssetPlatformToString(EAssetPlatform Platform);

/// @ingroup Asset System
/// @brief Data representation of an asset which maps to a PrototypeService::AssetDetail.
class OLY_API Asset
{
public:
    Asset();
    Asset(const Asset& Other) = default;
    oly_common::String AssetCollectionId;
    oly_common::String Id;
    oly_common::String FileName;
    oly_common::String Name;
    oly_common::String LanguageCode;
    EAssetType Type;
    oly_common::Array<EAssetPlatform> Platforms;
    oly_common::Array<oly_common::String> Styles;
    oly_common::String ExternalUri;
    /// @brief S3 blob URI for Download
    oly_common::String Uri;
    oly_common::String Checksum;
    int Version;
    oly_common::String MimeType;
    oly_common::String ExternalMimeType;

    /**
     * @brief Get the party packaged asset identifier of this asset
     * @return returns a string representing the party packaged asset identifier set for this asset.
     */
    const oly_common::String& GetThirdPartyPackagedAssetIdentifier() const;
    /**
     * @brief Set the party packaged asset identifier for this asset
     * @param InThirdPartyPackagedAssetIdentifier The third party packaged asset identifier to set.
     */
    void SetThirdPartyPackagedAssetIdentifier(const oly_common::String& InThirdPartyPackagedAssetIdentifier);

    /**
     * @brief Get the third party platform type of this asset
     * @return returns a string representing third party platform type set for this asset.
     */
    const EThirdPartyPlatform GetThirdPartyPlatformType() const;
    /**
     * @brief Set third party platform type for this asset
     * @param InThirdPartyPlatformType The third party platform type to set.
     */
    void SetThirdPartyPlatformType(const EThirdPartyPlatform InThirdPartyPlatformType);

private:
    oly_common::String ThirdPartyPackagedAssetIdentifier;
    EThirdPartyPlatform ThirdPartyPlatform;
};

OLY_INTERFACE class OLY_API AssetDataSource
{
public:
    /**
     * @brief Get the mime type of this data source
     * @return returns a string representing the mime type set for this data source.
     */
    virtual const oly_common::String& GetMimeType() const = 0;
    /**
     * @brief Set the mime type for this data source
     * @param InMimeType The mime type to set.
     */
    virtual void SetMimeType(const oly_common::String& InMimeType) = 0;

    OLY_NO_EXPORT virtual void SetUploadContent(
        oly_web::WebClient* InWebClient, oly_web::HttpPayload* InPayload, const oly_systems::Asset& InAsset) const
        = 0;

protected:
    virtual ~AssetDataSource() = default;
};

/// @ingroup Asset System
/// @brief Asset source file.
class OLY_API FileAssetDataSource : public AssetDataSource
{
public:
    /** @name Data Values
     *
     *   @{ */
    oly_common::String FilePath;
    /** @} */

    /**
     * @brief Get the mime type of this data source
     * @return returns a string representing the mime type set for this data source.
     */
    const oly_common::String& GetMimeType() const override;
    /**
     * @brief Set the mime type for this data source
     * @param InMimeType The mime type to set.
     */
    void SetMimeType(const oly_common::String& InMimeType) override;

private:
    void SetUploadContent(oly_web::WebClient* InWebClient, oly_web::HttpPayload* InPayload, const oly_systems::Asset& InAsset) const override;

    oly_common::String MimeType = "application/octet-stream";
};

/// @ingroup Asset System
/// @brief Asset source from memory buffer.
class OLY_API BufferAssetDataSource : public AssetDataSource
{
public:
    BufferAssetDataSource();

    /** @name Data Values
     *
     *   @{ */
    void* Buffer;
    size_t BufferLength;
    /** @} */

    /**
     * @brief Get the mime type of this data source
     * @return returns a string representing the mime type set for this data source.
     */
    const oly_common::String& GetMimeType() const override;
    /**
     * @brief Set the mime type for this data source
     * @param InMimeType The mime type to set.
     */
    void SetMimeType(const oly_common::String& InMimeType) override;

private:
    void SetUploadContent(oly_web::WebClient* InWebClient, oly_web::HttpPayload* InPayload, const oly_systems::Asset& InAsset) const override;
    oly_common::String MimeType = "application/octet-stream";
};

/// @ingroup Asset System
/// @brief Data class used to contain information when creating an asset.
class OLY_API AssetResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the asset result.
    /// @return Asset : const ref of asset class
    Asset& GetAsset();

    /// @brief Retrieves the asset result.
    /// @return Asset : const ref of asset class
    const Asset& GetAsset() const;

protected:
    AssetResult() = delete;
    AssetResult(void*) {};

private:
    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    OLY_NO_EXPORT AssetResult(const oly_services::ResultBase& InResult)
        : oly_services::ResultBase(InResult.GetResultCode(), InResult.GetHttpResultCode()) {};

    Asset Asset;
};

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to get an array of assets.
class OLY_API AssetsResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Creates an invalid AssetsResult instance that can be used to notify the user of an error.
    /// @return AssetsResult : invalid AssetsResult instance
    OLY_NO_EXPORT static AssetsResult Invalid();

    /// @brief Retrieves the asset array being stored as a pointer.
    /// @return oly_common::Array<Asset> : pointer to asset array being stored
    oly_common::Array<Asset>& GetAssets();

    /// @brief Retrieves the asset array being stored as a pointer.
    /// @return oly_common::Array<Asset> : pointer to asset array being stored
    const oly_common::Array<Asset>& GetAssets() const;

protected:
    AssetsResult() = delete;
    AssetsResult(void*) {};
    AssetsResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};

private:
    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    oly_common::Array<Asset> Assets;
};

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to upload an asset.
class OLY_API UriResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    friend class SpaceSystem;
    friend class SettingsSystem;
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Creates an invalid UriResult instance that can be used to notify the user of an error.
    /// @return UriResult : invalid UriResult instance
    OLY_NO_EXPORT static UriResult Invalid();

    /// @brief Retrieves the uri for the asset uploaded.
    /// @return oly_common::String : uri of the uploaded asset
    oly_common::String& GetUri();

    /// @brief Retrieves the uri for the asset uploaded.
    /// @return oly_common::String : uri of the uploaded asset
    const oly_common::String& GetUri() const;

protected:
    UriResult() = delete;
    UriResult(void*) {};

private:
    UriResult(const oly_common::String Uri);
    UriResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};
    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    oly_common::String Uri;
};

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to download asset data.
class OLY_API AssetDataResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    AssetDataResult(const AssetDataResult& Other);
    ~AssetDataResult();

    void* GetData();
    const void* GetData() const;

    size_t GetDataLength();
    const size_t GetDataLength() const;

protected:
    AssetDataResult() = delete;
    AssetDataResult(void*);

private:
    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    void* Data;
    size_t DataLength;
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

} // namespace oly_systems
