#pragma once
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/ComponentBase.h"
#include "Olympus/OlympusCommon.h"

namespace oly_multiplayer
{

enum class ScriptScope
{
    Local = 0,
    Owner,
    Num
};

enum class ScriptComponentPropertyKeys
{
    ScriptSource = 1,
    OwnerId,
    ScriptScope,
    Num
};

class OLY_API ScriptSpaceComponent : public ComponentBase
{
public:
    ScriptSpaceComponent(SpaceEntity* Parent);

    const oly_common::String& GetScriptSource() const;
    void SetScriptSource(const oly_common::String& ScriptSource);
    int64_t GetOwnerId() const;
    void SetOwnerId(int64_t OwnerId);
    ScriptScope GetScriptScope() const;
    void SetScriptScope(ScriptScope Scope);

protected:
    void SetPropertyFromPatch(uint32_t Key, const ReplicatedValue& Value) override;
    void OnRemove() override;
};

} // namespace oly_multiplayer
