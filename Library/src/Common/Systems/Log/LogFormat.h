/*
 * Copyright 2026 Magnopus LLC

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

#include "CSP/Common/Systems/Log/LogSystem.h"

#include <fmt/format.h>

#include <utility>

namespace csp::common
{

/// @brief Formats a message with fmt and logs it to the given LogSystem at the given level.
template <typename... Args> void LogFormat(LogSystem& Log, LogLevel Level, fmt::format_string<Args...> Format, Args&&... Arguments)
{
    if (Log.LoggingEnabled(Level))
    {
        Log.LogMsg(Level, fmt::format(Format, std::forward<Args>(Arguments)...).c_str());
    }
}

} // namespace csp::common
