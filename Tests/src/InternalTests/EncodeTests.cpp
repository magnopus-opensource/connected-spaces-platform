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

#include "Common/Encode.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

CSP_INTERNAL_TEST(CSPEngine, EncodeTests, EncodeURI)
{
    {
        // Encoding a string with no reserved characters should produce the same string.
        const csp::common::String stringWithNoReservedCharacters = "abcdefghijzlmnopqrstuvwxyz";
        csp::common::String encodedString = csp::common::Encode::URI(stringWithNoReservedCharacters);
        EXPECT_EQ(encodedString, stringWithNoReservedCharacters);
    }

    {
        // Encoding a string with reserved characters should produce a version of the string that uses the standard URI character encoding scheme.
        const csp::common::String stringWithReservedCharacters = " *";
        csp::common::String encodedString = csp::common::Encode::URI(stringWithReservedCharacters);
        EXPECT_NE(encodedString, stringWithReservedCharacters);
        EXPECT_EQ(encodedString, "%20%2A");
    }

    {
        // Encoding a string with reserved characters and then decoding it should produce a result equal to the original string.
        const csp::common::String originalUrl("abc defghij*zlmnopqrst#uvwxyz");
        csp::common::String encodedString = csp::common::Encode::URI(originalUrl);
        csp::common::String decodedString = csp::common::Decode::URI(encodedString);
        EXPECT_EQ(decodedString, originalUrl);
    }

    {
        // Double encoding and double decoding should produce the same end-result.
        const csp::common::String originalUrl("abc defghij*zlmnopqrst#uvwxyz");
        csp::common::String encodedString = csp::common::Encode::URI(originalUrl, true);
        csp::common::String decodedString = csp::common::Decode::URI(encodedString, true);
        EXPECT_EQ(decodedString, originalUrl);
    }
}