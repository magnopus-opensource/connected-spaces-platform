#pragma once

#include "Olympus/Common/String.h"

#include <map>
#include <string>

namespace oly_systems
{
class ScriptSystem;
}

namespace oly_multiplayer
{
class SpaceEntitySystem;

class SpaceEntity;
class ScriptSpaceComponent;

class OLY_API EntityScript
{
    OLY_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceEntity;
    /** @endcond */
    OLY_END_IGNORE

public:
    ~EntityScript();

    void SetScriptSource(const oly_common::String& ScriptSource);
    bool Invoke();

    void RunScript(const oly_common::String& ScriptSource);

    bool HasError();
    oly_common::String GetErrorText();
    oly_common::String GetScriptSource();

    void SetScriptSpaceComponent(ScriptSpaceComponent* InEnityScriptComponent);

    void OnPropertyChanged(int32_t ComponentId, int32_t PropertyKey);

    // Script Binding Interface
    OLY_NO_EXPORT void SubscribeToPropertyChange(int32_t ComponentId, int32_t PropertyKey, oly_common::String EventId);
    OLY_NO_EXPORT void SubscribeToMessage(const oly_common::String Message, const oly_common::String OnMessageCallback);
    void PostMessageToScript(const oly_common::String Message, const oly_common::String MessageParamsJson = "");

    void OnSourceChanged(const oly_common::String& InScriptSource);
    void RegisterSourceAsModule();
    void Bind();

    void SetOwnerId(uint64_t ClientId);
    uint64_t GetOwnerId() const;

    void Shutdown();

private:
    EntityScript(SpaceEntity* InEntity, SpaceEntitySystem* InSpaceEntitySystem);

    void CheckBinding();

    oly_systems::ScriptSystem* ScriptSystem;
    SpaceEntity* Entity;
    ScriptSpaceComponent* EntityScriptComponent;

    bool HasLastError;
    oly_common::String LastError;

    using PropertyChangeKey = std::pair<int32_t, int32_t>;
    using PropertyChangeMap = std::map<PropertyChangeKey, oly_common::String>;
    PropertyChangeMap PropertyMap;

    using SubscribedMessageMap = std::map<oly_common::String, oly_common::String>;
    SubscribedMessageMap MessageMap;

    bool HasBinding;

    SpaceEntitySystem* SpaceEntitySystemPtr;
};

} // namespace oly_multiplayer
