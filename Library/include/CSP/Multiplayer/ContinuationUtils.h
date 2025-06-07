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

#include "CSP/Common/ContinuationUtils.h"
#include "CSP/Systems/Log/LogSystem.h" //BANNED
#include "CSP/Systems/WebService.h" //BANNED
#include "CSP/Web/HTTPResponseCodes.h" //BANNED
#include "Multiplayer/ErrorCodeStrings.h"
#include <signalrclient/signalr_value.h>

namespace csp::multiplayer::continuations
{
/*
 * Checks the multiplayer::Errorcode of a (passed by continuation) code.
 * If not a success, logs an error and aborts continuation.
 * Otherwise, logs a success message and continues. Does not pass anything to the next continuation.
 * Error context objects are optional, if unset, default values Failed, 500, and Unknown are used
 */
template <typename ErrorResultT>
inline auto AssertRequestSuccessOrErrorFromMultiplayerErrorCode(std::function<void(const ErrorResultT&)> Callback, std::string SuccessMsg,
    ErrorResultT ErrorResult, csp::systems::LogLevel LogLevel = csp::systems::LogLevel::Log)
{
    return [Callback, SuccessMsg = std::move(SuccessMsg), ErrorResult = std::move(ErrorResult), LogLevel](
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
            csp::common::continuations::LogErrorAndCancelContinuation(std::move(ErrorMsg), LogLevel);
        }
        else
        {
            // Success Case
            CSP_LOG_MSG(csp::systems::LogLevel::Log, SuccessMsg.c_str());
        }
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

}

CSP_END_IGNORE
