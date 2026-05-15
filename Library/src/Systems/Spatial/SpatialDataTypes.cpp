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

bool GeoLocation::operator==(const GeoLocation& other) const
{
    const auto latitudeDiff = std::fabs(Latitude - other.Latitude);
    const auto longitudeDiff = std::fabs(Longitude - other.Longitude);
    return latitudeDiff <= 0.0000001 && longitudeDiff <= 0.0000001;
}
bool OlyRotation::operator==(const OlyRotation& other) const { return X == other.X && Y == other.Y && Z == other.Z && W == other.W; }

bool GeoLocation::operator!=(const GeoLocation& other) const { return !(*this == other); }
bool OlyRotation::operator!=(const OlyRotation& other) const { return !(*this == other); }

} // namespace csp::systems
