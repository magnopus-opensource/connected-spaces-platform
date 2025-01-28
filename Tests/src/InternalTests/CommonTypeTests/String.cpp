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

#if !defined(SKIP_INTERNAL_TESTS) || defined(RUN_COMMONTYPE_TESTS) || defined(RUN_COMMONTYPE_STRING_TESTS)

#include "CSP/Common/String.h"
#include "CSP/Common/List.h"

#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace csp::common;

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringDefaultInitialisationTest)
{
    try
    {
        String Instance;

        EXPECT_TRUE(Instance.IsEmpty());
        EXPECT_EQ(Instance, "");
        EXPECT_NE(Instance.c_str(), nullptr);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferLengthInitialisationTest)
{
    try
    {
        const char Buffer[] = "abcdefg";
        auto Length = strlen(Buffer);

        String Instance(Buffer, Length);

        EXPECT_FALSE(Instance.IsEmpty());

        // String contents should be equal, but buffer pointer should not
        EXPECT_EQ(Instance.Length(), Length);
        EXPECT_EQ(Instance, Buffer);
        EXPECT_NE(Instance.c_str(), nullptr);
        EXPECT_NE(Instance.c_str(), &Buffer[0]);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferLengthNullptrInitialisationTest)
{
    try
    {
        String Instance(nullptr, 5);

        EXPECT_TRUE(Instance.IsEmpty());
        EXPECT_EQ(Instance.Length(), 0);
        EXPECT_NE(Instance.c_str(), nullptr);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringLengthInitialisationTest)
{
    constexpr size_t Length = 5;

    try
    {
        String Instance(Length);

        EXPECT_FALSE(Instance.IsEmpty());
        EXPECT_EQ(Instance.Length(), Length);
        EXPECT_NE(Instance.c_str(), nullptr);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringLengthZeroInitialisationTest)
{
    try
    {
        String Instance(0ULL);

        EXPECT_TRUE(Instance.IsEmpty());
        EXPECT_EQ(Instance.Length(), 0);
        EXPECT_NE(Instance.c_str(), nullptr);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferInitialisationTest)
{
    try
    {
        const char Buffer[] = "abcdefg";
        auto Length = strlen(Buffer);

        String Instance(Buffer);

        EXPECT_FALSE(Instance.IsEmpty());

        // String contents should be equal, but buffer pointer should not
        EXPECT_EQ(Instance.Length(), Length);
        EXPECT_EQ(Instance, Buffer);
        EXPECT_NE(Instance.c_str(), nullptr);
        EXPECT_NE(Instance.c_str(), &Buffer[0]);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferNullptrInitialisationTest)
{
    try
    {
        String Instance((char*)nullptr);

        EXPECT_TRUE(Instance.IsEmpty());
        EXPECT_EQ(Instance.Length(), 0);
        EXPECT_NE(Instance.c_str(), nullptr);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringCopyInitialisationTest)
{
    try
    {
        String OtherInstance = "abcdefg";
        String Instance(OtherInstance);

        EXPECT_FALSE(Instance.IsEmpty());

        // Strings should be equal but not point to the same buffer
        EXPECT_EQ(Instance.Length(), OtherInstance.Length());
        EXPECT_EQ(Instance, OtherInstance);
        EXPECT_NE(Instance.c_str(), OtherInstance.c_str());
        EXPECT_NE(Instance.c_str(), nullptr);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringCopyAssignmentTest)
{
    try
    {
        String OtherInstance = "abcdefg";
        String Instance;
        Instance = OtherInstance;

        EXPECT_FALSE(Instance.IsEmpty());

        // Strings should be equal but not point to the same buffer
        EXPECT_EQ(Instance.Length(), OtherInstance.Length());
        EXPECT_EQ(Instance, OtherInstance);
        EXPECT_NE(Instance.c_str(), nullptr);
        EXPECT_NE(Instance.c_str(), OtherInstance.c_str());
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferAssignmentTest)
{
    try
    {
        const char Buffer[] = "abcdefg";
        String Instance;
        Instance = Buffer;

        EXPECT_FALSE(Instance.IsEmpty());

        // String contents should be equal, but buffer pointer should not
        EXPECT_EQ(Instance.Length(), strlen(Buffer));
        EXPECT_EQ(Instance, Buffer);
        EXPECT_NE(Instance.c_str(), nullptr);
        EXPECT_NE(Instance.c_str(), &Buffer[0]);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringSplitTest)
{
    try
    {
        String Instance = "abc;;def;";
        auto Parts = Instance.Split(';');

        // String::Split should keep empty parts
        EXPECT_EQ(Parts.Size(), 4);
        EXPECT_EQ(Parts[0], "abc");
        EXPECT_EQ(Parts[1], "");
        EXPECT_EQ(Parts[2], "def");
        EXPECT_EQ(Parts[3], "");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringSwapTest)
{
    try
    {
        String OtherInstance = "abcdefg";
        String Instance = "gfecdba";
        Instance.swap(OtherInstance);

        EXPECT_EQ(Instance, "abcdefg");
        EXPECT_EQ(OtherInstance, "gfecdba");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringEqualityTest)
{
    try
    {
        String OtherInstance = "abcdefg";
        String Instance = "abcdefg";

        EXPECT_EQ(Instance, OtherInstance);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringNonEqualityTest)
{
    try
    {
        String OtherInstance = "abcdefg";
        String Instance = "abcdefh";

        EXPECT_NE(Instance, OtherInstance);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferEqualityTest)
{
    try
    {
        const char Buffer[] = "abcdefg";
        String Instance = "abcdefg";

        EXPECT_EQ(Instance, Buffer);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferNonEqualityTest)
{
    try
    {
        const char Buffer[] = "abcdefg";
        String Instance = "abcdefh";

        EXPECT_NE(Instance, Buffer);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringLessThanTest)
{
    try
    {
        // The less than operator is used for ordering of String instances
        String OtherInstance = "abcdefh";
        String Instance = "abcdefg";

        EXPECT_LT(Instance, OtherInstance);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAppendTest)
{
    try
    {
        String OtherInstance = "defg";
        String Instance = "abc";
        Instance.Append(OtherInstance);

        // The appended String instance should not be modified
        EXPECT_EQ(OtherInstance, "defg");
        EXPECT_EQ(Instance, "abcdefg");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAppendEmptyTest)
{
    try
    {
        String OtherInstance;
        String Instance = "abc";
        Instance.Append(OtherInstance);

        // The appended String instance should not be modified
        EXPECT_EQ(OtherInstance, "");
        EXPECT_EQ(Instance, "abc");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAppendBufferTest)
{
    try
    {
        const char Buffer[] = "defg";
        String Instance = "abc";
        Instance.Append(Buffer);

        EXPECT_EQ(Instance, "abcdefg");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAppendBufferNullptrTest)
{
    try
    {
        String Instance = "abc";
        Instance.Append((char*)nullptr);

        // Appending a null buffer should not throw
        EXPECT_EQ(Instance, "abc");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddTest)
{
    try
    {
        String Instance = "abc";
        String OtherInstance = "defg";
        String Combined = Instance + OtherInstance;

        // Neither of the original String instances should be modified
        EXPECT_EQ(Instance, "abc");
        EXPECT_EQ(OtherInstance, "defg");
        EXPECT_EQ(Combined, "abcdefg");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddEmptyTest)
{
    try
    {
        String Instance = "abc";
        String OtherInstance;
        String Combined = Instance + OtherInstance;

        // Neither of the original String instances should be modified and the result should not be the LHS String
        //  instance
        EXPECT_EQ(Instance, "abc");
        EXPECT_EQ(OtherInstance, "");
        EXPECT_EQ(Combined, "abc");
        EXPECT_NE(Instance.c_str(), Combined.c_str());
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddBufferTest)
{
    try
    {
        String Instance = "abc";
        String Combined = Instance + "defg";

        // The original String instance should not be modified
        EXPECT_EQ(Instance, "abc");
        EXPECT_EQ(Combined, "abcdefg");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddBufferNullptrTest)
{
    try
    {
        String Instance = "abc";
        String Combined = Instance + (char*)nullptr;

        // Adding a null buffer should not throw and the result should not be the original String instance
        EXPECT_EQ(Instance, "abc");
        EXPECT_EQ(Combined, "abc");
        EXPECT_NE(Instance.c_str(), Combined.c_str());
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddAssignmentTest)
{
    try
    {
        String OtherInstance = "defg";
        String Instance = "abc";
        Instance += OtherInstance;

        // The appended String instance should not be modified
        EXPECT_EQ(OtherInstance, "defg");
        EXPECT_EQ(Instance, "abcdefg");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddAssignmentEmptyTest)
{
    try
    {
        String OtherInstance;
        String Instance = "abc";
        Instance += OtherInstance;

        // The appended String instance should not be modified
        EXPECT_EQ(OtherInstance, "");
        EXPECT_EQ(Instance, "abc");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddAssignmentBufferTest)
{
    try
    {
        const char Buffer[] = "defg";
        String Instance = "abc";
        Instance += Buffer;

        EXPECT_EQ(Instance, "abcdefg");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddAssignmentBufferNullptrTest)
{
    try
    {
        String Instance = "abc";
        Instance += (char*)nullptr;

        // Appending a null buffer should not throw
        EXPECT_EQ(Instance, "abc");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringTrimTest)
{
    try
    {
        String Instance = " \rabc\t\n  ";
        String Trimmed = Instance.Trim();

        // The original String instance should not be modified
        EXPECT_EQ(Instance, " \rabc\t\n  ");
        EXPECT_EQ(Trimmed, "abc");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringTrimNoWhitespaceTest)
{
    try
    {
        String Instance = "abc";
        String Trimmed = Instance.Trim();

        // The original String buffer should not be the same as the trimmed String buffer
        EXPECT_EQ(Instance, "abc");
        EXPECT_EQ(Trimmed, "abc");
        EXPECT_NE(Instance.c_str(), Trimmed.c_str());
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringTrimAllWhitespaceTest)
{
    try
    {
        String Instance = "  \r\n\r\n\t";
        String Trimmed = Instance.Trim();

        // The original String should not be modified
        EXPECT_EQ(Instance, "  \r\n\r\n\t");
        EXPECT_EQ(Trimmed, "");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringToLowerTest)
{
    try
    {
        String Instance = "\nAbC! _76-WHAT-lol";
        String Transformed = Instance.ToLower();

        // The original String instance should not be modified
        EXPECT_EQ(Instance, "\nAbC! _76-WHAT-lol");
        EXPECT_EQ(Transformed, "\nabc! _76-what-lol");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringJoinListTest)
{
    try
    {
        List<String> Parts = { "abc", "def", "ghi" };
        String Instance = String::Join(Parts);

        EXPECT_EQ(Instance, "abcdefghi");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringJoinListEmptyTest)
{
    try
    {
        List<String> Parts(0);
        String Instance = String::Join(Parts);

        EXPECT_EQ(Instance, "");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringJoinListSomeEmptyEntriesTest)
{
    try
    {
        List<String> Parts = { "abc", String(), String(0ULL) };
        String Instance = String::Join(Parts);

        EXPECT_EQ(Instance, "abc");
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringJoinListAllEmptyEntriesTest)
{
    try
    {
        List<String> Parts = { "", String(), String(0ULL) };
        String Instance = String::Join(Parts);

        EXPECT_EQ(Instance, "");
    }
    catch (...)
    {
        FAIL();
    }
}

#endif