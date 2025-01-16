#pragma once

#include "Olympus/OlympusCommon.h"

namespace oly_web
{

class WebClient;

}

namespace oly_systems
{

class OLY_API OLY_NO_DISPOSE SystemBase
{
protected:
    SystemBase()
        : WebClient(nullptr) {};
    OLY_NO_EXPORT SystemBase(oly_web::WebClient* InWebClient)
        : WebClient(InWebClient) {};

    oly_web::WebClient* WebClient;
};

} // namespace oly_systems
