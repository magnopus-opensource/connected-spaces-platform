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
#include "CSP/Common/CancellationToken.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

namespace
{

#if RUN_ALL_UNIT_TESTS || RUN_CANCELLATION_TOKEN_TESTS || RUN_CANCELLATIONTOKEN_CONSTRUCTION_TEST
CSP_PUBLIC_TEST(CSPEngine, CancellationTokenTests, ConstructionAndDestructionTest) { EXPECT_NO_THROW(csp::common::CancellationToken()); }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CANCELLATION_TOKEN_TESTS || RUN_CANCELLATIONTOKEN_CANCEL_TEST
CSP_PUBLIC_TEST(CSPEngine, CancellationTokenTests, CancelStateTest)
{
    csp::common::CancellationToken CancellationToken;
    EXPECT_FALSE(CancellationToken.Cancelled());

    CancellationToken.Cancel();
    EXPECT_TRUE(CancellationToken.Cancelled());

    EXPECT_NO_THROW(CancellationToken.Cancel()); // Test that multiple cancellations don't affect the state
    EXPECT_TRUE(CancellationToken.Cancelled());
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CANCELLATION_TOKEN_TESTS || RUN_CANCELLATIONTOKEN_COPYMOVE_TEST
CSP_PUBLIC_TEST(CSPEngine, CancellationTokenTests, CopyMoveTest)
{
    // Ensure copy and move operations are deleted
    ASSERT_FALSE(std::is_copy_constructible_v<csp::common::CancellationToken>);
    ASSERT_FALSE(std::is_move_constructible_v<csp::common::CancellationToken>);
    ASSERT_FALSE(std::is_copy_assignable_v<csp::common::CancellationToken>);
    ASSERT_FALSE(std::is_move_assignable_v<csp::common::CancellationToken>);
}
#endif

} // namespace