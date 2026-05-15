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

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, GraphQLSystemTests, QueryTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto graphQlSystem = systemsManager.GetGraphQLSystem();
    auto userSystem = systemsManager.GetUserSystem();
    auto spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::String userId;
    csp::systems::Space space;

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAGNOPUS";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAGNOPUS";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    csp::common::String testQuery = "spaces(pagination:{limit:10,skip:0},filters:{discoverable:false,requiresInvite:true}){itemTotalCount "
                                    "items{groupId name discoverable requiresInvite createdAt}}";

    // Create Space
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    csp::common::String expectedResponse = uniqueSpaceName;

    auto [Result] = AWAIT_PRE(graphQlSystem, RunQuery, RequestPredicate, testQuery);

    // Search Space Name
    EXPECT_NE(std::string(Result.GetResponse().c_str()).find(expectedResponse), std::string::npos);

    csp::common::String testQueryFull = "{\"query\":\"\n\nquery getSpaces($limit:Int!)  {\n  spaces(pagination: {limit:$limit}) {\n    items {\n     "
                                        " name\n    }\n  }\n}\n\n\",\"variables\":{\"limit\":5},\"operationName\":\"getSpaces\"}";

    // run full query
    auto [ResultFull] = AWAIT_PRE(graphQlSystem, RunRequest, RequestPredicate, testQueryFull);

    // Search Space Name
    EXPECT_NE(std::string(ResultFull.GetResponse().c_str()).find(expectedResponse), std::string::npos);

    // Delete Space
    DeleteSpace(spaceSystem, space.Id);

    // Log Out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, GraphQLSystemTests, RunQueryBadInputTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto graphQlSystem = systemsManager.GetGraphQLSystem();
    auto userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::common::String testQuery = "badQuery";
    auto [Result] = AWAIT_PRE(graphQlSystem, RunQuery, RequestPredicate, testQuery);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, GraphQLSystemTests, RunRequestBadInputTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto graphQlSystem = systemsManager.GetGraphQLSystem();
    auto userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::common::String testQuery = "badRequest";
    auto [Result] = AWAIT_PRE(graphQlSystem, RunRequest, RequestPredicate, testQuery);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

    LogOut(userSystem);
}