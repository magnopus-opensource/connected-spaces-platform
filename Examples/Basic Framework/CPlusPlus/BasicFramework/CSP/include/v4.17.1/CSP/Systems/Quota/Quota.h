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
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/WebService.h"
#include "CSP/Web/HTTPResponseCodes.h"

namespace csp::services
{

class ApiResponseBase;

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{
/// @ingroup Quota System
/// @brief Data representation period of time.
enum class PeriodEnum
{
    Total = 0,
    CalendarMonth,
    Hours24,
    Invalid
};

enum class TierFeatures
{
    SpaceOwner = 0,
    ScopeConcurrentUsers,
    ObjectCaptureUpload,
    AudioVideoUpload,
    TotalUploadSizeInKilobytes,
    Agora,
    OpenAI,
    Shopify,
    TicketedSpace,
    Invalid
};

enum class TierNames
{
    Basic = 0,
    Premium,
    Pro,
    Enterprise,
    Invalid
};

/// @ingroup Quota System
/// @brief Data representation of a progress of a specific feature.
/// Limit Value of -1 means unlimited usage
class CSP_API FeatureLimitInfo
{
public:
    FeatureLimitInfo()
        : ActivityCount(0)
        , Limit(-1) {};

    TierFeatures FeatureName;
    int32_t ActivityCount;
    int32_t Limit;
};

/// @ingroup Quota System
/// @brief Data representation of a progress of a specific feature.
class CSP_API UserTierInfo
{
public:
    UserTierInfo() = default;

    csp::common::String AssignToType;
    csp::common::String AssignToId;
    TierNames TierName;
};

/// @ingroup Quota System
/// @brief Data representation of a progress of a specific feature.
/// Limit Value of -1 means unlimited usage
class CSP_API FeatureQuotaInfo
{
public:
    FeatureQuotaInfo()
        : Limit(-1) {};
    CSP_NO_EXPORT FeatureQuotaInfo(TierFeatures FeatureNameIn, TierNames TierNameIn, int32_t LimitIn, PeriodEnum PeriodIn, bool AllowReductionsIn);
    TierFeatures FeatureName;
    TierNames TierName;
    int32_t Limit;
    PeriodEnum Period;
};

/// @ingroup Quota System
/// @brief Data class used to contain information when receiving an array of feature progress.
class CSP_API FeaturesLimitResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the feature progress result.
    /// @return csp::common::Array<FeatureProgress> : const array of feature progress class
    const csp::common::Array<FeatureLimitInfo>& GetFeaturesLimitInfo() const;

private:
    FeaturesLimitResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<FeatureLimitInfo> FeaturesLimitInfo;
};

/// @ingroup Quota System
/// @brief Data class used to contain information when receiving an array of feature progresses.
class CSP_API FeatureLimitResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the feature progress result.
    /// @returnFeatureProgress : const ref to feature progress class
    const FeatureLimitInfo& GetFeatureLimitInfo() const;

private:
    FeatureLimitResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    FeatureLimitInfo FeatureLimitInfo;
};

/// @ingroup Quota System
/// @brief Data class used to contain information when receiving user tier information.
class CSP_API UserTierResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the user tier result.
    /// @returnFeatureProgress : const ref to user tier information class
    const UserTierInfo& GetUserTierInfo() const;

private:
    UserTierResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    UserTierInfo UserTierInfo;
};

/// @ingroup Quota System
/// @brief Data class used to contain information when receiving feature quota information.
class CSP_API FeatureQuotaResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the feature quota result.
    /// @returnFeatureProgress : const ref to feature quota class
    const FeatureQuotaInfo& GetFeatureQuotaInfo() const;

private:
    FeatureQuotaResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    FeatureQuotaInfo FeatureQuotaInfo;
};

/// @ingroup Quota System
/// @brief Data class used to contain information when receiving an array of feature quota information.
class CSP_API FeaturesQuotaResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves an array of feature quota results.
    /// @returnFeatureProgress : const ref to feature quota class
    const csp::common::Array<FeatureQuotaInfo>& GetFeaturesQuotaInfo() const;

private:
    FeaturesQuotaResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<FeatureQuotaInfo> FeaturesQuotaInfo;
};

/// @brief Retrieves an array of feature quota results.
/// @param Value TierNames : enum value to be converted to string
/// @csp::common::String : Const enum Value as a string
const csp::common::String TierNameEnumToString(const TierNames& Value);

/// @brief Retrieves an array of feature quota results.
/// @param Value TierFeatures : enum value to be converted to string
/// @csp::common::String : Const enum Value as a string
const csp::common::String TierFeatureEnumToString(const TierFeatures& Value);

/// @brief Retrieves an array of feature quota results.
/// @param Value csp::common::String : EnumValue as a string
/// @TierNames : Const string as an enum value
const TierNames StringToTierNameEnum(const csp::common::String& Value);

/// @brief Retrieves an array of feature quota results.
/// @param Value csp::common::String : EnumValue as a string
/// @TierFeatures : Const string as an enum value
const TierFeatures StringToTierFeatureEnum(const csp::common::String& Value);

/// @brief Callback containing array of feature progress.
/// @param Result FeatureProgressResult : result class
typedef std::function<void(const FeaturesLimitResult& Result)> FeaturesLimitCallback;

/// @brief Callback containing feature progress.
/// @param Result FeatureProgressResult : result class
typedef std::function<void(const FeatureLimitResult& Result)> FeatureLimitCallback;

/// @brief Callback containing User Tier Information.
/// @param Result FeatureProgressResult : result class
typedef std::function<void(const UserTierResult& Result)> UserTierCallback;

/// @brief Callback containing Tier Feature Quota Information.
/// @param Result FeatureProgressResult : result class
typedef std::function<void(const FeatureQuotaResult& Result)> FeatureQuotaCallback;

/// @brief Callback containing Tier Features Quota Information.
/// @param Result FeatureProgressResult : result class
typedef std::function<void(const FeaturesQuotaResult& Result)> FeaturesQuotaCallback;
} // namespace csp::systems
