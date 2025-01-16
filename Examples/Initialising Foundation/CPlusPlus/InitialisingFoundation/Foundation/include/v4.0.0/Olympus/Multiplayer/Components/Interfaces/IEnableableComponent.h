#pragma once

#include "Olympus/OlympusCommon.h"

namespace oly_multiplayer
{

OLY_INTERFACE class OLY_API IEnableableComponent
{
public:
    virtual bool GetIsEnabled() const = 0;
    virtual void SetIsEnabled(bool InValue) = 0;

protected:
    virtual ~IEnableableComponent() = default;
};

} // namespace oly_multiplayer
