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

#include <string>

namespace csp::common
{

csp::common::String Encode::URI(const csp::common::String& uriToEncode, bool doubleEncode)
{
    std::string rawString(uriToEncode.c_str());
    std::string encodedString;

    for (auto Char : rawString)
    {
        // Covering alphanumeric characters.
        if ((Char >= 'a' && Char <= 'z') || (Char >= 'A' && Char <= 'Z') || (Char >= '0' && Char <= '9') || Char == '-' || Char == '_' || Char == '.'
            || Char == '~')
        {
            encodedString += Char;
        }
        else
        {
            // A non-alphanumeric character needs encoding with the 0xXX pattern.
            char buffer[10];
            sprintf(buffer, "%X", Char);
            if (Char < 16)
            {
                // Single hex value with a preceding 0 will suffice
                encodedString += "%0";
            }
            else
            {
                // Needs both hex values to represent it.
                encodedString += "%";
            }

            encodedString += buffer;
        }
    }

    csp::common::String returnValue(encodedString.c_str());

    if (doubleEncode)
    {
        returnValue = Encode::URI(returnValue);
    }

    return returnValue;
}

csp::common::String Decode::URI(const csp::common::String& uriToDecode, bool doubleDecode)
{
    std::string encodedString(uriToDecode.c_str());
    std::string rawString;

    for (size_t i = 0; i < encodedString.length(); i++)
    {
        if (encodedString[i] != '%')
        {
            rawString += encodedString[i]; // Just a regular character.
        }
        else
        {
            // We have an encoded character. Decode to int, cast to char, and append.
            int charInteger = 0x0;
            sscanf(encodedString.substr(i + 1, 2).c_str(), "%x", &charInteger);
            rawString += static_cast<char>(charInteger);
            i = i + 2; // done, skip the entire hex value and move onto the next character.
        }
    }

    csp::common::String returnValue(rawString.c_str());

    if (doubleDecode)
    {
        returnValue = Decode::URI(returnValue);
    }

    return returnValue;
}

}; // namespace csp::common
