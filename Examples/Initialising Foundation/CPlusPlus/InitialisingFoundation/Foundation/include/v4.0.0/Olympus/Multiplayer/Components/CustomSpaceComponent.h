#pragma once
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/ComponentBase.h"
#include "Olympus/OlympusCommon.h"

namespace oly_multiplayer
{
enum class CustomComponentPropertyKeys
{
    ApplicationOrigin,
    Num
};

/// @ingroup CustomSpaceComponent
/// @brief Data representation of an CustomSpaceComponent.
class OLY_API CustomSpaceComponent : public ComponentBase
{
public:
    CustomSpaceComponent(SpaceEntity* Parent);

    const oly_common::String& GetApplicationOrigin() const;
    void SetApplicationOrigin(const oly_common::String& Value);

    bool HasCustomProperty(const oly_common::String& Key) const;
    const ReplicatedValue& GetCustomProperty(const oly_common::String& Key) const;
    void SetCustomProperty(const oly_common::String& Key, const ReplicatedValue& Value);
    void RemoveCustomProperty(const oly_common::String& Key);
    oly_common::List<oly_common::String> GetCustomPropertyKeys() const;

    int32_t GetNumProperties() const;
    uint32_t GetCustomPropertySubscriptionKey(const oly_common::String& Key);

private:
    void AddKey(const oly_common::String& Key);
    void RemoveKey(const oly_common::String& Key);
};

} // namespace oly_multiplayer
