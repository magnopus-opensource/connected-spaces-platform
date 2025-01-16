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

csp::common::String Encode::URI(const csp::common::String& UriToEncode, bool DoubleEncode)
{
    std::string RawString(UriToEncode.c_str());
    std::string EncodedString;

    for (auto Char : RawString)
    {
        // Covering alphanumeric characters.
        if ((Char >= 'a' && Char <= 'z') || (Char >= 'A' && Char <= 'Z') || (Char >= '0' && Char <= '9') || Char == '-' || Char == '_' || Char == '.'
            || Char == '~')
        {
            EncodedString += Char;
        }
        else
        {
            // A non-alphanumeric character needs encoding with the 0xXX pattern.
            char Buffer[10];
            sprintf(Buffer, "%X", Char);
            if (Char < 16)
            {
                // Single hex value with a preceding 0 will suffice
                EncodedString += "%0";
            }
            else
            {
                // Needs both hex values to represent it.
                EncodedString += "%";
            }

            EncodedString += Buffer;
        }
    }

    csp::common::String ReturnValue(EncodedString.c_str());

    if (DoubleEncode)
    {
        ReturnValue = Encode::URI(ReturnValue);
    }

    return ReturnValue;
}

csp::common::String Decode::URI(const csp::common::String& UriToDecode, bool DoubleDecode)
{
    std::string EncodedString(UriToDecode.c_str());
    std::string RawString;

    for (int i = 0; i < EncodedString.length(); i++)
    {
        if (EncodedString[i] != '%')
        {
            RawString += EncodedString[i]; // Just a regular character.
        }
        else
        {
            // We have an encoded character. Decode to int, cast to char, and append.
            int CharInteger = 0x0;
            sscanf(EncodedString.substr(i + 1, 2).c_str(), "%x", &CharInteger);
            RawString += static_cast<char>(CharInteger);
            i = i + 2; // done, skip the entire hex value and move onto the next character.
        }
    }

    csp::common::String ReturnValue(RawString.c_str());

    if (DoubleDecode)
    {
        ReturnValue = Decode::URI(ReturnValue);
    }

    return ReturnValue;
}

}; // namespace csp::common
