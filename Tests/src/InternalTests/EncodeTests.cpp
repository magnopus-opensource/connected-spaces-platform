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

#ifndef SKIP_INTERNAL_TESTS

#include "Common/Encode.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

CSP_INTERNAL_TEST(CSPEngine, EncodeTests, EncodeURI)
{
    {
        // Encoding a string with no reserved characters should produce the same string.
        const csp::common::String StringWithNoReservedCharacters = "abcdefghijzlmnopqrstuvwxyz";
        csp::common::String EncodedString = csp::common::Encode::URI(StringWithNoReservedCharacters);
        EXPECT_EQ(EncodedString, StringWithNoReservedCharacters);
    }

    {
        // Encoding a string with reserved characters should produce a version of the string that uses the standard URI character encoding scheme.
        const csp::common::String StringWithReservedCharacters = " *";
        csp::common::String EncodedString = csp::common::Encode::URI(StringWithReservedCharacters);
        EXPECT_NE(EncodedString, StringWithReservedCharacters);
        EXPECT_EQ(EncodedString, "%20%2A");
    }

    {
        // Encoding a string with reserved characters and then decoding it should produce a result equal to the original string.
        const csp::common::String OriginalURL("abc defghij*zlmnopqrst#uvwxyz");
        csp::common::String EncodedString = csp::common::Encode::URI(OriginalURL);
        csp::common::String DecodedString = csp::common::Decode::URI(EncodedString);
        EXPECT_EQ(DecodedString, OriginalURL);
    }

    {
        // Double encoding and double decoding should produce the same end-result.
        const csp::common::String OriginalURL("abc defghij*zlmnopqrst#uvwxyz");
        csp::common::String EncodedString = csp::common::Encode::URI(OriginalURL, true);
        csp::common::String DecodedString = csp::common::Decode::URI(EncodedString, true);
        EXPECT_EQ(DecodedString, OriginalURL);
    }
}

#endif