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
#include "Common/Web/Uri.h"

#include "Common/NumberFormatter.h"

#include <iostream>
#include <string>

namespace csp::web
{

Uri::Uri()
    : m_numParams(0)
{
}

Uri::Uri(const char* inUri)
    : m_uriPath(csp::common::String(inUri))
    , m_numParams(0)
{
}

Uri::Uri(csp::common::String& inUri)
    : m_uriPath(inUri)
    , m_numParams(0)
{
}

const char* Uri::GetAsString() const { return m_uriPath.c_str(); }

const std::string Uri::GetAsStdString() const { return std::string(m_uriPath.c_str()); }

void Uri::SetWithParams(const char* inUri, std::initializer_list<csp::common::String> params)
{
    std::string uri(inUri);
    constexpr size_t searchStartIndex = 0;

    for (auto param : params)
    {
        const char* paramStr = param.c_str();

        size_t startIndex = uri.find('{', searchStartIndex);
        size_t endIndex = uri.find('}', startIndex) + 1;
        size_t len = endIndex - startIndex;

        uri = uri.replace(startIndex, len, paramStr);
    }

    m_uriPath = csp::common::String(uri.c_str());
}

csp::common::String Uri::Encode(const csp::common::String& inUri)
{
    std::string encodedString;

    const std::string inString(inUri.c_str());
    const std::string reserved("<>{}|\\\"^`!*'()$,[]&$@~#%");

    for (auto c : inString)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
        {
            encodedString += c;
        }
        // if its a non-alphanumeric ASCII character (below 0x20 or above 0x7f) OR its one of our reserved characters
        // within the range 0x20-0x7f that isn't alphanumeric
        else if (c <= 0x20 || c >= 0x7F || reserved.find(c) != std::string::npos)
        {
            encodedString += '%';
            encodedString += csp::NumberFormatter::FormatHex(static_cast<unsigned>(static_cast<unsigned char>(c)), 2);
        }
        else
            encodedString += c;
    }

    return csp::common::String(encodedString.c_str());
}

} // namespace csp::web
