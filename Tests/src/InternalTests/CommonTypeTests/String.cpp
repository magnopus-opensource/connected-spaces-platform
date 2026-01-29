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

#include "CSP/Common/String.h"
#include "CSP/Common/List.h"

#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace csp::common;

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringDefaultInitialisationTest)
{
    String Instance;

    EXPECT_TRUE(Instance.IsEmpty());
    EXPECT_EQ(Instance, "");
    EXPECT_NE(Instance.c_str(), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferLengthInitialisationTest)
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

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferLengthNullptrInitialisationTest)
{
    String Instance(nullptr, 5);

    EXPECT_TRUE(Instance.IsEmpty());
    EXPECT_EQ(Instance.Length(), 0);
    EXPECT_NE(Instance.c_str(), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringLengthInitialisationTest)
{
    constexpr size_t Length = 5;

    String Instance(Length);

    EXPECT_FALSE(Instance.IsEmpty());
    EXPECT_EQ(Instance.Length(), Length);
    EXPECT_NE(Instance.c_str(), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringLengthZeroInitialisationTest)
{
    String Instance(0ULL);

    EXPECT_TRUE(Instance.IsEmpty());
    EXPECT_EQ(Instance.Length(), 0);
    EXPECT_NE(Instance.c_str(), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferInitialisationTest)
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

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferNullptrInitialisationTest)
{
    String Instance((char*)nullptr);

    EXPECT_TRUE(Instance.IsEmpty());
    EXPECT_EQ(Instance.Length(), 0);
    EXPECT_NE(Instance.c_str(), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringCopyInitialisationTest)
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

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringCopyAssignmentTest)
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

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferAssignmentTest)
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

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringSplitTest)
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

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringSwapTest)
{
    String OtherInstance = "abcdefg";
    String Instance = "gfecdba";
    Instance.swap(OtherInstance);

    EXPECT_EQ(Instance, "abcdefg");
    EXPECT_EQ(OtherInstance, "gfecdba");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringEqualityTest)
{
    String OtherInstance = "abcdefg";
    String Instance = "abcdefg";

    EXPECT_EQ(Instance, OtherInstance);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringNonEqualityTest)
{
    String OtherInstance = "abcdefg";
    String Instance = "abcdefh";

    EXPECT_NE(Instance, OtherInstance);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferEqualityTest)
{
    const char Buffer[] = "abcdefg";
    String Instance = "abcdefg";

    EXPECT_EQ(Instance, Buffer);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferNonEqualityTest)
{
    const char Buffer[] = "abcdefg";
    String Instance = "abcdefh";

    EXPECT_NE(Instance, Buffer);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringLessThanTest)
{
    // The less than operator is used for ordering of String instances
    String OtherInstance = "abcdefh";
    String Instance = "abcdefg";

    EXPECT_LT(Instance, OtherInstance);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAppendTest)
{
    String OtherInstance = "defg";
    String Instance = "abc";
    Instance.Append(OtherInstance);

    // The appended String instance should not be modified
    EXPECT_EQ(OtherInstance, "defg");
    EXPECT_EQ(Instance, "abcdefg");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAppendEmptyTest)
{
    String OtherInstance;
    String Instance = "abc";
    Instance.Append(OtherInstance);

    // The appended String instance should not be modified
    EXPECT_EQ(OtherInstance, "");
    EXPECT_EQ(Instance, "abc");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAppendBufferTest)
{
    const char Buffer[] = "defg";
    String Instance = "abc";
    Instance.Append(Buffer);

    EXPECT_EQ(Instance, "abcdefg");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAppendBufferNullptrTest)
{
    String Instance = "abc";
    Instance.Append((char*)nullptr);

    // Appending a null buffer should not throw
    EXPECT_EQ(Instance, "abc");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddTest)
{
    String Instance = "abc";
    String OtherInstance = "defg";
    String Combined = Instance + OtherInstance;

    // Neither of the original String instances should be modified
    EXPECT_EQ(Instance, "abc");
    EXPECT_EQ(OtherInstance, "defg");
    EXPECT_EQ(Combined, "abcdefg");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddEmptyTest)
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

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddBufferTest)
{
    String Instance = "abc";
    String Combined = Instance + "defg";

    // The original String instance should not be modified
    EXPECT_EQ(Instance, "abc");
    EXPECT_EQ(Combined, "abcdefg");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddBufferNullptrTest)
{
    String Instance = "abc";
    String Combined = Instance + (char*)nullptr;

    // Adding a null buffer should not throw and the result should not be the original String instance
    EXPECT_EQ(Instance, "abc");
    EXPECT_EQ(Combined, "abc");
    EXPECT_NE(Instance.c_str(), Combined.c_str());
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddAssignmentTest)
{
    String OtherInstance = "defg";
    String Instance = "abc";
    Instance += OtherInstance;

    // The appended String instance should not be modified
    EXPECT_EQ(OtherInstance, "defg");
    EXPECT_EQ(Instance, "abcdefg");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddAssignmentEmptyTest)
{
    String OtherInstance;
    String Instance = "abc";
    Instance += OtherInstance;

    // The appended String instance should not be modified
    EXPECT_EQ(OtherInstance, "");
    EXPECT_EQ(Instance, "abc");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddAssignmentBufferTest)
{
    const char Buffer[] = "defg";
    String Instance = "abc";
    Instance += Buffer;

    EXPECT_EQ(Instance, "abcdefg");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddAssignmentBufferNullptrTest)
{
    String Instance = "abc";
    Instance += (char*)nullptr;

    // Appending a null buffer should not throw
    EXPECT_EQ(Instance, "abc");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringTrimTest)
{
    String Instance = " \rabc\t\n  ";
    String Trimmed = Instance.Trim();

    // The original String instance should not be modified
    EXPECT_EQ(Instance, " \rabc\t\n  ");
    EXPECT_EQ(Trimmed, "abc");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringTrimNoWhitespaceTest)
{
    String Instance = "abc";
    String Trimmed = Instance.Trim();

    // The original String buffer should not be the same as the trimmed String buffer
    EXPECT_EQ(Instance, "abc");
    EXPECT_EQ(Trimmed, "abc");
    EXPECT_NE(Instance.c_str(), Trimmed.c_str());
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringTrimAllWhitespaceTest)
{
    String Instance = "  \r\n\r\n\t";
    String Trimmed = Instance.Trim();

    // The original String should not be modified
    EXPECT_EQ(Instance, "  \r\n\r\n\t");
    EXPECT_EQ(Trimmed, "");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringToLowerTest)
{
    String Instance = "\nAbC! _76-WHAT-lol";
    String Transformed = Instance.ToLower();

    // The original String instance should not be modified
    EXPECT_EQ(Instance, "\nAbC! _76-WHAT-lol");
    EXPECT_EQ(Transformed, "\nabc! _76-what-lol");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringJoinListTest)
{
    List<String> Parts = { "abc", "def", "ghi" };
    String Instance = String::Join(Parts);

    EXPECT_EQ(Instance, "abcdefghi");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringJoinListEmptyTest)
{
    List<String> Parts(0);
    String Instance = String::Join(Parts);

    EXPECT_EQ(Instance, "");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringJoinListSomeEmptyEntriesTest)
{
    List<String> Parts = { "abc", String(), String(0ULL) };
    String Instance = String::Join(Parts);

    EXPECT_EQ(Instance, "abc");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringJoinListAllEmptyEntriesTest)
{
    List<String> Parts = { "", String(), String(0ULL) };
    String Instance = String::Join(Parts);

    EXPECT_EQ(Instance, "");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringContainsTest)
{
    String Instance = "abc_def_ghi_jkl";
    String Substring = "def_g";

    EXPECT_TRUE(Instance.Contains(Substring));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringContainsSubstringNotFoundTest)
{
    String Instance = "abc_def_ghi_jkl";
    String Substring = "xyz";

    EXPECT_FALSE(Instance.Contains(Substring));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringContainsSubstringEmptyTest)
{
    String Instance = "abc_def_ghi_jkl";
    String Substring = "";

    EXPECT_FALSE(Instance.Contains(Substring));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringContainsSubstringTooLongTest)
{
    String Instance = "abc_def_ghi_jkl";
    String Substring = "abc_def_ghi_jkl_mno";

    EXPECT_FALSE(Instance.StartsWith(Substring));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringStartsWithTest)
{
    String Instance = "Hello_World";
    String Prefix = "Hello";

    EXPECT_TRUE(Instance.StartsWith(Prefix));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringStartsWithEmptyPrefixTest)
{
    String Instance = "Hello_World";
    String Prefix = "";

    EXPECT_FALSE(Instance.StartsWith(Prefix));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringStartsWithPrefixTooLongTest)
{
    String Instance = "Hello_World";
    String Prefix = "Hello_Worldy";

    EXPECT_FALSE(Instance.StartsWith(Prefix));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringEndsWithTest)
{
    String Instance = "Hello_World";
    String Postfix = "World";

    EXPECT_TRUE(Instance.EndsWith(Postfix));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringEndsWithEmptyPostfixTest)
{
    String Instance = "Hello_World";
    String Postfix = "";

    EXPECT_FALSE(Instance.EndsWith(Postfix));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringEndsWithPostfixTooLongTest)
{
    String Instance = "Hello_World";
    String Postfix = "Hello_Worldy";

    EXPECT_FALSE(Instance.EndsWith(Postfix));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringSubStringTest)
{
    String Instance = "Believe you can and you're halfway there.";
    size_t Offset = 8;
    size_t Length = 7;

    EXPECT_EQ(Instance.SubString(Offset, Length), "you can");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringSubStringInvalidOffsetTest)
{
    String Instance = "Believe you can and you're halfway there.";
    size_t Offset = Instance.Length() + 1;

    EXPECT_EQ(Instance.SubString(Offset), "");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringSubStringNoLengthTest)
{
    String Instance = "Believe you can and you're halfway there.";
    size_t Offset = 8;

    EXPECT_EQ(Instance.SubString(Offset), "you can and you're halfway there.");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringSubStringInvalidLengthTest)
{
    String Instance = "Believe you can and you're halfway there.";
    size_t Offset = 8;
    size_t Length = Instance.Length() + 1;

    EXPECT_EQ(Instance.SubString(Offset, Length), "you can and you're halfway there.");
}
