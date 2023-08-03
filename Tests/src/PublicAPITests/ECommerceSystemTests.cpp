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
#include "CSP/Systems/ECommerce/ECommerceSystem.h"
#include "CSP/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>

using namespace csp::systems;

csp::common::Map<csp::common::String, csp::common::String> GetShopifyDetails()
{
	if (!std::filesystem::exists("ShopifyDetails.txt"))
	{
		LogFatal("ShopifyDetails.txt not found! This file must exist and must contain the following information:\nSpaceId "
				 "<SpaceId>\nProductId <ProductId>");
	}

	csp::common::Map<csp::common::String, csp::common::String> OutMap;

	std::ifstream CredsFile;
	CredsFile.open("ShopifyDetails.txt");
	std::string Key, Value;
	while (CredsFile >> Key >> Value)
	{
		OutMap[Key.c_str()] = Value.c_str();
	}

	return OutMap;
}

#if RUN_ECOMMERCE_TESTS || RUN_ECOMMERCE_GET_PROPERTY_INFORMATION_TEST
CSP_PUBLIC_TEST(CSPEngine, ECommerceSystemTests, GetProductInformationTest)
{
	SetRandSeed();

	auto& SystemsManager  = csp::systems::SystemsManager::Get();
	auto* UserSystem	  = SystemsManager.GetUserSystem();
	auto* ECommerceSystem = SystemsManager.GetECommerceSystem();

	// To use this test please follow end to end testing steps first:
	// https://docs.google.com/document/d/1D2fzF88c4NfPp26ciJHf-qelNFY5jqQHmt8lqolOmq0/edit?usp=sharing

	// These variables much be change to products values
	const char* TestProductDescription = "Gift Card";
	const char* TestSpaceDescription   = "This is a gift card for the store";

	csp::common::String UserId;
	LogIn(UserSystem, UserId);
	auto Details = GetShopifyDetails();

	auto [Result] = AWAIT(ECommerceSystem, GetProductInformation, Details["SpaceId"], Details["ProductId"]);
	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	LogOut(UserSystem);
}
#endif