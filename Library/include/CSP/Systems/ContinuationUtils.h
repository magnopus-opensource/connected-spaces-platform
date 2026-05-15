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
inline void LogHTTPErrorAndCancelContinuation(std::string errorMsg, ResultT result, csp::common::LogLevel logLevel = csp::common::LogLevel::Log)
{
    CSP_LOG_MSG(logLevel, errorMsg.c_str());
    throw csp::common::continuations::ResultException("Continuation cancelled", std::forward<ResultT>(result)); // Cancels the continuation chain.
}

/*
 * Checks the result code of a (passed by continuation) result object.
 * If not a success, logs an error and aborts continuation.
 * Otherwise, logs a success message and continues, forwarding the result to the next continuation.
 * Error context objects are optional, if unset, the values from the result object will be used.
 */
template <typename ResultT>
inline auto AssertRequestSuccessOrErrorFromResult(std::string successMsg, std::string errorMsg, std::optional<EResultCode> resultCode,
    std::optional<csp::web::EResponseCodes> httpResultCode, std::optional<ERequestFailureReason> failureReason,
    csp::common::LogLevel logLevel = csp::common::LogLevel::Log)
{
    return [successMsg = std::move(successMsg), errorMsg = std::move(errorMsg), resultCode, httpResultCode, failureReason, logLevel](
               const ResultT& result)
    {
        if (result.GetResultCode() != EResultCode::Success)
        {
            auto resultCodeToUse = resultCode.value_or(result.GetResultCode());
            auto httpResultCodeToUse = httpResultCode.value_or(static_cast<csp::web::EResponseCodes>(result.GetHttpResultCode()));
            auto failureReasonToUse = failureReason.value_or(result.GetFailureReason());
            ResultT internalResult(resultCodeToUse, httpResultCodeToUse, failureReasonToUse);
            LogHTTPErrorAndCancelContinuation<ResultT>(std::move(errorMsg), std::move(internalResult), logLevel);
        }
        else
        {
            // Success Case
            CSP_LOG_MSG(csp::common::LogLevel::Log, successMsg.c_str());
        }
        return result;
    };
}

/* Print a success message and report a successfull result via the callback */
template <typename ResultT> inline auto ReportSuccess(std::function<void(const ResultT&)> callback, std::string successMsg)
{
    return [callback, successMsg = std::move(successMsg)]()
    {
        /* Continuation was a success. We're done! */
        CSP_LOG_MSG(csp::common::LogLevel::Log, successMsg.c_str());
        ResultT successResult(EResultCode::Success, csp::web::EResponseCodes::ResponseOK, ERequestFailureReason::None);
        if (callback)
        {
            callback(successResult);
        }
    };
}

/* Print a success message and report and send a result via the callback */
template <typename ResultT> inline auto SendResult(std::function<void(const ResultT&)> callback, std::string successMsg)
{
    return [callback, successMsg = std::move(successMsg)](const ResultT& result)
    {
        /* Continuation was a success. We're done! */
        CSP_LOG_MSG(csp::common::LogLevel::Log, successMsg.c_str());
        if (callback)
        {
            callback(result);
        }
    };
}

/* Stores the result in a shared pointer for access outside of continuation */
template <typename ResultT> inline auto GetResultFromContinuation(std::shared_ptr<ResultT>& ptr)
{
    return [ptr](const ResultT& result)
    {
        *ptr = result;
        return result;
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
        inline void SpawnChainThatThrowsNoExceptionWithHandlerAtEnd(ExceptionHandlerCallable&& exceptionHandler)
        {
            async::spawn([]() { return; })
                .then(csp::common::continuations::InvokeIfExceptionInChain(
                    *csp::systems::SystemsManager::Get().GetLogSystem(), std::forward<ExceptionHandlerCallable>(exceptionHandler)));
        }

        template <typename ExpectedHandlerCallable, typename UnexpectedHandlerCallable, typename ExceptionThrowable>
        inline void SpawnChainThatThrowsGeneralExceptionWithHandlerAtEnd(
            ExpectedHandlerCallable&& expectedHandler, UnexpectedHandlerCallable&& unexpectedHandler, ExceptionThrowable&& throwable)
        {
            async::spawn([throwable]() { throw throwable; })
                .then(csp::common::continuations::InvokeIfExceptionInChain(*csp::systems::SystemsManager::Get().GetLogSystem(),
                    std::forward<ExpectedHandlerCallable>(expectedHandler), std::forward<UnexpectedHandlerCallable>(unexpectedHandler)));
        }

        template <typename ExceptionHandlerCallable>
        inline void SpawnChainThatCallsLogHTTPErrorAndCancelContinuationWithHandlerAtEnd(ExceptionHandlerCallable&& exceptionHandler)
        {
            async::spawn(
                []()
                {
                    NullResult result(EResultCode::Failed, csp::web::EResponseCodes::ResponseInit, ERequestFailureReason::Unknown);
                    LogHTTPErrorAndCancelContinuation<NullResult>("", result);
                })
                .then(csp::common::continuations::InvokeIfExceptionInChain(
                    *csp::systems::SystemsManager::Get().GetLogSystem(), std::forward<ExceptionHandlerCallable>(exceptionHandler)));
        }

        template <typename IntermediateStepCallable, typename ExceptionHandlerCallable>
        inline void SpawnChainThatCallsLogHTTPErrorAndCancelContinuationWithIntermediateStepAndHandlerAtEnd(
            IntermediateStepCallable&& intermediateStep, ExceptionHandlerCallable&& exceptionHandler)
        {
            async::spawn(
                []()
                {
                    NullResult result(EResultCode::Failed, csp::web::EResponseCodes::ResponseInit, ERequestFailureReason::Unknown);
                    LogHTTPErrorAndCancelContinuation<NullResult>("", result);
                })
                .then(std::forward<IntermediateStepCallable>(intermediateStep))
                .then(csp::common::continuations::InvokeIfExceptionInChain(
                    *csp::systems::SystemsManager::Get().GetLogSystem(), std::forward<ExceptionHandlerCallable>(exceptionHandler)));
        }
    }
}

}

CSP_END_IGNORE
