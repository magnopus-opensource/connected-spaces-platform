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

#include "CSP/Multiplayer/Components/PropertyAnimationSpaceComponent.h"

#include "Debug/Logging.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>


using namespace csp::common;


namespace csp::multiplayer
{

PropertyAnimationSpaceComponent::PropertyAnimationSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::PropertyAnimation, Parent)
{
	Properties[static_cast<uint32_t>(PropertyAnimationPropertyKeys::Name)]		= "";
	Properties[static_cast<uint32_t>(PropertyAnimationPropertyKeys::Length)]	= 0.0f;
	Properties[static_cast<uint32_t>(PropertyAnimationPropertyKeys::Tracks)]	= "";
	Properties[static_cast<uint32_t>(PropertyAnimationPropertyKeys::IsPlaying)] = false;
}


const String& PropertyAnimationSpaceComponent::GetName() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(PropertyAnimationPropertyKeys::Name));

	if (RepVal.GetReplicatedValueType() != ReplicatedValueType::String)
	{
		CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

		return ReplicatedValue::GetDefaultString();
	}

	return RepVal.GetString();
}

void PropertyAnimationSpaceComponent::SetName(const String& InValue)
{
	SetProperty(static_cast<uint32_t>(PropertyAnimationPropertyKeys::Name), InValue);
}


float PropertyAnimationSpaceComponent::GetLength() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(PropertyAnimationPropertyKeys::Length));

	if (RepVal.GetReplicatedValueType() != ReplicatedValueType::Float)
	{
		CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

		return 0.0f;
	}

	return RepVal.GetFloat();
}

void PropertyAnimationSpaceComponent::SetLength(float InValue)
{
	SetProperty(static_cast<uint32_t>(PropertyAnimationPropertyKeys::Length), InValue);
}


List<PropertyAnimationTrack> PropertyAnimationSpaceComponent::GetTracks() const
{
	List<PropertyAnimationTrack> Tracks;

	const auto& RepVal = GetProperty(static_cast<uint32_t>(PropertyAnimationPropertyKeys::Tracks));

	if (RepVal.GetReplicatedValueType() != ReplicatedValueType::String)
	{
		CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

		return Tracks;
	}

	rapidjson::Document Json;
	Json.Parse(RepVal.GetString().c_str());

	if (!Json.IsArray())
	{
		CSP_LOG_ERROR_MSG("Underlying JSON value type not valid!");

		return Tracks;
	}

	for (const auto& TrackJson : Json.GetArray())
	{
		PropertyAnimationTrack Track;
		Track.PropertyName		= TrackJson["property"].GetString();
		Track.InterpolationMode = static_cast<PropertyAnimationTrackInterpolationMode>(TrackJson["interpolation_mode"].GetInt());

		for (const auto& KeyJson : TrackJson["keys"].GetArray())
		{
			PropertyAnimationKey Key;
			Key.Time = KeyJson["time"].GetFloat();

			auto ValueType		  = static_cast<ReplicatedValueType>(KeyJson["value_type"].GetInt());
			const auto& ValueJson = KeyJson["value"];
			ReplicatedValue Value;

			switch (ValueType)
			{
				case ReplicatedValueType::Vector3:
				{
					const auto& ArrayValue = ValueJson.GetArray();
					Vector3 VectorValue	   = {ArrayValue[0].GetFloat(), ArrayValue[1].GetFloat(), ArrayValue[2].GetFloat()};
					Value.SetVector3(VectorValue);
					break;
				}
				case ReplicatedValueType::Vector4:
				{
					const auto& ArrayValue = ValueJson.GetArray();
					Vector4 VectorValue	   = {ArrayValue[0].GetFloat(), ArrayValue[1].GetFloat(), ArrayValue[2].GetFloat(), ArrayValue[3].GetFloat()};
					Value.SetVector4(VectorValue);
					break;
				}
				default:
					CSP_LOG_ERROR_MSG("Unsupported property animation key value type!");

					break;
			}

			Key.Value = Value;
			Track.Keys.Append(Key);
		}

		Tracks.Append(Track);
	}

	return Tracks;
}

void PropertyAnimationSpaceComponent::SetTracks(const List<PropertyAnimationTrack>& InValue)
{
	rapidjson::Document Json;
	Json.SetArray();
	auto& ArrayJson = Json.GetArray();

	for (int i = 0; i < InValue.Size(); ++i)
	{
		rapidjson::Value ValueJson(rapidjson::kObjectType);
		auto& ObjectJson = ValueJson.GetObject();

		const auto& Track = InValue[i];
		ObjectJson.AddMember("property", rapidjson::Value(Track.PropertyName.c_str(), Track.PropertyName.Length()), Json.GetAllocator());
		ObjectJson.AddMember("interpolation_mode", static_cast<int>(Track.InterpolationMode), Json.GetAllocator());

		rapidjson::Value KeysValueJson(rapidjson::kArrayType);
		auto& KeysJson = KeysValueJson.GetArray();

		const auto& Keys = Track.Keys;

		for (int j = 0; j < Keys.Size(); ++j)
		{
			const auto& Key = Keys[j];

			rapidjson::Value KeyValueJson(rapidjson::kObjectType);
			auto& KeyObjectJson = KeyValueJson.GetObject();
			KeyObjectJson.AddMember("time", Key.Time, Json.GetAllocator());

			const auto& Value = Key.Value;
			KeyObjectJson.AddMember("value_type", static_cast<int>(Value.GetReplicatedValueType()), Json.GetAllocator());

			switch (Value.GetReplicatedValueType())
			{
				case ReplicatedValueType::Vector3:
				{
					const auto& Vector = Value.GetVector3();

					KeyObjectJson.AddMember("value", rapidjson::Value(rapidjson::kArrayType), Json.GetAllocator());
					auto& ValueArrayJson = KeyObjectJson["value"].GetArray();
					ValueArrayJson.PushBack(Vector.X, Json.GetAllocator());
					ValueArrayJson.PushBack(Vector.Y, Json.GetAllocator());
					ValueArrayJson.PushBack(Vector.Z, Json.GetAllocator());
					break;
				}
				case ReplicatedValueType::Vector4:
				{
					const auto& Vector = Value.GetVector4();

					KeyObjectJson.AddMember("value", rapidjson::Value(rapidjson::kArrayType), Json.GetAllocator());
					auto& ValueArrayJson = KeyObjectJson["value"].GetArray();
					ValueArrayJson.PushBack(Vector.X, Json.GetAllocator());
					ValueArrayJson.PushBack(Vector.Y, Json.GetAllocator());
					ValueArrayJson.PushBack(Vector.Z, Json.GetAllocator());
					ValueArrayJson.PushBack(Vector.W, Json.GetAllocator());

					break;
				}
				default:
					CSP_LOG_ERROR_MSG("Unsupported property animation key value type!");

					break;
			}

			KeysJson.PushBack(KeyObjectJson, Json.GetAllocator());
		}

		ObjectJson.AddMember("keys", KeysJson, Json.GetAllocator());
		ArrayJson.PushBack(ObjectJson, Json.GetAllocator());
	}

	rapidjson::StringBuffer JsonBuffer;
	rapidjson::Writer<rapidjson::StringBuffer> JsonWriter(JsonBuffer);
	Json.Accept(JsonWriter);

	SetProperty(static_cast<uint32_t>(PropertyAnimationPropertyKeys::Tracks), String(JsonBuffer.GetString(), JsonBuffer.GetSize()));
}


bool PropertyAnimationSpaceComponent::GetIsPlaying() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(PropertyAnimationPropertyKeys::IsPlaying));

	if (RepVal.GetReplicatedValueType() != ReplicatedValueType::Boolean)
	{
		CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

		return false;
	}

	return RepVal.GetBool();
}

void PropertyAnimationSpaceComponent::SetIsPlaying(bool InValue)
{
	SetProperty(static_cast<uint32_t>(PropertyAnimationPropertyKeys::IsPlaying), InValue);
}

} // namespace csp::multiplayer
