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

#include "CSP/Common/String.h"
#include "CSP/Common/Map.h"
#include "CSP/Systems/WebService.h"

#include <map>
#include <string>

namespace csp::systems
{
class LocalScriptResult;
}

namespace csp::services
{

class ApiResponseBase;

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

/// @brief Namespace that encompasses everything in the multiplayer system
namespace csp::systems
{
/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to download a collection of local script data.
class CSP_API LocalScriptResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retreives the LocalScriptResult from the result.
    const csp::common::Map<csp::common::String, csp::common::String>& GetLocalScripts() const;

    CSP_NO_EXPORT LocalScriptResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    LocalScriptResult(void*) {};

    void SetLocalScripts(const csp::common::Map<csp::common::String, csp::common::String>& LocalScripts);

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Map<csp::common::String, csp::common::String> LocalScripts;
};

/// @brief Callback containing a collection of local script data.
/// @param Result Map<String, String> : result class
typedef std::function<void(const LocalScriptResult& Result)> LocalScriptResultCallback;


} // namespace csp::systems