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

#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Systems/Quota/QuotaSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
using namespace csp::systems;


namespace
{

bool RequestPredicate(const csp::services::ResultBase& Result)
{
	return Result.GetResultCode() != csp::services::EResultCode::InProgress;
}

} // namespace

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_QUERY_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, TierNameEnumTesttoStringTest)
{
	EXPECT_EQ(TierNameEnumToString(csp::systems::TierNames::Basic), "Basic");
	EXPECT_EQ(TierNameEnumToString(csp::systems::TierNames::Premium), "Premium");
	EXPECT_EQ(TierNameEnumToString(csp::systems::TierNames::Pro), "Pro");
	EXPECT_EQ(TierNameEnumToString(csp::systems::TierNames::Enterprise), "Enterprise");
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_QUERY_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, StringToTierNameEnumTestTest)
{
	EXPECT_EQ(StringToTierNameEnum("Basic"), csp::systems::TierNames::Basic);
	EXPECT_EQ(StringToTierNameEnum("Premium"), csp::systems::TierNames::Premium);
	EXPECT_EQ(StringToTierNameEnum("Pro"), csp::systems::TierNames::Pro);
	EXPECT_EQ(StringToTierNameEnum("Enterprise"), csp::systems::TierNames::Enterprise);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_QUERY_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, TierFeatureEnumTesttoStringTest)
{
	EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::Agora), "Agora");
	EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::AudioVideoUpload), "AudioVideoUpload");
	EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::ObjectCaptureUpload), "ObjectCaptureUpload");
	EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::OpenAI), "OpenAI");
	EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::ScopeConcurrentUsers), "ScopeConcurrentUsers");
	EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::Shopify), "Shopify");
	EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::SpaceOwner), "SpaceOwner");
	EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::TicketedSpace), "TicketedSpace");
	EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::TotalUploadSizeInKilobytes), "TotalUploadSizeInKilobytes");
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_QUERY_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, StringToTierFeatureEnumTestTest)
{
	EXPECT_EQ(StringToTierFeatureEnum("Agora"), csp::systems::TierFeatures::Agora);
	EXPECT_EQ(StringToTierFeatureEnum("AudioVideoUpload"), csp::systems::TierFeatures::AudioVideoUpload);
	EXPECT_EQ(StringToTierFeatureEnum("ObjectCaptureUpload"), csp::systems::TierFeatures::ObjectCaptureUpload);
	EXPECT_EQ(StringToTierFeatureEnum("OpenAI"), csp::systems::TierFeatures::OpenAI);
	EXPECT_EQ(StringToTierFeatureEnum("ScopeConcurrentUsers"), csp::systems::TierFeatures::ScopeConcurrentUsers);
	EXPECT_EQ(StringToTierFeatureEnum("Shopify"), csp::systems::TierFeatures::Shopify);
	EXPECT_EQ(StringToTierFeatureEnum("SpaceOwner"), csp::systems::TierFeatures::SpaceOwner);
	EXPECT_EQ(StringToTierFeatureEnum("TicketedSpace"), csp::systems::TierFeatures::TicketedSpace);
	EXPECT_EQ(StringToTierFeatureEnum("TotalUploadSizeInKilobytes"), csp::systems::TierFeatures::TotalUploadSizeInKilobytes);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_QUERY_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetTotalSpacesOwnedByUserTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto UserSystem		 = SystemsManager.GetUserSystem();
	auto QuotaSystem	 = SystemsManager.GetQuotaSystem();

	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	auto [Result] = AWAIT_PRE(QuotaSystem, GetTotalSpacesOwnedByUser, RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);
	EXPECT_EQ(Result.GetFeatureLimitInfo().ActivityCount, 0);
	EXPECT_EQ(Result.GetFeatureLimitInfo().Limit, 0);
	EXPECT_EQ(Result.GetFeatureLimitInfo().FeatureName, csp::systems::TierFeatures::SpaceOwner);
}
#endif
