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
#include "CSP/Systems/Spatial/SpatialDataTypes.h"

#include <cmath>

namespace csp::systems
{

bool GeoLocation::IsValid() const { return Latitude >= -90.0 && Latitude <= 90.0 && Longitude >= -180.0 && Latitude <= 180.0; }

bool GeoLocation::operator==(const GeoLocation& Other) const
{
    const auto LatitudeDiff = std::fabs(Latitude - Other.Latitude);
    const auto LongitudeDiff = std::fabs(Longitude - Other.Longitude);
    return LatitudeDiff <= 0.0000001 && LongitudeDiff <= 0.0000001;
}

bool GeoLocation::operator!=(const GeoLocation& Other) const { return !operator==(Other); }

} // namespace csp::systems
