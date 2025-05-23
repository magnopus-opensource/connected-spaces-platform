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
 *    --- !!! Not true, we discovered that even capturing lambdas can be stored in a std::function, this whole reasoning may be incorrect.
 * - We _could_ explicitly define continuation types with specific member variables to mimic capture,
 *   but I worry that losing the flexibility of capture lists will disincentivise future utilities.
 *   (Capture is the reason we can't use std::function or equivilants, we require state.)
 *
 * If you can figure out a good way to get these in a .cpp file, it'd help build times a little :)
 */

#pragma once

#define LIBASYNC_CUSTOM_DEFAULT_SCHEDULER 1;

// Declare async::default_scheduler with csp_scheduler
class csp_scheduler;

namespace async
{
csp_scheduler& default_scheduler();
}

#include "CSP/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsResult.h"
#include "CSP/Systems/WebService.h"
#include "CallHelpers.h"
#include "Debug/Logging.h"
#include "Multiplayer/ErrorCodeStrings.h"
#include <async++.h>
#include <memory>
#include <optional>
#include <signalrclient/signalr_value.h>
#include <string>
#include <type_traits>

// Custom version of the async::inline_scheduler.
class csp_scheduler
{
public:
    static void schedule(async::task_run_handle t) { t.run(); }
};

// Set our custom scheduler as the default,
// so we dont have to repeat async::inline_scheduler() in each .then() call.
namespace async
{
inline csp_scheduler& default_scheduler()
{
    static csp_scheduler s;
    return s;
}
}

namespace csp::common::continuations
{

using namespace csp::systems;

/*
 * Print an error with provided error context objects and HTTP request status information, and throw a cancellation error.
 * Calls the main callback as an error before throwing.
 */
template <typename ErrorResultT>
inline void LogHTTPErrorAndCancelContinuation(std::function<void(const ErrorResultT&)> Callback, std::string ErrorMsg, EResultCode ResultCode,
    csp::web::EResponseCodes HttpResultCode, ERequestFailureReason FailureReason, csp::systems::LogLevel LogLevel = csp::systems::LogLevel::Log)
{
    CSP_LOG_MSG(LogLevel, ErrorMsg.c_str());
    ErrorResultT FailureResult(ResultCode, HttpResultCode, FailureReason);
    INVOKE_IF_NOT_NULL(Callback, FailureResult);
    throw std::runtime_error("Continuation cancelled"); // Cancels the continuation chain.
}

/*
 * Print an error with provided string, and throw a cancellation error.
 */
inline void LogErrorAndCancelContinuation(std::string ErrorMsg, csp::systems::LogLevel LogLevel = csp::systems::LogLevel::Log)
{
    CSP_LOG_MSG(LogLevel, ErrorMsg.c_str());
    throw std::runtime_error("Continuation cancelled"); // Cancels the continuation chain.
}

/*
 * Checks the result code of a (passed by continuation) result object.
 * If not a success, logs an error and aborts continuation.
 * Otherwise, logs a success message and continues, forwarding the result to the next continuation.
 * Error context objects are optional, if unset, the values from the result object will be used.
 */
template <typename ResultT, typename ErrorResultT>
inline auto AssertRequestSuccessOrErrorFromResult(std::function<void(const ErrorResultT&)> Callback, std::string SuccessMsg, std::string ErrorMsg,
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
            LogHTTPErrorAndCancelContinuation<ErrorResultT>(
                Callback, std::move(ErrorMsg), ResultCodeToUse, HTTPResultCodeToUse, FailureReasonToUse, LogLevel);
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
template <typename ErrorResultT>
inline auto AssertRequestSuccessOrErrorFromErrorCode(std::function<void(const ErrorResultT&)> Callback, std::string SuccessMsg,
    std::optional<EResultCode> ResultCode, std::optional<csp::web::EResponseCodes> HttpResultCode, std::optional<ERequestFailureReason> FailureReason,
    csp::systems::LogLevel LogLevel = csp::systems::LogLevel::Log)
{
    return [Callback, SuccessMsg = std::move(SuccessMsg), ResultCode, HttpResultCode, FailureReason, LogLevel](
               const std::optional<csp::multiplayer::ErrorCode>& ErrorCode)
    {
        if (ErrorCode.has_value())
        {
            // Error Case. We have an error message, abort
            std::string ErrorMsg = std::string("Operation errored with error code: ") + csp::multiplayer::ErrorCodeToString(ErrorCode.value());
            LogHTTPErrorAndCancelContinuation<ErrorResultT>(Callback, std::move(ErrorMsg), ResultCode.value_or(EResultCode::Failed),
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
template <typename ResultT> inline auto ReportSuccess(std::function<void(const ResultT&)> Callback, std::string SuccessMsg)
{
    return [Callback, SuccessMsg = std::move(SuccessMsg)]()
    {
        /* Continuation was a success. We're done! */
        CSP_LOG_MSG(LogLevel::Log, SuccessMsg.c_str());
        ResultT SuccessResult(EResultCode::Success, csp::web::EResponseCodes::ResponseOK, ERequestFailureReason::None);
        INVOKE_IF_NOT_NULL(Callback, SuccessResult);
    };
}

/* Print a success message and report and send a result via the callback */
template <typename ResultT> inline auto SendResult(std::function<void(const ResultT&)> Callback, std::string SuccessMsg)
{
    return [Callback, SuccessMsg = std::move(SuccessMsg)](const ResultT& Result)
    {
        /* Continuation was a success. We're done! */
        CSP_LOG_MSG(LogLevel::Log, SuccessMsg.c_str());
        INVOKE_IF_NOT_NULL(Callback, Result);
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

/* Stores the result in a shared pointer for access outside of continuation */
template <typename ResultT> inline auto GetResultFromContinuation(std::shared_ptr<ResultT>& Ptr)
{
    return [Ptr](const ResultT& Result)
    {
        *Ptr = Result;
        return Result;
    };
}

/* Continuations out of SignalR Invoke come back as a task<tuple<value, exception_ptr>>
   This function transforms that into just a value, rethrowing the exception if it's populated. */
template <bool ForwardResult = true>
auto UnwrapSignalRResultOrThrow()
    -> std::conditional_t<ForwardResult, std::function<signalr::value(std::tuple<const signalr::value&, std::exception_ptr>)>,
        std::function<void(std::tuple<const signalr::value&, std::exception_ptr>)>>
{
    return [](std::tuple<const signalr::value&, std::exception_ptr> Results)
    {
        auto [Result, Exception] = Results;
        if (Exception)
        {
            std::rethrow_exception(Exception);
        }

        if constexpr (ForwardResult)
        {
            return Result;
        }
    };
}

//"Private" namespace for testing, to allow us not to link async++ in tests,
// for the few tests where we want to strictly test mechanisms.
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
            async::spawn([]() { return; }).then(InvokeIfExceptionInChain(std::forward<ExceptionHandlerCallable>(ExceptionHandler)));
        }

        template <typename ExceptionHandlerCallable>
        inline void SpawnChainThatThrowsGeneralExceptionWithHandlerAtEnd(ExceptionHandlerCallable&& ExceptionHandler)
        {
            async::spawn([]() { throw std::runtime_error(""); })
                .then(InvokeIfExceptionInChain(std::forward<ExceptionHandlerCallable>(ExceptionHandler)));
        }

        template <typename ExceptionHandlerCallable>
        inline void SpawnChainThatCallsLogHTTPErrorAndCancelContinuationWithHandlerAtEnd(
            ExceptionHandlerCallable&& ExceptionHandler, NullResultCallback ResultCallback)
        {
            async::spawn(
                [ResultCallback]()
                {
                    LogHTTPErrorAndCancelContinuation(
                        ResultCallback, "", EResultCode::Failed, csp::web::EResponseCodes::ResponseInit, ERequestFailureReason::Unknown);
                })
                .then(InvokeIfExceptionInChain(std::forward<ExceptionHandlerCallable>(ExceptionHandler)));
        }

        template <typename IntermediateStepCallable, typename ExceptionHandlerCallable>
        inline void SpawnChainThatCallsLogHTTPErrorAndCancelContinuationWithIntermediateStepAndHandlerAtEnd(
            IntermediateStepCallable&& IntermediateStep, ExceptionHandlerCallable&& ExceptionHandler, NullResultCallback ResultCallback)
        {
            async::spawn(
                [ResultCallback]()
                {
                    LogHTTPErrorAndCancelContinuation(
                        ResultCallback, "", EResultCode::Failed, csp::web::EResponseCodes::ResponseInit, ERequestFailureReason::Unknown);
                })
                .then(std::forward<IntermediateStepCallable>(IntermediateStep))
                .then(InvokeIfExceptionInChain(std::forward<ExceptionHandlerCallable>(ExceptionHandler)));
        }
    }
}
} // namespace csp::common::continuations
