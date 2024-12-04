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
