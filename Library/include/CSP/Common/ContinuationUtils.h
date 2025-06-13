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
#include "CSP/Common/Systems/Log/LogSystem.h"

namespace csp::common::continuations
{

/*
 * Print an error with provided string, and throw a cancellation error.
 */
inline void LogErrorAndCancelContinuation(
    std::string ErrorMsg, common::LogSystem* LogSystem, csp::common::LogLevel LogLevel = csp::common::LogLevel::Log)
{
    LogSystem->LogMsg(LogLevel, ErrorMsg.c_str());
    throw std::runtime_error("Continuation cancelled"); // Cancels the continuation chain.
}

/*
 * Intended to be placed at the end of an async++ continuation chain.
 * If the chain throws an unhandled exception, this will attempt to unwrap it,
 * and call a passed in callable, (probably a state-reset or cleanup function of some sort).
 * The callable is perfectly forwarded.
 */
template <typename Callable> inline auto InvokeIfExceptionInChain(Callable&& InvokeIfExceptionCallable, csp::common::LogSystem* LogSystem)
{
    static_assert(std::is_invocable_v<Callable, const std::exception&>,
        "InvokeIfExceptionCallable must be invocable with a single const std::exception& arg, if you need other state, try passing a capturing "
        "lambda.");

    return [InvokeIfExceptionCallable = std::forward<Callable>(InvokeIfExceptionCallable), LogSystem](async::task<void> ExceptionTask)
    {
        try
        {
            ExceptionTask.get();
        }
        catch (const std::exception& exception)
        {
            LogSystem->LogMsg(
                csp::common::LogLevel::Verbose, "Caught exception during async++ chain. Invoking callable from InvokeIfExceptionInChain");
            InvokeIfExceptionCallable(exception);
        }
    };
}

/*
 * Checks the multiplayer::Errorcode of a (passed by continuation) code.
 * If not a success, logs an error and aborts continuation.
 * Otherwise, logs a success message and continues. Does not pass anything to the next continuation.
 * Error context objects are optional, if unset, default values Failed, 500, and Unknown are used
 *
 * This being here, and not in multiplayer, is non-ideal, and a symptom of the fact that we have not yet
 * factored our Result types to have the concept of a multiplayer result.
 * This allows us to verify multiplayer functionality across the api-bounds, which is a bit of a leaky
 * abstraction, but pragmatically necessary to do the modularization.
 */
template <typename ErrorResultT>
inline auto AssertRequestSuccessOrErrorFromMultiplayerErrorCode(std::function<void(const ErrorResultT&)> Callback, std::string SuccessMsg,
    ErrorResultT ErrorResult, csp::common::LogSystem* LogSystem, csp::common::LogLevel LogLevel = csp::common::LogLevel::Log)
{
    return [Callback, SuccessMsg = std::move(SuccessMsg), ErrorResult = std::move(ErrorResult), LogSystem, LogLevel](
               const std::optional<csp::multiplayer::ErrorCode>& ErrorCode)
    {
        if (ErrorCode.has_value())
        {
            // Error Case. We have an error message, abort
            std::string ErrorMsg = std::string("Operation errored with error code: ") + csp::multiplayer::ErrorCodeToString(ErrorCode.value());
            if (Callback)
            {
                Callback(ErrorResult);
            }
            csp::common::continuations::LogErrorAndCancelContinuation(std::move(ErrorMsg), LogSystem, LogLevel);
        }
        else
        {
            // Success Case
            LogSystem->LogMsg(csp::common::LogLevel::Log, SuccessMsg.c_str());
        }
    };
}

}

CSP_END_IGNORE
