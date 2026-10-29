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

#include "CSP/CSPFoundation.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsManager.h"

#include <string>

CSP_NO_EXPORT

#if defined(CSP_WINDOWS)
#define csp_snprintf(SIZE, ...) _snprintf_s<SIZE>(__VA_ARGS__)
#else
#define csp_snprintf(SIZE, ...) snprintf(__VA_ARGS__)
#endif

// Use to suppress 'variable not used' warnings for profile or debug data
#define CSP_UNUSED(x) (void)(x)

namespace csp::profile
{
constexpr const int CSP_MAX_LOG_FORMAT_LEN = 1024;

template <typename... Args> void LogMsg(const csp::common::LogLevel Level, const csp::common::String& FormatStr, Args... args)
{
    if (csp::CSPFoundation::GetIsInitialised())
    {
        if (csp::systems::SystemsManager::Get().GetLogSystem()->LoggingEnabled(Level))
        {
            char MarkerString[CSP_MAX_LOG_FORMAT_LEN];
            csp_snprintf(CSP_MAX_LOG_FORMAT_LEN, MarkerString, CSP_MAX_LOG_FORMAT_LEN - 1, FormatStr, args...);

            csp::systems::SystemsManager::Get().GetLogSystem()->LogMsg(Level, MarkerString);
        }
    }
}

#define CSP_LOG_MSG(LEVEL, MSG)                                                                                                                      \
    if (csp::CSPFoundation::GetIsInitialised())                                                                                                      \
    {                                                                                                                                                \
        csp::systems::SystemsManager::Get().GetLogSystem()->LogMsg(LEVEL, MSG);                                                                      \
    }

#define CSP_LOG_FORMAT(LEVEL, FORMAT_STR, ...) csp::profile::LogMsg(LEVEL, FORMAT_STR, __VA_ARGS__)

#define CSP_LOG_ERROR_MSG(MSG)                                                                                                                       \
    if (csp::CSPFoundation::GetIsInitialised())                                                                                                      \
    {                                                                                                                                                \
        csp::systems::SystemsManager::Get().GetLogSystem()->LogMsg(csp::common::LogLevel::Error, MSG);                                               \
    }

#define CSP_LOG_ERROR_FORMAT(FORMAT_STR, ...) csp::profile::LogMsg(csp::common::LogLevel::Error, FORMAT_STR, __VA_ARGS__)

#define CSP_LOG_WARN_MSG(MSG)                                                                                                                        \
    if (csp::CSPFoundation::GetIsInitialised())                                                                                                      \
    {                                                                                                                                                \
        csp::systems::SystemsManager::Get().GetLogSystem()->LogMsg(csp::common::LogLevel::Warning, MSG);                                             \
    }

#define CSP_LOG_WARN_FORMAT(FORMAT_STR, ...) csp::profile::LogMsg(csp::common::LogLevel::Warning, FORMAT_STR, __VA_ARGS__)

} // namespace csp::profile
