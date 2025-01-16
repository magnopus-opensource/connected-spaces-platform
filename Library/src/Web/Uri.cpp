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
#include "Web/Uri.h"

#include "Common/NumberFormatter.h"
#include "Common/StlString.h"

#include <iostream>
#include <string>

namespace csp::web
{

Uri::Uri()
    : NumParams(0)
{
}

Uri::Uri(const char* InUri)
    : UriPath(csp::common::String(InUri))
    , NumParams(0)
{
}

Uri::Uri(csp::common::String& InUri)
    : UriPath(InUri)
    , NumParams(0)
{
}

const char* Uri::GetAsString() const { return UriPath.c_str(); }

const std::string Uri::GetAsStdString() const { return std::string(UriPath.c_str()); }

void Uri::SetWithParams(const char* InUri, std::initializer_list<csp::common::String> Params)
{
    csp::StlString Uri(InUri);
    constexpr size_t SearchStartIndex = 0;

    for (auto Param : Params)
    {
        const char* ParamStr = Param.c_str();

        size_t StartIndex = Uri.find('{', SearchStartIndex);
        size_t EndIndex = Uri.find('}', StartIndex) + 1;
        size_t Len = EndIndex - StartIndex;

        Uri = Uri.replace(StartIndex, Len, ParamStr);
    }

    UriPath = csp::common::String(Uri.c_str());
}

csp::common::String Uri::Encode(const csp::common::String& InUri)
{
    std::string EncodedString;

    const std::string InString(InUri.c_str());
    const std::string Reserved("<>{}|\\\"^`!*'()$,[]&$@~#%");

    for (auto c : InString)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
        {
            EncodedString += c;
        }
        // if its a non-alphanumeric ASCII character (below 0x20 or above 0x7f) OR its one of our reserved characters
        // within the range 0x20-0x7f that isn't alphanumeric
        else if (c <= 0x20 || c >= 0x7F || Reserved.find(c) != std::string::npos)
        {
            EncodedString += '%';
            EncodedString += csp::NumberFormatter::FormatHex(static_cast<unsigned>(static_cast<unsigned char>(c)), 2);
        }
        else
            EncodedString += c;
    }

    return csp::common::String(EncodedString.c_str());
}

} // namespace csp::web
