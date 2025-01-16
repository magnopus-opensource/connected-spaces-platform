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
#pragma once

#include "CSP/Common/String.h"

namespace csp::common
{
/// @brief Utility class containing static helper functions for encoding data.
class Encode
{
public:
    /// @brief URI-encodes the given string by escaping reserved and non-ASCII characters.
    /// @param UriToEncode const csp::common::String&: The string to encode.
    /// @param DoubleEncode bool: Whether to doubly encode the string. Typically necessary for GET requests that include a URL-like parameter.
    static csp::common::String URI(const csp::common::String& UriToEncode, bool DoublEncode = false);
};

/// @brief Utility class containing static helper functions for decoding data.
class Decode
{
public:
    /// @brief URI-decodes the given string by replacing percent-encoded characters with the actual character.
    /// @param UriToDecode const csp::common::String&: The string to decode.
    /// @param DoubleDecode bool: Whether to doubly decode the string.
    static csp::common::String URI(const csp::common::String& UriToDecode, bool DoubleDecode = false);
};
};
