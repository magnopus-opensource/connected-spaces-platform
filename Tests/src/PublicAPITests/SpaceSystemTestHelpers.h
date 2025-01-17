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

void CreateSpace(csp::systems::SpaceSystem* SpaceSystem, const csp::common::String& Name, const csp::common::String& Description,
    csp::systems::SpaceAttributes SpaceAttributes,
    const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& SpaceMetadata,
    const csp::common::Optional<csp::systems::InviteUserRoleInfoCollection>& InviteUsers,
    const csp::common::Optional<csp::systems::FileAssetDataSource>& Thumbnail,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags, csp::systems::Space& OutSpace);
void CreateSpaceWithBuffer(csp::systems::SpaceSystem* SpaceSystem, const csp::common::String& Name, const csp::common::String& Description,
    csp::systems::SpaceAttributes SpaceAttributes,
    const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& SpaceMetadata,
    const csp::common::Optional<csp::systems::InviteUserRoleInfoCollection>& InviteUsers, csp::systems::BufferAssetDataSource& Thumbnail,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags, csp::systems::Space& OutSpace);
void DeleteSpace(csp::systems::SpaceSystem* SpaceSystem, const csp::common::String& SpaceId);
void GetSpace(csp::systems::SpaceSystem* SpaceSystem, const csp::common::String& SpaceId, csp::systems::Space& OutSpace);
void CreateDefaultTestSpace(csp::systems::SpaceSystem* SpaceSystem, csp::systems::Space& OutSpace);