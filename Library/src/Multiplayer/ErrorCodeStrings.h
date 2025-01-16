/*
 * Copyright 2023 Magnopus LLC

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
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include <string>

namespace csp::multiplayer
{
static inline std::string ErrorCodeToString(csp::multiplayer::ErrorCode ErrorCode)
{
    std::string ErrorCodeString;
    switch (ErrorCode)
    {
    case csp::multiplayer::ErrorCode::None:
    {
        ErrorCodeString = "None";
        break;
    }
    case csp::multiplayer::ErrorCode::Unknown:
    {
        ErrorCodeString = "Unknown";
        break;
    }
    case csp::multiplayer::ErrorCode::NotConnected:
    {
        ErrorCodeString = "NotConnected";
        break;
    }
    case csp::multiplayer::ErrorCode::AlreadyConnected:
    {
        ErrorCodeString = "AlreadyConnected";
        break;
    }
    case csp::multiplayer::ErrorCode::SpaceUserLimitExceeded:
    {
        ErrorCodeString = "SpaceUserLimitExceeded";
        break;
    }
    default:
    {
        ErrorCodeString = std::string("Unknown error code. Value") + std::to_string(static_cast<unsigned int>(ErrorCode));
        break;
    }
    }

    return ErrorCodeString;
}
};
