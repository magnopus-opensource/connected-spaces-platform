#pragma once

#include "Olympus/OlympusCommon.h"

namespace oly_multiplayer
{

OLY_INTERFACE class OLY_API IVisibleComponent
{
public:
    virtual bool GetIsVisible() const = 0;
    virtual void SetIsVisible(bool InValue) = 0;

    virtual bool GetIsARVisible() const = 0;
    virtual void SetIsARVisible(bool InValue) = 0;

protected:
    virtual ~IVisibleComponent() = default;
};

} // namespace oly_multiplayer
