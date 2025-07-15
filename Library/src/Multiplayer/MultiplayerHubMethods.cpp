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

#include "CSP//Multiplayer/MultiplayerHubMethods.h"

#include "CSP/Common/fmt_Formatters.h"
#include "Debug/Logging.h"

#include <fmt/format.h>

namespace csp::multiplayer
{

bool MultiplayerHubMethodMap::CheckPrerequisites(const csp::common::Array<csp::common::String>& MethodNames) const
{
    for (const auto& Method : *this)
    {
        // Validate that the current methods is in the available method names array
        if (const auto itt = std::find(MethodNames.begin(), MethodNames.end(), Method.second.c_str()); itt == MethodNames.end())
        {
            const auto Message = fmt::format("Failed to resolve the Multiplayer Hub Method: {0}", Method.second);

            CSP_LOG_MSG(csp::common::LogLevel::Fatal, Message.c_str());
            return false;
        }
    }

    return true;
}

} // namespace csp::multiplayer
