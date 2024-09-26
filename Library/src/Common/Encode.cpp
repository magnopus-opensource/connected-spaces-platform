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
#include "Poco/URI.h"

namespace csp::common
{

csp::common::String Encode::URI(const csp::common::String& UriToEncode, bool DoubleEncode)
{
    std::string RawString(UriToEncode.c_str());
	std::string EncodedString;
    Poco::URI::encode(RawString, "", EncodedString);

    csp::common::String ReturnValue(EncodedString.c_str());

    if(DoubleEncode)
    {
	    ReturnValue = Encode::URI(ReturnValue);
    }

	return ReturnValue;
}

csp::common::String Decode::URI(const csp::common::String& UriToDecode, bool DoubleDecode)
{
    std::string EncodedString(UriToDecode.c_str());
    std::string RawString;
    Poco::URI::decode(EncodedString, RawString);

    csp::common::String ReturnValue(RawString.c_str());

    if(DoubleDecode)
    {
	    ReturnValue = Decode::URI(ReturnValue);
    }

	return ReturnValue;
}

}; // namespace csp::common
