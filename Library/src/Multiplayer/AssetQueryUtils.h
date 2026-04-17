/*
 * Copyright 2026 Magnopus LLC

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

#include "CSP/Common/Array.h"
#include "CSP/Common/Optional.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"

#include <set>
#include <string>
#include <vector>

namespace csp::multiplayer
{

/// @brief Filter criteria extracted from a JS asset query JSON tree.
/// Leaf predicates are unioned into these fields; logical operators (and/or/not)
/// are resolved by post-filtering the fetched result set (same approach as EntityQueryUtils).
struct AssetQueryCriteria
{
    csp::common::Optional<csp::common::Array<csp::systems::EAssetType>> AssetTypes;
    csp::common::Optional<csp::common::Array<csp::common::String>> CollectionNames;
};

/// @brief Extract the union of filter criteria from a JSON asset query tree.
/// Supports kinds: "assetType", "collectionName", "and", "or", "not".
AssetQueryCriteria ExtractAssetQueryCriteria(const std::string& QueryJson);

/// @brief Evaluate the full query tree against assets (and their collections), returning
/// the subset that satisfies the predicate. Used for client-side filtering after
/// the network fetch has narrowed the result set via ExtractAssetQueryCriteria.
std::vector<csp::systems::Asset> EvaluateAssetQuery(const std::string& QueryJson, const std::vector<csp::systems::Asset>& Assets,
    const std::vector<csp::systems::AssetCollection>& Collections);

/// @brief Serialise an array of assets to a JSON array string.
/// Each entry has shape: {"id":"...","assetCollectionId":"...","name":"...","url":"...","type":N}
std::string SerializeAssetsToJson(const std::vector<csp::systems::Asset>& Assets);

} // namespace csp::multiplayer
