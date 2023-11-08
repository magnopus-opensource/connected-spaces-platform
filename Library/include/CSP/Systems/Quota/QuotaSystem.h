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

#include "CSP/Systems/Quota/Quota.h"
#include "CSP/Systems/SystemBase.h"


namespace csp::services
{

class ApiBase;

}


namespace csp::web
{

class WebClient;

}

namespace csp::systems
{
/// @ingroup GraphQL System
/// @brief Public facing system that allows interfacing with Magnopus Connect Services' Quota Server.
/// Offers methods for receiving Quota Queries.
class CSP_API CSP_NO_DISPOSE QuotaSystem : public SystemBase
{
	/** @cond DO_NOT_DOCUMENT */
	friend class SystemsManager;
	/** @endcond */

public:
	~QuotaSystem();

	/// @brief Get the total number of Spaces own by the current user and their tier Space limit
	/// @param Callback FeatureProgressCallback : callback when asynchronous task finishes
	CSP_ASYNC_RESULT void GetTotalSpacesOwnedByUser(FeatureLimitCallback Callback);

	/// @brief Gets total number of user inside of a space
	/// @param SpaceId csp::common::String : Id of the Space
	/// @param Callback FeatureProgressCallback : callback when asynchronous task finishes
	CSP_ASYNC_RESULT void GetConcurrentUsersInSpace(const csp::common::String& SpaceId, FeatureLimitCallback Callback);

	/// @brief Get total size of all assets within a space
	/// @param SpaceId csp::common::String : Id of the Space
	/// @param Callback FeatureProgressCallback : callback when asynchronous task finishes
	CSP_ASYNC_RESULT void GetTotalSpaceSizeinKilobytes(const csp::common::String& SpaceId, FeatureLimitCallback Callback);

	/// @brief Get Array of feature progress for a user
	/// @param FeatureNames csp::common::Array<TierFeatures> : Array of feature names that will be retrieved
	/// @param Callback FeatureProgressCallback : callback when asynchronous task finishes
	CSP_ASYNC_RESULT void GetTierFeatureProgressForUser(const csp::common::Array<TierFeatures>& FeatureNames, FeaturesLimitCallback Callback);

	/// @brief Get Array of feature progress for a user Space
	/// @param SpaceId csp::common::String : Id of the Space
	/// @param FeatureNames csp::common::Array<TierFeatures> : Array of feature names that will be retrieved
	/// @param Callback FeatureProgressCallback : callback when asynchronous task finishes
	CSP_ASYNC_RESULT void GetTierFeatureProgressForSpace(const csp::common::String& SpaceId,
														 const csp::common::Array<TierFeatures>& FeatureNames,
														 FeaturesLimitCallback Callback);

	/// @brief Get current users tier information
	/// @param Callback UserTierCallback : callback when asynchronous task finishes
	CSP_ASYNC_RESULT void GetCurrentUserTier(UserTierCallback Callback);

	/// @brief Get current feature quota information
	/// @param TierName TierNames : Name of the tier
	/// @param FeatureName TierFeatures : Name of the feature
	/// @param Callback UserTierCallback : callback when asynchronous task finishes
	CSP_ASYNC_RESULT void GetTierFeatureQuota(const TierNames& TierName, const TierFeatures& FeatureName, FeatureQuotaCallback Callback);

	/// @brief Get current array of current feature quota information inside a tier
	/// @param TierName TierNames : Name of the tier
	/// @param Callback UserTierCallback : callback when asynchronous task finishes
	CSP_ASYNC_RESULT void GetTierFeaturesQuota(const TierNames& TierName, FeaturesQuotaCallback Callback);

private:
	QuotaSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
	CSP_NO_EXPORT QuotaSystem(csp::web::WebClient* InWebClient);

	csp::services::ApiBase* QuotaTierAssignmentAPI;
	csp::services::ApiBase* QuotaManagementAPI;
	csp::services::ApiBase* QuotaActivityAPI;
};
} // namespace csp::systems
