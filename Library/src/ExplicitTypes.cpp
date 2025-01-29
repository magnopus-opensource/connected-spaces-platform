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
#include "CSP/Common/Array.h"
#include "CSP/Common/List.h"
#include "CSP/Common/Optional.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/Spaces/Site.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Spaces/UserRoles.h"
#include "CSP/Systems/Spatial/Anchor.h"
#include "CSP/Systems/Spatial/PointOfInterest.h"
#include "CSP/Systems/Users/Profile.h"
#include "CSP/Systems/Users/ThirdPartyAuthentication.h"

/*
 * This file contains explicit template type instantiations so that types will be wrapped in C # or JS
 * without being stripped out, and to ensure that clients use Connected Spaces Platform's allocator for
 * template instances.
 */

// csp::common::Array
template class CSP_API csp::common::Array<csp::common::Map<csp::common::String, csp::common::String>>;
template class CSP_API csp::common::Array<csp::common::String>;
template class CSP_API csp::common::Array<csp::multiplayer::ComponentBase*>;
template class CSP_API csp::common::Array<csp::multiplayer::ComponentUpdateInfo>;
template class CSP_API csp::common::Array<csp::multiplayer::MessageInfo>;
template class CSP_API csp::common::Array<csp::multiplayer::ReplicatedValue>;
template class CSP_API csp::common::Array<csp::systems::Anchor>;
template class CSP_API csp::common::Array<csp::systems::AnchorResolution>;
template class CSP_API csp::common::Array<csp::systems::Asset>;
template class CSP_API csp::common::Array<csp::systems::AssetCollection>;
template class CSP_API csp::common::Array<csp::systems::BasicProfile>;
template class CSP_API csp::common::Array<csp::systems::BasicSpace>;
template class CSP_API csp::common::Array<csp::systems::EAssetPlatform>;
template class CSP_API csp::common::Array<csp::systems::EAssetType>;
template class CSP_API csp::common::Array<csp::systems::EThirdPartyAuthenticationProviders>;
template class CSP_API csp::common::Array<csp::systems::GeoLocation>;
template class CSP_API csp::common::Array<csp::systems::InviteUserRoleInfo>;
template class CSP_API csp::common::Array<csp::systems::PointOfInterest>;
template class CSP_API csp::common::Array<csp::systems::Site>;
template class CSP_API csp::common::Array<csp::systems::Space>;
template class CSP_API csp::common::Array<csp::systems::UserRoleInfo>;

// csp::common::List
template class CSP_API csp::common::List<csp::common::String>;
template class CSP_API csp::common::List<csp::common::Vector3>;

// csp::common::Map
template class CSP_API csp::common::Map<uint16_t, csp::multiplayer::ComponentBase*>;
template class CSP_API csp::common::Map<uint32_t, csp::multiplayer::ReplicatedValue>;
template class CSP_API csp::common::Map<csp::common::String, csp::common::Map<csp::common::String, csp::common::String>>;
template class CSP_API csp::common::Map<csp::common::String, csp::common::String>;

// csp::common::Optional
template class CSP_API csp::common::Optional<bool>;
template class CSP_API csp::common::Optional<float>;
template class CSP_API csp::common::Optional<int>;
template class CSP_API csp::common::Optional<csp::common::Array<csp::common::String>>;
template class CSP_API csp::common::Optional<csp::common::Array<csp::systems::EAssetType>>;
template class CSP_API csp::common::Optional<csp::common::Array<csp::systems::GeoLocation>>;
template class CSP_API csp::common::Optional<csp::common::Array<csp::systems::InviteUserRoleInfo>>;
template class CSP_API csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>;
template class CSP_API csp::common::Optional<csp::common::String>;
template class CSP_API csp::common::Optional<csp::systems::EAssetCollectionType>;
template class CSP_API csp::common::Optional<csp::systems::FileAssetDataSource>;
template class CSP_API csp::common::Optional<csp::systems::Space>;
template class CSP_API csp::common::Optional<csp::systems::SpaceAttributes>;
