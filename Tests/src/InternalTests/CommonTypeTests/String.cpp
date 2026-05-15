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
    String instance;

    EXPECT_TRUE(instance.IsEmpty());
    EXPECT_EQ(instance, "");
    EXPECT_NE(instance.c_str(), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferLengthInitialisationTest)
{
    const char buffer[] = "abcdefg";
    auto length = strlen(buffer);

    String instance(buffer, length);

    EXPECT_FALSE(instance.IsEmpty());

    // String contents should be equal, but buffer pointer should not
    EXPECT_EQ(instance.Length(), length);
    EXPECT_EQ(instance, buffer);
    EXPECT_NE(instance.c_str(), nullptr);
    EXPECT_NE(instance.c_str(), &buffer[0]);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferLengthNullptrInitialisationTest)
{
    String instance(nullptr, 5);

    EXPECT_TRUE(instance.IsEmpty());
    EXPECT_EQ(instance.Length(), 0);
    EXPECT_NE(instance.c_str(), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringLengthInitialisationTest)
{
    constexpr size_t length = 5;

    String instance(length);

    EXPECT_FALSE(instance.IsEmpty());
    EXPECT_EQ(instance.Length(), length);
    EXPECT_NE(instance.c_str(), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringLengthZeroInitialisationTest)
{
    String instance(static_cast<size_t>(0ULL));

    EXPECT_TRUE(instance.IsEmpty());
    EXPECT_EQ(instance.Length(), 0);
    EXPECT_NE(instance.c_str(), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferInitialisationTest)
{
    const char buffer[] = "abcdefg";
    auto length = strlen(buffer);

    String instance(buffer);

    EXPECT_FALSE(instance.IsEmpty());

    // String contents should be equal, but buffer pointer should not
    EXPECT_EQ(instance.Length(), length);
    EXPECT_EQ(instance, buffer);
    EXPECT_NE(instance.c_str(), nullptr);
    EXPECT_NE(instance.c_str(), &buffer[0]);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferNullptrInitialisationTest)
{
    String instance((char*)nullptr);

    EXPECT_TRUE(instance.IsEmpty());
    EXPECT_EQ(instance.Length(), 0);
    EXPECT_NE(instance.c_str(), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringCopyInitialisationTest)
{
    String otherInstance = "abcdefg";
    String instance(otherInstance);

    EXPECT_FALSE(instance.IsEmpty());

    // Strings should be equal but not point to the same buffer
    EXPECT_EQ(instance.Length(), otherInstance.Length());
    EXPECT_EQ(instance, otherInstance);
    EXPECT_NE(instance.c_str(), otherInstance.c_str());
    EXPECT_NE(instance.c_str(), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringCopyAssignmentTest)
{
    String otherInstance = "abcdefg";
    String instance;
    instance = otherInstance;

    EXPECT_FALSE(instance.IsEmpty());

    // Strings should be equal but not point to the same buffer
    EXPECT_EQ(instance.Length(), otherInstance.Length());
    EXPECT_EQ(instance, otherInstance);
    EXPECT_NE(instance.c_str(), nullptr);
    EXPECT_NE(instance.c_str(), otherInstance.c_str());
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferAssignmentTest)
{
    const char buffer[] = "abcdefg";
    String instance;
    instance = buffer;

    EXPECT_FALSE(instance.IsEmpty());

    // String contents should be equal, but buffer pointer should not
    EXPECT_EQ(instance.Length(), strlen(buffer));
    EXPECT_EQ(instance, buffer);
    EXPECT_NE(instance.c_str(), nullptr);
    EXPECT_NE(instance.c_str(), &buffer[0]);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringSplitTest)
{
    String instance = "abc;;def;";
    auto parts = instance.Split(';');

    // String::Split should keep empty parts
    EXPECT_EQ(parts.Size(), 4);
    EXPECT_EQ(parts[0], "abc");
    EXPECT_EQ(parts[1], "");
    EXPECT_EQ(parts[2], "def");
    EXPECT_EQ(parts[3], "");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringSwapTest)
{
    String otherInstance = "abcdefg";
    String instance = "gfecdba";
    instance.swap(otherInstance);

    EXPECT_EQ(instance, "abcdefg");
    EXPECT_EQ(otherInstance, "gfecdba");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringEqualityTest)
{
    String otherInstance = "abcdefg";
    String instance = "abcdefg";

    EXPECT_EQ(instance, otherInstance);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringNonEqualityTest)
{
    String otherInstance = "abcdefg";
    String instance = "abcdefh";

    EXPECT_NE(instance, otherInstance);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferEqualityTest)
{
    const char buffer[] = "abcdefg";
    String instance = "abcdefg";

    EXPECT_EQ(instance, buffer);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringBufferNonEqualityTest)
{
    const char buffer[] = "abcdefg";
    String instance = "abcdefh";

    EXPECT_NE(instance, buffer);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringLessThanTest)
{
    // The less than operator is used for ordering of String instances
    String otherInstance = "abcdefh";
    String instance = "abcdefg";

    EXPECT_LT(instance, otherInstance);
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAppendTest)
{
    String otherInstance = "defg";
    String instance = "abc";
    instance.Append(otherInstance);

    // The appended String instance should not be modified
    EXPECT_EQ(otherInstance, "defg");
    EXPECT_EQ(instance, "abcdefg");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAppendEmptyTest)
{
    String otherInstance;
    String instance = "abc";
    instance.Append(otherInstance);

    // The appended String instance should not be modified
    EXPECT_EQ(otherInstance, "");
    EXPECT_EQ(instance, "abc");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAppendBufferTest)
{
    const char buffer[] = "defg";
    String instance = "abc";
    instance.Append(buffer);

    EXPECT_EQ(instance, "abcdefg");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAppendBufferNullptrTest)
{
    String instance = "abc";
    instance.Append((char*)nullptr);

    // Appending a null buffer should not throw
    EXPECT_EQ(instance, "abc");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddTest)
{
    String instance = "abc";
    String otherInstance = "defg";
    String combined = instance + otherInstance;

    // Neither of the original String instances should be modified
    EXPECT_EQ(instance, "abc");
    EXPECT_EQ(otherInstance, "defg");
    EXPECT_EQ(combined, "abcdefg");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddEmptyTest)
{
    String instance = "abc";
    String otherInstance;
    String combined = instance + otherInstance;

    // Neither of the original String instances should be modified and the result should not be the LHS String
    //  instance
    EXPECT_EQ(instance, "abc");
    EXPECT_EQ(otherInstance, "");
    EXPECT_EQ(combined, "abc");
    EXPECT_NE(instance.c_str(), combined.c_str());
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddBufferTest)
{
    String instance = "abc";
    String combined = instance + "defg";

    // The original String instance should not be modified
    EXPECT_EQ(instance, "abc");
    EXPECT_EQ(combined, "abcdefg");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddBufferNullptrTest)
{
    String instance = "abc";
    String combined = instance + (char*)nullptr;

    // Adding a null buffer should not throw and the result should not be the original String instance
    EXPECT_EQ(instance, "abc");
    EXPECT_EQ(combined, "abc");
    EXPECT_NE(instance.c_str(), combined.c_str());
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddAssignmentTest)
{
    String otherInstance = "defg";
    String instance = "abc";
    instance += otherInstance;

    // The appended String instance should not be modified
    EXPECT_EQ(otherInstance, "defg");
    EXPECT_EQ(instance, "abcdefg");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddAssignmentEmptyTest)
{
    String otherInstance;
    String instance = "abc";
    instance += otherInstance;

    // The appended String instance should not be modified
    EXPECT_EQ(otherInstance, "");
    EXPECT_EQ(instance, "abc");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddAssignmentBufferTest)
{
    const char buffer[] = "defg";
    String instance = "abc";
    instance += buffer;

    EXPECT_EQ(instance, "abcdefg");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringAddAssignmentBufferNullptrTest)
{
    String instance = "abc";
    instance += (char*)nullptr;

    // Appending a null buffer should not throw
    EXPECT_EQ(instance, "abc");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringTrimTest)
{
    String instance = " \rabc\t\n  ";
    String trimmed = instance.Trim();

    // The original String instance should not be modified
    EXPECT_EQ(instance, " \rabc\t\n  ");
    EXPECT_EQ(trimmed, "abc");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringTrimNoWhitespaceTest)
{
    String instance = "abc";
    String trimmed = instance.Trim();

    // The original String buffer should not be the same as the trimmed String buffer
    EXPECT_EQ(instance, "abc");
    EXPECT_EQ(trimmed, "abc");
    EXPECT_NE(instance.c_str(), trimmed.c_str());
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringTrimAllWhitespaceTest)
{
    String instance = "  \r\n\r\n\t";
    String trimmed = instance.Trim();

    // The original String should not be modified
    EXPECT_EQ(instance, "  \r\n\r\n\t");
    EXPECT_EQ(trimmed, "");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringToLowerTest)
{
    String instance = "\nAbC! _76-WHAT-lol";
    String transformed = instance.ToLower();

    // The original String instance should not be modified
    EXPECT_EQ(instance, "\nAbC! _76-WHAT-lol");
    EXPECT_EQ(transformed, "\nabc! _76-what-lol");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringJoinListTest)
{
    List<String> parts = { "abc", "def", "ghi" };
    String instance = String::Join(parts);

    EXPECT_EQ(instance, "abcdefghi");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringJoinListEmptyTest)
{
    List<String> parts(0);
    String instance = String::Join(parts);

    EXPECT_EQ(instance, "");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringJoinListSomeEmptyEntriesTest)
{
    List<String> parts = { "abc", String(), String(static_cast<size_t>(0ULL)) };
    String instance = String::Join(parts);

    EXPECT_EQ(instance, "abc");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringJoinListAllEmptyEntriesTest)
{
    List<String> parts = { "", String(), String(static_cast<size_t>(0ULL)) };
    String instance = String::Join(parts);

    EXPECT_EQ(instance, "");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringContainsTest)
{
    String instance = "abc_def_ghi_jkl";
    String substring = "def_g";

    EXPECT_TRUE(instance.Contains(substring));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringContainsSubstringNotFoundTest)
{
    String instance = "abc_def_ghi_jkl";
    String substring = "xyz";

    EXPECT_FALSE(instance.Contains(substring));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringContainsSubstringEmptyTest)
{
    String instance = "abc_def_ghi_jkl";
    String substring = "";

    EXPECT_FALSE(instance.Contains(substring));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringContainsSubstringTooLongTest)
{
    String instance = "abc_def_ghi_jkl";
    String substring = "abc_def_ghi_jkl_mno";

    EXPECT_FALSE(instance.StartsWith(substring));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringStartsWithTest)
{
    String instance = "Hello_World";
    String prefix = "Hello";

    EXPECT_TRUE(instance.StartsWith(prefix));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringStartsWithEmptyPrefixTest)
{
    String instance = "Hello_World";
    String prefix = "";

    EXPECT_FALSE(instance.StartsWith(prefix));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringStartsWithPrefixTooLongTest)
{
    String instance = "Hello_World";
    String prefix = "Hello_Worldy";

    EXPECT_FALSE(instance.StartsWith(prefix));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringEndsWithTest)
{
    String instance = "Hello_World";
    String postfix = "World";

    EXPECT_TRUE(instance.EndsWith(postfix));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringEndsWithEmptyPostfixTest)
{
    String instance = "Hello_World";
    String postfix = "";

    EXPECT_FALSE(instance.EndsWith(postfix));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringEndsWithPostfixTooLongTest)
{
    String instance = "Hello_World";
    String postfix = "Hello_Worldy";

    EXPECT_FALSE(instance.EndsWith(postfix));
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringSubStringTest)
{
    String instance = "Believe you can and you're halfway there.";
    size_t offset = 8;
    size_t length = 7;

    EXPECT_EQ(instance.SubString(offset, length), "you can");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringSubStringInvalidOffsetTest)
{
    String instance = "Believe you can and you're halfway there.";
    size_t offset = instance.Length() + 1;

    EXPECT_EQ(instance.SubString(offset), "");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringSubStringNoLengthTest)
{
    String instance = "Believe you can and you're halfway there.";
    size_t offset = 8;

    EXPECT_EQ(instance.SubString(offset), "you can and you're halfway there.");
}

CSP_INTERNAL_TEST(CSPEngine, CommonStringTests, StringSubStringInvalidLengthTest)
{
    String instance = "Believe you can and you're halfway there.";
    size_t offset = 8;
    size_t length = instance.Length() + 1;

    EXPECT_EQ(instance.SubString(offset, length), "you can and you're halfway there.");
}
