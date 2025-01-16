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

#include "CSP/CSPCommon.h"

namespace csp::web
{

class WebClient;

} // namespace csp::web

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

namespace csp::systems
{

/// @brief System class for handling VOIP. Provides Connected Spaces Platform specific overidden functionality.
class CSP_API VoipSystem
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    friend void csp::memory::Delete<VoipSystem>(VoipSystem* Ptr);
    /** @endcond */
    CSP_END_IGNORE

public:
    /// @brief Mutes a local user. Not implemented.
    /// @param IsMuted
    void MuteLocalUser(bool IsMuted);
    /// @brief Checks if the user is muted. Not implemented.
    /// @return Is the user muted.
    bool IsLocalUserMuted() const;

private:
    VoipSystem();
    ~VoipSystem();
};

} // namespace csp::systems
