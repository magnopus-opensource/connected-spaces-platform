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

#if !defined(SKIP_INTERNAL_TESTS) || defined(RUN_COMMONTYPE_TESTS) || defined(RUN_COMMONTYPE_STRINGFORMAT_TESTS)

#include "CSP/Common/StringFormat.h"
#include "CSP/Common/List.h"

#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace csp::common;

CSP_INTERNAL_TEST(CSPEngine, CommonStringFormatTests, StringFormatTest)
{
    try
    {
        String Instance = StringFormat("%c %d %i %.1f %s", '1', 2, 3, 4.0f, "five");

        EXPECT_FALSE(Instance.IsEmpty());
        EXPECT_NE(Instance.c_str(), nullptr);
        EXPECT_EQ(Instance, "1 2 3 4.0 five");
    }
    catch (...)
    {
        FAIL();
    }
}

#endif