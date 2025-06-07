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
#pragma once

#include "CSP/CSPCommon.h"
CSP_START_IGNORE

#include <optional>

#include "CSP/Common/CSPAsyncScheduler.h"
#include "CSP/Systems/Log/LogSystem.h"
#include "Debug/Logging.h"

namespace csp::common::continuations
{

/*
 * Print an error with provided string, and throw a cancellation error.
 */
inline void LogErrorAndCancelContinuation(std::string ErrorMsg, csp::systems::LogLevel LogLevel = csp::systems::LogLevel::Log)
{
    CSP_LOG_MSG(LogLevel, ErrorMsg.c_str());
    throw std::runtime_error("Continuation cancelled"); // Cancels the continuation chain.
}

/*
 * Intended to be placed at the end of an async++ continuation chain.
 * If the chain throws an unhandled exception, this will attempt to unwrap it,
 * and call a passed in callable, (probably a state-reset or cleanup function of some sort).
 * The callable is perfectly forwarded.
 */
template <typename Callable> inline auto InvokeIfExceptionInChain(Callable&& InvokeIfExceptionCallable)
{
    static_assert(std::is_invocable_v<Callable, const std::exception&>,
        "InvokeIfExceptionCallable must be invocable with a single const std::exception& arg, if you need other state, try passing a capturing "
        "lambda.");

    return [InvokeIfExceptionCallable = std::forward<Callable>(InvokeIfExceptionCallable)](async::task<void> ExceptionTask)
    {
        try
        {
            ExceptionTask.get();
        }
        catch (const std::exception& exception)
        {
            CSP_LOG_MSG(csp::systems::LogLevel::Verbose, "Caught exception during async++ chain. Invoking callable from InvokeIfExceptionInChain");
            InvokeIfExceptionCallable(exception);
        }
    };
}
}

CSP_END_IGNORE
