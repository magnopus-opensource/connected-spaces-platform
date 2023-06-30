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
#include "CSP/Systems/GraphQL/GraphQLSystem.h"
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


#if RUN_ALL_UNIT_TESTS || RUN_GRAPHQLSYSTEM_TESTS || RUN_GRAPHQLSYSTEM_QUERY_TEST
CSP_PUBLIC_TEST(CSPEngine, GraphQLSystemTests, QueryTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto GraphQLSystem	 = SystemsManager.GetGraphQLSystem();
	auto UserSystem		 = SystemsManager.GetUserSystem();
	auto SpaceSystem	 = SystemsManager.GetSpaceSystem();

	csp::common::String UserId;
	csp::systems::Space Space;

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-MAGNOPUS";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-MAGNOPUS";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	// Log in
	LogIn(UserSystem, UserId);

	csp::common::String testQuery = "spaces(pagination:{limit:10,skip:0},filters:{discoverable:false,requiresInvite:true}){itemTotalCount "
									"items{groupId name discoverable requiresInvite createdAt}}";

	// Create Space
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	csp::common::String expectedResponse = UniqueSpaceName;

	auto [Result] = AWAIT_PRE(GraphQLSystem, RunQuery, RequestPredicate, testQuery);

	// Search Space Name
	EXPECT_NE(std::string(Result.GetResponse().c_str()).find(expectedResponse), std::string::npos);

	csp::common::String testQueryFull = "{\"query\":\"\n\nquery getSpaces($limit:Int!)  {\n  spaces(pagination: {limit:$limit}) {\n    items {\n     "
										" name\n    }\n  }\n}\n\n\",\"variables\":{\"limit\":5},\"operationName\":\"getSpaces\"}";

	// run full query
	auto [ResultFull] = AWAIT_PRE(GraphQLSystem, RunFullQuery, RequestPredicate, testQueryFull);

	// Search Space Name
	EXPECT_NE(std::string(ResultFull.GetResponse().c_str()).find(expectedResponse), std::string::npos);

	// Delete Space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log Out
	LogOut(UserSystem);
}
#endif