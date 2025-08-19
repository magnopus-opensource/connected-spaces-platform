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

CSP_START_IGNORE

#include <optional>

#include "CSP/Common/ContinuationUtils.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsResult.h"
#include "CSP/Systems/WebService.h"
#include "Debug/Logging.h"

namespace csp::systems::continuations
{
using namespace csp::systems;

/*
 * Print an error with provided error context objects and HTTP request status information, and throw a cancellation error.
 */
template <typename ResultT>
inline void LogHTTPErrorAndCancelContinuation(std::string ErrorMsg, ResultT Result, csp::common::LogLevel LogLevel = csp::common::LogLevel::Log)
{
    CSP_LOG_MSG(LogLevel, ErrorMsg.c_str());
    throw csp::common::continuations::ResultException("Continuation cancelled", Result); // Cancels the continuation chain.
}

/*
 * Checks the result code of a (passed by continuation) result object.
 * If not a success, logs an error and aborts continuation.
 * Otherwise, logs a success message and continues, forwarding the result to the next continuation.
 * Error context objects are optional, if unset, the values from the result object will be used.
 */
template <typename ResultT>
inline auto AssertRequestSuccessOrErrorFromResult(std::string SuccessMsg, std::string ErrorMsg, std::optional<EResultCode> ResultCode,
    std::optional<csp::web::EResponseCodes> HttpResultCode, std::optional<ERequestFailureReason> FailureReason,
    csp::common::LogLevel LogLevel = csp::common::LogLevel::Log)
{
    return [SuccessMsg = std::move(SuccessMsg), ErrorMsg = std::move(ErrorMsg), ResultCode, HttpResultCode, FailureReason, LogLevel](
               const ResultT& Result)
    {
        if (Result.GetResultCode() != EResultCode::Success)
        {
            auto ResultCodeToUse = ResultCode.value_or(Result.GetResultCode());
            auto HTTPResultCodeToUse = HttpResultCode.value_or(static_cast<csp::web::EResponseCodes>(Result.GetHttpResultCode()));
            auto FailureReasonToUse = FailureReason.value_or(Result.GetFailureReason());
            ResultT InternalResult(ResultCodeToUse, HTTPResultCodeToUse, FailureReasonToUse);
            LogHTTPErrorAndCancelContinuation<ResultT>(std::move(ErrorMsg), InternalResult, LogLevel);
        }
        else
        {
            // Success Case
            CSP_LOG_MSG(csp::common::LogLevel::Log, SuccessMsg.c_str());
        }
        return Result;
    };
}

/* Print a success message and report a successfull result via the callback */
template <typename ResultT> inline auto ReportSuccess(std::function<void(const ResultT&)> Callback, std::string SuccessMsg)
{
    return [Callback, SuccessMsg = std::move(SuccessMsg)]()
    {
        /* Continuation was a success. We're done! */
        CSP_LOG_MSG(csp::common::LogLevel::Log, SuccessMsg.c_str());
        ResultT SuccessResult(EResultCode::Success, csp::web::EResponseCodes::ResponseOK, ERequestFailureReason::None);
        if (Callback)
        {
            Callback(SuccessResult);
        }
    };
}

/* Print a success message and report and send a result via the callback */
template <typename ResultT> inline auto SendResult(std::function<void(const ResultT&)> Callback, std::string SuccessMsg)
{
    return [Callback, SuccessMsg = std::move(SuccessMsg)](const ResultT& Result)
    {
        /* Continuation was a success. We're done! */
        CSP_LOG_MSG(csp::common::LogLevel::Log, SuccessMsg.c_str());
        if (Callback)
        {
            Callback(Result);
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
            async::spawn([]() { return; })
                .then(csp::common::continuations::InvokeIfExceptionInChain(*csp::systems::SystemsManager::Get().GetLogSystem(),
                    std::forward<ExceptionHandlerCallable>(ExceptionHandler), []([[maybe_unused]] const std::exception& exception) {}));
        }

        template <typename ExpectedHandlerCallable, typename UnexpectedHandlerCallable, typename ExceptionThrowable>
        inline void SpawnChainThatThrowsGeneralExceptionWithHandlerAtEnd(
            ExpectedHandlerCallable&& ExpectedHandler, UnexpectedHandlerCallable&& UnexpectedHandler, ExceptionThrowable&& Throwable)
        {
            async::spawn([Throwable]() { throw Throwable; })
                .then(csp::common::continuations::InvokeIfExceptionInChain(*csp::systems::SystemsManager::Get().GetLogSystem(),
                    std::forward<ExpectedHandlerCallable>(ExpectedHandler), std::forward<UnexpectedHandlerCallable>(UnexpectedHandler)));
        }

        template <typename ExceptionHandlerCallable>
        inline void SpawnChainThatCallsLogHTTPErrorAndCancelContinuationWithHandlerAtEnd(ExceptionHandlerCallable&& ExceptionHandler)
        {
            async::spawn(
                []()
                {
                    NullResult Result(EResultCode::Failed, csp::web::EResponseCodes::ResponseInit, ERequestFailureReason::Unknown);
                    LogHTTPErrorAndCancelContinuation<NullResult>("", Result);
                })
                .then(csp::common::continuations::InvokeIfExceptionInChain(*csp::systems::SystemsManager::Get().GetLogSystem(),
                    std::forward<ExceptionHandlerCallable>(ExceptionHandler), []([[maybe_unused]] const std::exception& exception) {}));
        }

        template <typename IntermediateStepCallable, typename ExceptionHandlerCallable>
        inline void SpawnChainThatCallsLogHTTPErrorAndCancelContinuationWithIntermediateStepAndHandlerAtEnd(
            IntermediateStepCallable&& IntermediateStep, ExceptionHandlerCallable&& ExceptionHandler)
        {
            async::spawn(
                []()
                {
                    NullResult Result(EResultCode::Failed, csp::web::EResponseCodes::ResponseInit, ERequestFailureReason::Unknown);
                    LogHTTPErrorAndCancelContinuation<NullResult>("", Result);
                })
                .then(std::forward<IntermediateStepCallable>(IntermediateStep))
                .then(csp::common::continuations::InvokeIfExceptionInChain(*csp::systems::SystemsManager::Get().GetLogSystem(),
                    std::forward<ExceptionHandlerCallable>(ExceptionHandler), []([[maybe_unused]] const std::exception& exception) {}));
        }
    }
}

}

CSP_END_IGNORE
