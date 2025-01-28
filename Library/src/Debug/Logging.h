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
#include "CSP/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsManager.h"

#include <string>

CSP_NO_EXPORT

#if defined(__clang__) || defined(__GNUC__)
#define CSP_FUNC_DEF __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define CSP_FUNC_DEF __FUNCSIG__
#else
#error Compiler not supported
#endif

#if !defined(CSP_PROFILING_ENABLED)
#if defined(NDEBUG)
// Disable profiling in release by default (change this to override)
#define CSP_PROFILING_ENABLED 0
#else
#define CSP_PROFILING_ENABLED 1
#endif
#endif

#if defined(CSP_WINDOWS)
#define csp_snprintf(SIZE, ...) _snprintf_s<SIZE>(__VA_ARGS__)
#else
#define csp_snprintf(SIZE, ...) snprintf(__VA_ARGS__)
#endif

// Use to suppress 'variable not used' warnings for profile or debug data
#define CSP_UNUSED(x) (void)(x)

// Concat used to make scoped vars unique using __LINE__
// e.g. CSP_CONCAT(ProfilerTag, __LINE__) becomes something like ProfilerTag128
// Note the two nested macros are needed otherwise it becomes ProfilerTag__LINE__
#define CSP_CONCAT_IMPL(x, y) x##y
#define CSP_CONCAT(x, y) CSP_CONCAT_IMPL(x, y)

namespace csp::profile
{
constexpr const int CSP_MAX_LOG_FORMAT_LEN = 1024;

template <typename... Args> void LogMsg(const csp::systems::LogLevel Level, const csp::common::String& FormatStr, Args... args)
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
        csp::systems::SystemsManager::Get().GetLogSystem()->LogMsg(csp::systems::LogLevel::Error, MSG);                                              \
    }

#define CSP_LOG_ERROR_FORMAT(FORMAT_STR, ...) csp::profile::LogMsg(csp::systems::LogLevel::Error, FORMAT_STR, __VA_ARGS__)

#define CSP_LOG_WARN_MSG(MSG)                                                                                                                        \
    if (csp::CSPFoundation::GetIsInitialised())                                                                                                      \
    {                                                                                                                                                \
        csp::systems::SystemsManager::Get().GetLogSystem()->LogMsg(csp::systems::LogLevel::Warning, MSG);                                            \
    }

#define CSP_LOG_WARN_FORMAT(FORMAT_STR, ...) csp::profile::LogMsg(csp::systems::LogLevel::Warning, FORMAT_STR, __VA_ARGS__)

#if CSP_PROFILING_ENABLED

class ScopedProfiler
{
public:
    ScopedProfiler(const char* Tag)
    {
        if (csp::CSPFoundation::GetIsInitialised())
        {
            csp::systems::SystemsManager::Get().GetLogSystem()->BeginMarker(csp::common::String(Tag));
        }
    }

    ScopedProfiler(const csp::common::String& Tag)
    {
        if (csp::CSPFoundation::GetIsInitialised())
        {
            csp::systems::SystemsManager::Get().GetLogSystem()->BeginMarker(Tag);
        }
    }

    ScopedProfiler(const std::string& Tag)
    {
        if (csp::CSPFoundation::GetIsInitialised())
        {
            csp::systems::SystemsManager::Get().GetLogSystem()->BeginMarker(Tag.c_str());
        }
    }

    template <typename... Args> ScopedProfiler(const csp::common::String& FormatStr, Args... args)
    {
        if (csp::CSPFoundation::GetIsInitialised())
        {
            char MsgString[CSP_MAX_LOG_FORMAT_LEN];
            csp_snprintf(CSP_MAX_LOG_FORMAT_LEN, MsgString, CSP_MAX_LOG_FORMAT_LEN - 1, FormatStr, args...);

            csp::systems::SystemsManager::Get().GetLogSystem()->BeginMarker(MsgString);
        }
    }

    ~ScopedProfiler()
    {
        if (csp::CSPFoundation::GetIsInitialised())
        {
            csp::systems::SystemsManager::Get().GetLogSystem()->EndMarker();
        }
    }
};

inline std::string TrimFunctionTag(const std::string& Tag)
{
    const size_t TrimStart = Tag.find("csp::");
    if (TrimStart != std::string::npos)
    {
        const size_t TrimLength = Tag.find_first_of('(') - TrimStart;
        return Tag.substr(TrimStart, TrimLength);
    }
    else
    {
        const size_t TrimLength = Tag.find_first_of('(');
        return Tag.substr(0, TrimLength);
    }
}

template <typename... Args> void BeginMarker(const csp::common::String& FormatStr, Args... args)
{
    if (csp::CSPFoundation::GetIsInitialised())
    {
        char MarkerString[CSP_MAX_LOG_FORMAT_LEN];
        csp_snprintf(CSP_MAX_LOG_FORMAT_LEN, MarkerString, CSP_MAX_LOG_FORMAT_LEN - 1, FormatStr, args...);

        csp::systems::SystemsManager::Get().GetLogSystem()->BeginMarker(MarkerString);
    }
}

template <typename... Args> void LogEvent(const csp::common::String& FormatStr, Args... args)
{
    if (csp::CSPFoundation::GetIsInitialised())
    {
        char MarkerString[CSP_MAX_LOG_FORMAT_LEN];
        csp_snprintf(CSP_MAX_LOG_FORMAT_LEN, MarkerString, CSP_MAX_LOG_FORMAT_LEN - 1, FormatStr, args...);

        csp::systems::SystemsManager::Get().GetLogSystem()->LogEvent(MarkerString);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Scoped profiling event which automatically grabs current function name.
//
// Example:
//		void Function()
//		{
//			CSP_PROFILE_SCOPED();
//			... code ...
//		}
#define CSP_PROFILE_SCOPED() csp::profile::ScopedProfiler CSP_CONCAT(ProfilerTag, __LINE__)(csp::profile::TrimFunctionTag(CSP_FUNC_DEF))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Manual Begin/End profiling markers
//
// Example:
//		void Function()
//		{
//			... code ...
//			... code ...
//			CSP_PROFILE_BEGIN("Subsection Tag");
//			... subsection of code to profile...
//			CSP_PROFILE_END();
//			... code ...
//		}
#define CSP_PROFILE_BEGIN(TAG)                                                                                                                       \
    if (csp::CSPFoundation::GetIsInitialised())                                                                                                      \
    {                                                                                                                                                \
        csp::systems::SystemsManager::Get().GetLogSystem()->BeginMarker(TAG);                                                                        \
    }

#define CSP_PROFILE_END()                                                                                                                            \
    if (csp::CSPFoundation::GetIsInitialised())                                                                                                      \
    {                                                                                                                                                \
        csp::systems::SystemsManager::Get().GetLogSystem()->EndMarker();                                                                             \
    }

#define CSP_PROFILE_BEGIN_FORMAT(FORMAT_STR, ...) csp::profile::BeginMarker(FORMAT_STR, __VA_ARGS__);
#define CSP_PROFILE_SCOPED_FORMAT(FORMAT_STR, ...) csp::profile::ScopedProfiler CSP_CONCAT(ProfilerTag, __LINE__)(FORMAT_STR, __VA_ARGS__)
#define CSP_PROFILE_SCOPED_TAG(TAG) csp::profile::ScopedProfiler CSP_CONCAT(ProfilerTag, __LINE__)(TAG)

#define CSP_PROFILE_EVENT_TAG(TAG)                                                                                                                   \
    if (csp::CSPFoundation::GetIsInitialised())                                                                                                      \
    {                                                                                                                                                \
        csp::systems::SystemsManager::Get().GetLogSystem()->LogEvent(TAG);                                                                           \
    }

#define CSP_PROFILE_EVENT_FORMAT(FORMAT_STR, ...) csp::profile::LogEvent(FORMAT_STR, __VA_ARGS__);

#else

// Compile everything out for zero overhead when disabled

#define CSP_PROFILE_SCOPED()

#define CSP_PROFILE_BEGIN(TAG)
#define CSP_PROFILE_END()

#define CSP_PROFILE_BEGIN_FORMAT(FORMAT_STR, ...)
#define CSP_PROFILE_SCOPED_FORMAT(FORMAT_STR, ...)
#define CSP_PROFILE_SCOPED_TAG(TAG)

#define CSP_PROFILE_EVENT_TAG(TAG)
#define CSP_PROFILE_EVENT_FORMAT(FORMAT_STR, ...)

#endif

} // namespace csp::profile
