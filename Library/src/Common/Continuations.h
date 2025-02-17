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
 * General purpose utility continuations for use in async++ based task chaining.
 * These exist for a couple of reasons:
 *  - Raw nested callbacks were becoming unreadable and causing bugs
 *  - Via using common continuations, we can move towards standardised error handling
 * If you're wondering why we go to the effort of integrate an async lib rather than
 * simply using a standard blocking future/promise approach, it's because of WASM.
 * WASM in browsers does not allow you to block the main thread, ever.
 *
 * For more info on the specifics of continuations : https://github.com/Amanieu/asyncplusplus/wiki/Tasks
 *
 * You may be wondering why these are defined in a header file, it's an annoying
 * set of tradeoffs
 * - We want a shared location to enable common error management
 * - Continuations must be callables, so we need to return an invokable object, obvious choice is a lambda.
 * - Lambdas require auto return types, which require the definition to be known at point of call
 * - We _could_ explicitly define continuation types with specific member variables to mimic capture,
 *   but I worry that losing the flexibility of capture lists will disincentivise future utilities.
 *   (Capture is the reason we can't use std::function or equivilants, we require state.)
 *
 * If you can figure out a good way to get these in a .cpp file, it'd help build times a little :)
 */

#pragma once

#include "CSP/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsResult.h"
#include "CSP/Systems/WebService.h"
#include "CallHelpers.h"
#include "Debug/Logging.h"
#include "Multiplayer/ErrorCodeStrings.h"
#include <async++.h>
#include <optional>
#include <string>
#include <type_traits>

namespace csp::common::continuations
{

using namespace csp::systems;

/*
 * Print an error with provided error context objects, and throw a cancellation error.
 * Calls the main callback as an error before throwing.
 */
inline void LogErrorAndCancelContinuation(NullResultCallback Callback, std::string ErrorMsg, EResultCode ResultCode,
    csp::web::EResponseCodes HttpResultCode, ERequestFailureReason FailureReason, csp::systems::LogLevel LogLevel = csp::systems::LogLevel::Log)
{
    CSP_LOG_MSG(LogLevel, ErrorMsg.c_str());
    NullResult FailureResult(ResultCode, HttpResultCode, FailureReason);
    INVOKE_IF_NOT_NULL(Callback, FailureResult);
    throw async::task_canceled(); // Cancels the continuation chain.
}

/*
 * Checks the result code of a (passed by continuation) result object.
 * If not a success, logs an error and aborts continuation.
 * Otherwise, logs a success message and continues, forwarding the result to the next continuation.
 * Error context objects are optional, if unset, the values from the result object will be used.
 */
template <typename ResultT>
inline auto AssertRequestSuccessOrErrorFromResult(NullResultCallback Callback, std::string SuccessMsg, std::string ErrorMsg,
    std::optional<EResultCode> ResultCode, std::optional<csp::web::EResponseCodes> HttpResultCode, std::optional<ERequestFailureReason> FailureReason,
    csp::systems::LogLevel LogLevel = csp::systems::LogLevel::Log)
{
    return [Callback, SuccessMsg = std::move(SuccessMsg), ErrorMsg = std::move(ErrorMsg), ResultCode, HttpResultCode, FailureReason, LogLevel](
               const ResultT& Result)
    {
        if (Result.GetResultCode() != EResultCode::Success)
        {
            // Error Case
            auto ResultCodeToUse = ResultCode.value_or(Result.GetResultCode());
            auto HTTPResultCodeToUse = HttpResultCode.value_or(static_cast<csp::web::EResponseCodes>(Result.GetHttpResultCode()));
            auto FailureReasonToUse = FailureReason.value_or(Result.GetFailureReason());
            LogErrorAndCancelContinuation(Callback, std::move(ErrorMsg), ResultCodeToUse, HTTPResultCodeToUse, FailureReasonToUse, LogLevel);
        }
        else
        {
            // Success Case
            CSP_LOG_MSG(csp::systems::LogLevel::Log, SuccessMsg.c_str());
        }
        return Result;
    };
}

/*
 * Checks the multiplayer::Errorcode of a (passed by continuation) code.
 * If not a success, logs an error and aborts continuation.
 * Otherwise, logs a success message and continues. Does not pass anything to the next continuation.
 * Error context objects are optional, if unset, default values Failed, 500, and Unknown are used
 */
inline auto AssertRequestSuccessOrErrorFromErrorCode(NullResultCallback Callback, std::string SuccessMsg, std::optional<EResultCode> ResultCode,
    std::optional<csp::web::EResponseCodes> HttpResultCode, std::optional<ERequestFailureReason> FailureReason,
    csp::systems::LogLevel LogLevel = csp::systems::LogLevel::Log)
{
    return [Callback, SuccessMsg = std::move(SuccessMsg), ResultCode, HttpResultCode, FailureReason, LogLevel](
               const std::optional<csp::multiplayer::ErrorCode>& ErrorCode)
    {
        if (ErrorCode.has_value())
        {
            // Error Case. We have an error message, abort
            std::string ErrorMsg = std::string("Operation errored with error code: ") + csp::multiplayer::ErrorCodeToString(ErrorCode.value());
            LogErrorAndCancelContinuation(Callback, std::move(ErrorMsg), ResultCode.value_or(EResultCode::Failed),
                HttpResultCode.value_or(csp::web::EResponseCodes::ResponseInternalServerError),
                FailureReason.value_or(ERequestFailureReason::Unknown), LogLevel);
        }
        else
        {
            // Success Case
            CSP_LOG_MSG(csp::systems::LogLevel::Log, SuccessMsg.c_str());
        }
    };
}

/* Print a success message and report a successfull result via the callback */
inline auto ReportSuccess(NullResultCallback Callback, std::string SuccessMsg)
{
    return [Callback, SuccessMsg = std::move(SuccessMsg)]()
    {
        /* We joined the space and refreshed the multiplayer connection to change scopes. We're done! */
        CSP_LOG_MSG(LogLevel::Log, SuccessMsg.c_str());
        NullResult SuccessResult(EResultCode::Success, csp::web::EResponseCodes::ResponseOK, ERequestFailureReason::None);
        INVOKE_IF_NOT_NULL(Callback, SuccessResult);
    };
}

/*
 * Intended to be placed at the end of an async++ continuation chain.
 * If the chain throws an unhandled exception, this will attempt to unwrap it,
 * and call a passed in callable, (probably a state-reset or cleanup function of some sort).
 * The callable is perfectly forwarded.
 */
template <typename Callable> inline auto InvokeIfExceptionInChain(Callable&& InvokeIfExceptionCallable)
{
    static_assert(std::is_invocable_v<Callable>,
        "InvokeIfExceptionCallable must be invocable with zero arguments, if you need state, try passing a capturing lambda.");

    return [InvokeIfExceptionCallable = std::forward<Callable>(InvokeIfExceptionCallable)](async::task<void> ExceptionTask)
    {
        try
        {
            ExceptionTask.get();
        }
        catch (...)
        {
            CSP_LOG_MSG(csp::systems::LogLevel::Verbose, "Caught exception during async++ chain. Invoking callable from InvokeIfExceptionInChain");
            InvokeIfExceptionCallable();
        }
    };
}
//"Private" namespace for testing, to allow us not to link async++ in tests,
// for the few tests where we want to stricly test mechanisms.
namespace detail
{
    namespace testing
    {
        /* These chains spawned to be able to test InvokeIfExceptionIsInChain
         * Normally, I might make the argument that this is testing our dependencies, but
         * this seems like it might become so essential to our foundational structuring
         * that you'll forgive me for being a little paranoid.
         */
        template <typename ExceptionHandlerCallable>
        inline void SpawnChainThatThrowsNoExceptionWithHandlerAtEnd(ExceptionHandlerCallable&& ExceptionHandler)
        {
            async::spawn(async::inline_scheduler(), []() { return; })
                .then(async::inline_scheduler(), InvokeIfExceptionInChain(std::forward<ExceptionHandlerCallable>(ExceptionHandler)));
        }

        template <typename ExceptionHandlerCallable>
        inline void SpawnChainThatThrowsGeneralExceptionWithHandlerAtEnd(ExceptionHandlerCallable&& ExceptionHandler)
        {
            async::spawn(async::inline_scheduler(), []() { throw std::runtime_error(""); })
                .then(async::inline_scheduler(), InvokeIfExceptionInChain(std::forward<ExceptionHandlerCallable>(ExceptionHandler)));
        }

        template <typename ExceptionHandlerCallable>
        inline void SpawnChainThatCallsLogErrorAndCancelContinuationWithHandlerAtEnd(
            ExceptionHandlerCallable&& ExceptionHandler, NullResultCallback ResultCallback)
        {
            async::spawn(async::inline_scheduler(),
                [ResultCallback]() {
                    LogErrorAndCancelContinuation(
                        ResultCallback, "", EResultCode::Failed, csp::web::EResponseCodes::ResponseInit, ERequestFailureReason::Unknown);
                })
                .then(async::inline_scheduler(), InvokeIfExceptionInChain(std::forward<ExceptionHandlerCallable>(ExceptionHandler)));
        }

        template <typename IntermediateStepCallable, typename ExceptionHandlerCallable>
        inline void SpawnChainThatCallsLogErrorAndCancelContinuationWithIntermediateStepAndHandlerAtEnd(
            IntermediateStepCallable&& IntermediateStep, ExceptionHandlerCallable&& ExceptionHandler, NullResultCallback ResultCallback)
        {
            async::spawn(async::inline_scheduler(),
                [ResultCallback]() {
                    LogErrorAndCancelContinuation(
                        ResultCallback, "", EResultCode::Failed, csp::web::EResponseCodes::ResponseInit, ERequestFailureReason::Unknown);
                })
                .then(async::inline_scheduler(), std::forward<IntermediateStepCallable>(IntermediateStep))
                .then(async::inline_scheduler(), InvokeIfExceptionInChain(std::forward<ExceptionHandlerCallable>(ExceptionHandler)));
        }
    }
}
} // namespace csp::common::continuations
