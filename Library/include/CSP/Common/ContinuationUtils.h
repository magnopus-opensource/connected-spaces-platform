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
    ResultException(const std::string& Message, const csp::systems::ResultBase& Result)
        : ExpectedExceptionBase(Message)
        , Result(Result)
    {
    }

    const ExceptionType GetExceptionType() const override { return ExceptionType::Result; }

    const csp::systems::ResultBase& GetResult() const { return Result; }

private:
    const csp::systems::ResultBase Result;
};

template <typename T> const inline T GetResultExceptionOrInvalid(const csp::common::continuations::ExpectedExceptionBase& Exception)
{
    // Check that the exception base is of type result and cast to derived type.
    if (Exception.GetExceptionType() == csp::common::continuations::ExceptionType::Result)
    {
        const auto ResultException = static_cast<const csp::common::continuations::ResultException*>(&Exception);
        const auto Result = ResultException->GetResult();
        return T(Result.GetResultCode(), static_cast<csp::web::EResponseCodes>(Result.GetHttpResultCode()), Result.GetFailureReason());
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
    explicit ErrorCodeException(csp::multiplayer::ErrorCode Code, const std::string& Message)
        : ExpectedExceptionBase(Message)
        , m_Code(Code)
    {
    }

    const ExceptionType GetExceptionType() const override { return ExceptionType::Multiplayer; }

    csp::multiplayer::ErrorCode Code() const noexcept { return m_Code; }

private:
    csp::multiplayer::ErrorCode m_Code;
};

/*
 * Print an error with provided string, and throw a cancellation error.
 */
inline void LogErrorAndCancelContinuation(std::string ErrorMsg, common::LogSystem& LogSystem)
{
    LogSystem.LogMsg(csp::common::LogLevel::Error, ErrorMsg.c_str());
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
    csp::common::LogSystem& LogSystem, ExpectedCallable&& InvokeIfExpectedErrorCallable, UnexpectedCallable&& InvokeIfUnexpectedErrorCallable)
{
    static_assert(std::is_invocable_v<ExpectedCallable, const ExpectedExceptionBase&>,
        "InvokeIfExpectedErrorCallable must be invocable with a single const ExpectedExceptionBase& arg, if you need other state, try passing a "
        "capturing lambda.");

    static_assert(std::is_invocable_v<UnexpectedCallable, const std::exception&>,
        "InvokeIfUnexpectedErrorCallable must be invocable with a single const std::exception& arg, if you need other state, try passing a capturing "
        "lambda.");

    return [InvokeIfExpectedErrorCallable = std::forward<ExpectedCallable>(InvokeIfExpectedErrorCallable),
               InvokeIfUnexpectedErrorCallable = std::forward<UnexpectedCallable>(InvokeIfUnexpectedErrorCallable),
               &LogSystem](async::task<void> ExceptionTask)
    {
        try
        {
            ExceptionTask.get();
        }
        catch (const csp::common::InvalidInterfaceUseError& exception)
        {
            // Keep specific custom catches here for other types.
            LogSystem.LogMsg(LogLevel::Verbose, "Error, CSP expects derived IRealtimeEngine type, but has received a base instantiation.");
            InvokeIfUnexpectedErrorCallable(std::runtime_error(exception.msg));
        }
        catch (const ExpectedExceptionBase& ExpectedException)
        {
            LogSystem.LogMsg(LogLevel::Verbose, "Caught expected exception during async++ chain. Invoking callable from InvokeIfExceptionInChain");
            LogSystem.LogMsg(LogLevel::Verbose, ExpectedException.what());
            InvokeIfExpectedErrorCallable(ExpectedException);
        }
        catch (const std::exception& exception)
        {
            // Catches all other standard exceptions (the generic fallback).
            LogSystem.LogMsg(LogLevel::Fatal, "Caught unexpected exception during async++ chain. Invoking callable from InvokeIfExceptionInChain");
            InvokeIfUnexpectedErrorCallable(exception);
        }
    };
}

template <typename ExpectedCallable>
inline auto InvokeIfExceptionInChain(csp::common::LogSystem& LogSystem, ExpectedCallable&& InvokeIfExpectedErrorCallable)
{
    return InvokeIfExceptionInChain(
        LogSystem, std::forward<ExpectedCallable>(InvokeIfExpectedErrorCallable), []([[maybe_unused]] const std::exception& exception) {});
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
inline auto AssertRequestSuccessOrErrorFromMultiplayerErrorCode(std::string SuccessMsg, ErrorResultT ErrorResult, csp::common::LogSystem& LogSystem)
{
    return [SuccessMsg = std::move(SuccessMsg), ErrorResult = std::move(ErrorResult), &LogSystem](
               const std::optional<csp::multiplayer::ErrorCode>& ErrorCode)
    {
        if (ErrorCode.has_value())
        {
            // Error Case. We have an error message, abort
            std::string ErrorMsg = std::string("Operation errored with error code: ") + csp::multiplayer::ErrorCodeToString(ErrorCode.value());
            csp::common::continuations::LogErrorAndCancelContinuation(std::move(ErrorMsg), LogSystem);
        }
        else
        {
            // Success Case
            LogSystem.LogMsg(csp::common::LogLevel::Log, SuccessMsg.c_str());
        }
    };
}

}

CSP_END_IGNORE
