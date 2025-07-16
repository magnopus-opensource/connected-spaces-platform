/*
 * Copyright 2025 Magnopus LLC

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

/*
 * fmt Formatters to allow internal formatting of common types
 * If you are including csp as a library, you may wish to include this, but you will
 * need to have linked against the same fmt library CSP has.
 * This does not constitute public interface.
 */

#pragma once

CSP_START_IGNORE

#include "CSP/Common/String.h"

#include <fmt/base.h>
#include <string_view>

namespace fmt
{

// Formatter for csp::common::String.

template <> struct fmt::formatter<csp::common::String> : formatter<std::string_view>
{
    // parse is inherited from formatter<string_view>.

    auto format(const csp::common::String& s, format_context& ctx) const -> format_context::iterator
    {
        // wrap raw data in a string_view and forward to fmt
        return formatter<std::string_view>::format(std::string_view(s.c_str(), s.Length()), ctx);
    }
};

} // namespace fmt

CSP_END_IGNORE
