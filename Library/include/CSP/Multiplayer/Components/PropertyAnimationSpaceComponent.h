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

/// @file PropertyAnimationSpaceComponent.h
/// @brief Definitions and support for property animation components.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/List.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"


namespace csp::multiplayer
{

enum class PropertyAnimationPropertyKeys
{
	Name = 0,
	Length,
	Tracks,
	IsPlaying,
	Num
};


enum class PropertyAnimationTrackInterpolationMode
{
	Nearest,
	Linear,
	Cubic
};


class CSP_API PropertyAnimationKey
{
public:
	float Time;
	ReplicatedValue Value;

	CSP_NO_EXPORT bool operator==(const PropertyAnimationKey& Rhs)
	{
		// return Time == Rhs.Time && Value == Rhs.Value;
		return this == &Rhs;
	}
};


class CSP_API PropertyAnimationTrack
{
public:
	csp::common::String PropertyName;
	PropertyAnimationTrackInterpolationMode InterpolationMode;
	csp::common::List<PropertyAnimationKey> Keys;

	CSP_NO_EXPORT bool operator==(const PropertyAnimationTrack& Rhs)
	{
		// return PropertyName == Rhs.PropertyName && InterpolationMode == Rhs.InterpolationMode && Keys == Rhs.Keys;
		return this == &Rhs;
	}
};


class CSP_API PropertyAnimationSpaceComponent : public ComponentBase
{
public:
	PropertyAnimationSpaceComponent(SpaceEntity* Parent);

	const csp::common::String& GetName() const;
	void SetName(const csp::common::String& InValue);

	float GetLength() const;
	void SetLength(float InValue);

	csp::common::List<PropertyAnimationTrack> GetTracks() const;
	void SetTracks(const csp::common::List<PropertyAnimationTrack>& InValue);

	bool GetIsPlaying() const;
	void SetIsPlaying(bool InValue);
};

} // namespace csp::multiplayer
