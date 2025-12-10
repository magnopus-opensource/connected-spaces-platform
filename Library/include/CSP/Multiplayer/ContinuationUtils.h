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
#include "CSP/Common/SharedEnums.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "MultiPlayerConnection.h"
#include <signalrclient/signalr_value.h>

#include <string>

namespace csp::multiplayer::continuations
{

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

        if (Exception != nullptr)
        {
            auto [Error, ExceptionErrorMsg] = MultiplayerConnection::ParseMultiplayerErrorFromExceptionPtr(Exception);

            // Throw an ErrorException using the parsed data instead of the original
            throw csp::common::continuations::ErrorCodeException(Error, "Multiplayer Error. " + ExceptionErrorMsg);
        }

        if constexpr (ForwardResult)
        {
            return Result;
        }
    };
}

}

CSP_END_IGNORE
