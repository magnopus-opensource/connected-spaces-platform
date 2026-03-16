#pragma once

#include "CSP/Multiplayer/SpaceEntity.h"

#include <optional>
#include <set>
#include <string>
#include <vector>

namespace csp::multiplayer
{

std::set<uint64_t> ResolveEntityIdsFromQueryJson(const std::string& QueryJson, const std::vector<SpaceEntity*>& Entities);
std::optional<uint64_t> ResolveEntityIdFromQueryJson(const std::string& QueryJson, const std::vector<SpaceEntity*>& Entities);

} // namespace csp::multiplayer
