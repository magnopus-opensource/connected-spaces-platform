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
#include "CSP/Common/Interfaces/InvalidInterfaceUserError.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsResult.h"
#include "CSP/Systems/WebService.h"

namespace csp::common::continuations
{

enum class ExceptionType
{
    Result,
    Multiplayer
};

/**
 * @brief Serves as the base class for all expected, or business-logic, exceptions.
 * @details This class provides a common interface for exceptions that are
 * part of the normal program flow, distinguishing them from unexpected system-level
 * errors like out-of-memory or file not found.
 */
class ExpectedExceptionBase : public std::runtime_error
{
public:
    ExpectedExceptionBase(const std::string& message)
        : std::runtime_error(message)
    {
    }

    virtual const ExceptionType GetExceptionType() const = 0;
};

/**
 * @brief An exception class for API request results.
 * @details This template class captures the result object alongside the
 * exception. It's ideal for handling errors from HTTP requests where specific
 * result codes, HTTP status, and failure reasons need to be preserved.
 */
class ResultException : public ExpectedExceptionBase
{
public:
    template <typename T>
    ResultException(const std::string& message, const T& result)
        : ExpectedExceptionBase(message)
        , m_result(std::make_shared<T>(result))
    {
    }

    const ExceptionType GetExceptionType() const override { return ExceptionType::Result; }

    const csp::systems::ResultBase& GetResult() const { return *m_result; }

private:
    // Throwing an exception copy-initializes a temporary object, meaning unique_ptr will fail due to enforcing exclusive ownership.
    std::shared_ptr<const csp::systems::ResultBase> m_result;
};

template <typename T> const inline T GetResultExceptionOrInvalid(const csp::common::continuations::ExpectedExceptionBase& exception)
{
    // Check that the exception base is of type result and cast to derived type.
    if (exception.GetExceptionType() == csp::common::continuations::ExceptionType::Result)
    {
        const auto resultException = static_cast<const csp::common::continuations::ResultException*>(&exception);
        const auto result = resultException->GetResult();
        return T(result.GetResultCode(), static_cast<csp::web::EResponseCodes>(result.GetHttpResultCode()), result.GetFailureReason());
    }

    return T(csp::systems::EResultCode::Failed, 0);
}

/**
 * @brief An exception class for Multiplayer Error code.
 * @details This template class captures the Multiplayer Error code object alongside the
 * exception. It's ideal for handling errors from Multiplayer where specific
 * error codes need to be preserved.
 */
class ErrorCodeException : public ExpectedExceptionBase
{
public:
    explicit ErrorCodeException(csp::multiplayer::ErrorCode code, const std::string& message)
        : ExpectedExceptionBase(message)
        , m_code(code)
    {
    }

    const ExceptionType GetExceptionType() const override { return ExceptionType::Multiplayer; }

    csp::multiplayer::ErrorCode Code() const noexcept { return m_code; }

private:
    csp::multiplayer::ErrorCode m_code;
};

/*
 * Print an error with provided string, and throw a cancellation error.
 */
inline void LogErrorAndCancelContinuation(std::string errorMsg, common::LogSystem& logSystem)
{
    logSystem.LogMsg(csp::common::LogLevel::Error, errorMsg.c_str());
    throw std::runtime_error("Continuation cancelled"); // Cancels the continuation chain.
}

/*
 * Intended to be placed at the end of an async++ continuation chain.
 * If the chain throws an unhandled exception, this will attempt to unwrap it,
 * and call a passed in callable, (probably a state-reset or cleanup function of some sort).
 * The callable is perfectly forwarded.
 */
template <typename ExpectedCallable, typename UnexpectedCallable>
inline auto InvokeIfExceptionInChain(
    csp::common::LogSystem& logSystem, ExpectedCallable&& invokeIfExpectedErrorCallable, UnexpectedCallable&& invokeIfUnexpectedErrorCallable)
{
    static_assert(std::is_invocable_v<ExpectedCallable, const ExpectedExceptionBase&>,
        "InvokeIfExpectedErrorCallable must be invocable with a single const ExpectedExceptionBase& arg, if you need other state, try passing a "
        "capturing lambda.");

    static_assert(std::is_invocable_v<UnexpectedCallable, const std::exception&>,
        "InvokeIfUnexpectedErrorCallable must be invocable with a single const std::exception& arg, if you need other state, try passing a capturing "
        "lambda.");

    return [invokeIfExpectedErrorCallable = std::forward<ExpectedCallable>(invokeIfExpectedErrorCallable),
               invokeIfUnexpectedErrorCallable = std::forward<UnexpectedCallable>(invokeIfUnexpectedErrorCallable),
               &logSystem](async::task<void> exceptionTask)
    {
        try
        {
            exceptionTask.get();
        }
        catch (const csp::common::InvalidInterfaceUseError& exception)
        {
            // Keep specific custom catches here for other types.
            logSystem.LogMsg(LogLevel::Verbose, "Error, CSP expects derived IRealtimeEngine type, but has received a base instantiation.");
            invokeIfUnexpectedErrorCallable(std::runtime_error(exception.msg));
        }
        catch (const ExpectedExceptionBase& expectedException)
        {
            logSystem.LogMsg(LogLevel::Verbose, "Caught expected exception during async++ chain. Invoking callable from InvokeIfExceptionInChain");
            logSystem.LogMsg(LogLevel::Verbose, expectedException.what());
            invokeIfExpectedErrorCallable(expectedException);
        }
        catch (const std::exception& exception)
        {
            // Catches all other standard exceptions (the generic fallback).
            logSystem.LogMsg(LogLevel::Fatal, "Caught unexpected exception during async++ chain. Invoking callable from InvokeIfExceptionInChain");
            invokeIfUnexpectedErrorCallable(exception);
        }
    };
}

template <typename ExpectedCallable>
inline auto InvokeIfExceptionInChain(csp::common::LogSystem& logSystem, ExpectedCallable&& invokeIfExpectedErrorCallable)
{
    return InvokeIfExceptionInChain(
        logSystem, std::forward<ExpectedCallable>(invokeIfExpectedErrorCallable), []([[maybe_unused]] const std::exception& exception) {});
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
inline auto AssertRequestSuccessOrErrorFromMultiplayerErrorCode(std::string successMsg, ErrorResultT errorResult, csp::common::LogSystem& logSystem)
{
    return [successMsg = std::move(successMsg), errorResult = std::move(errorResult), &logSystem](
               const std::optional<csp::multiplayer::ErrorCode>& errorCode)
    {
        if (errorCode.has_value())
        {
            // Error Case. We have an error message, abort
            std::string errorMsg = std::string("Operation errored with error code: ") + csp::multiplayer::ErrorCodeToString(errorCode.value());
            csp::common::continuations::LogErrorAndCancelContinuation(std::move(errorMsg), logSystem);
        }
        else
        {
            // Success Case
            logSystem.LogMsg(csp::common::LogLevel::Log, successMsg.c_str());
        }
    };
}

}

CSP_END_IGNORE
