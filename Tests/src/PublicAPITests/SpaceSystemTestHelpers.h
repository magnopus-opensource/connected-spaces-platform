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
#include "CSP/Systems/Spaces/SpaceSystem.h"

void CreateSpace(csp::systems::SpaceSystem* spaceSystem, const csp::common::String& name, const csp::common::String& description,
    csp::systems::SpaceAttributes spaceAttributes,
    const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& spaceMetadata,
    const csp::common::Optional<csp::systems::InviteUserRoleInfoCollection>& inviteUsers,
    const csp::common::Optional<csp::systems::FileAssetDataSource>& thumbnail,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags, csp::systems::Space& outSpace);
void CreateSpaceWithBuffer(csp::systems::SpaceSystem* spaceSystem, const csp::common::String& name, const csp::common::String& description,
    csp::systems::SpaceAttributes spaceAttributes,
    const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& spaceMetadata,
    const csp::common::Optional<csp::systems::InviteUserRoleInfoCollection>& inviteUsers, csp::systems::BufferAssetDataSource& thumbnail,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags, csp::systems::Space& outSpace);
void DeleteSpace(csp::systems::SpaceSystem* spaceSystem, const csp::common::String& spaceId);
void GetSpace(csp::systems::SpaceSystem* spaceSystem, const csp::common::String& spaceId, csp::systems::Space& outSpace);
void CreateDefaultTestSpace(csp::systems::SpaceSystem* spaceSystem, csp::systems::Space& outSpace,
    csp::systems::SpaceAttributes attributes = csp::systems::SpaceAttributes::Private);