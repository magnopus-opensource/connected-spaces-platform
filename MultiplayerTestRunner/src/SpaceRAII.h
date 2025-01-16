/*
 * Copyright 2024 Magnopus LLC

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

#include "CSP/Systems/Spaces/SpaceSystem.h"

#include <optional>
#include <string>

/*
 * RAII container object to facilitate automatically cleaning up a space when leaving scope
 * This involves 2 actions, leaving the space, and destroying the space.
 * This type will create a new random space upon construction if a SpaceId is not set.
 * If a SpaceID is set, this type only joins and leaves the room, not destroying it, as it is assumed to already exist.
 */
class SpaceRAII
{
public:
    SpaceRAII(std::optional<std::string> SpaceId);
    ~SpaceRAII();

    /*
     * The SpaceID of the space this object is managing.
     * If the object was constructed with an empty SpaceId, then this will be the ID of the newly created space.
     */
    std::string GetSpaceId();

    /*
     * Build a default space with a random name.
     * Static method for test convenience.
     */
    static csp::systems::Space SpaceRAII::CreateDefaultTestSpace(csp::systems::SpaceSystem& SpaceSystem);

private:
    bool CreatedThisSpace = false; // If we created this space, we should destroy it when done.
    std::string SpaceId;
};
