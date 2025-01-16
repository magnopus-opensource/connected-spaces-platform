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
#include "CSP/Memory/DllAllocator.h"

#include <cstdio>
#include <functional>
#include <memory>

CSP_NO_EXPORT

namespace csp::common
{

/// @brief Builds a string with the specified format and arguments.
/// Based on sprintf:
/// https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
/// @param Format String : Format for string (See sprintf)
/// @param args Args... : Arguments to format into the string
/// @return String : String built from arguments
template <typename... Args> static String StringFormat(const String& Format, Args... args)
{
    int SizeS = std::snprintf(nullptr, 0, Format.c_str(), args...) + 1; // Extra space for '\0'

    if (SizeS <= 0)
    {
        return String();
    }

    auto Size = static_cast<size_t>(SizeS);

    std::unique_ptr<char[], csp::memory::DllDeleter<char>> Buf((char*)csp::memory::DllAlloc(Size));

    std::snprintf(Buf.get(), Size, Format.c_str(), args...);

    return String(Buf.get(), Size - 1); // We don't want the '\0' inside
};

} // namespace csp::common
