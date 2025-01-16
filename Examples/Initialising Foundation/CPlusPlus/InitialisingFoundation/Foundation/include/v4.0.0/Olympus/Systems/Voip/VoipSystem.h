#pragma once

#include "Olympus/OlympusCommon.h"

namespace oly_web
{

class WebClient;

}

namespace oly_systems
{

class OLY_API OLY_NO_DISPOSE VoipSystem
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */

public:
    ~VoipSystem();

    void MuteLocalUser(bool IsMuted);
    bool IsLocalUserMuted() const;

private:
    VoipSystem();
};

} // namespace oly_systems
