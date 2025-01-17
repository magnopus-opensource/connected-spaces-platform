/*
 * Copyright 2023 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "Debug/Logging.h"

namespace csp::systems
{

class PointOfInterestHelpers
{
public:
    // Enum to string conversion utilities. This rovides a centralized mechanism for mapping between the two
    // and should be used for all POI type/string conversions internally.
    static csp::common::String TypeToString(EPointOfInterestType Type)
    {
        csp::common::String String;

        switch (Type)
        {
        case EPointOfInterestType::SPACE:
        {
            String = "SpaceGeoLocation";
            break;
        }
        case EPointOfInterestType::DEFAULT:
        {
            String = "Default";
            break;
        }
        default:
        {
            CSP_LOG_ERROR_MSG(
                "Unkwnown POI type detected when attempting to derive its string representation. The type string being returned will be empty.");
            break;
        }
        }

        return String;
    }

    static EPointOfInterestType StringToType(const csp::common::String& String)
    {
        EPointOfInterestType Type = EPointOfInterestType::DEFAULT;

        if (String == "Default")
        {
            Type = csp::systems::EPointOfInterestType::DEFAULT;
        }
        // Two terms map to space geolocation, because `OKOSpaceGeoLocation` is a legacy term, preserved for compatibility reasons.
        else if ((String == csp::common::String("SpaceGeoLocation")) || (String == csp::common::String("OKOSpaceGeoLocation")))
        {
            Type = csp::systems::EPointOfInterestType::SPACE;
        }

        return Type;
    }
};

} // namespace csp::systems
