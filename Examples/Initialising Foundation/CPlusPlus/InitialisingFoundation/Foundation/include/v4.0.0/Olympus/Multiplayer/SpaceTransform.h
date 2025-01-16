#pragma once

#include "Olympus/Common/Vector.h"
#include "Olympus/OlympusCommon.h"

namespace oly_multiplayer
{

class OLY_API SpaceTransform
{
public:
    SpaceTransform();
    SpaceTransform(const oly_common::Vector3& Position, const oly_common::Vector4& Rotation, const oly_common::Vector3& Scale);

    oly_common::Vector3 Position;
    oly_common::Vector4 Rotation;
    oly_common::Vector3 Scale;
};

} // namespace oly_multiplayer
