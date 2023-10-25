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

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"

#include <functional>

namespace csp::multiplayer
{

/// @brief Enum representing the type of a replicated value.
enum class ReplicatedValueType
{
	InvalidType,
	Boolean,
	Integer,
	Float,
	String,
	Vector3,
	Vector4,
};

/// @brief ReplicatedValue is an intermediate class that enables clients to pack data into types that are supported by Connected Spaces Platform
/// replication systems.
class CSP_API ReplicatedValue
{
public:
	/// @brief A default ReplicatedValue will not have a valid type ("ReplicatedValueType::InvalidType"), and will have no internal value associated.
	///
	/// Do not use this constructor unless you know what you are doing!
	ReplicatedValue();

	/// @brief Construct a ReplicatedValue based on a bool type.
	/// @param InBoolValue bool : Initial value.
	ReplicatedValue(bool InBoolValue);

	/// @brief Construct a ReplicatedValue based on a float type.
	/// @param InFloatValue float : Initial value.
	ReplicatedValue(float InFloatValue);

	/// @brief Construct a ReplicatedValue based on a Long (uint64_t) type.
	/// @param InLongValue int64_t : Initial value.
	ReplicatedValue(int64_t InLongValue);

	/// @brief Construct a ReplicatedValue based on a csp::common::String type derived from the given char*.
	/// @param InLongValue int64_t : Initial value.
	CSP_NO_EXPORT ReplicatedValue(const char* InStringValue);

	/// @brief Construct a ReplicatedValue based on an csp::common::String type.
	/// @param InStringValue csp::common::String : Initial value.
	ReplicatedValue(const csp::common::String& InStringValue);

	/// @brief Construct a ReplicatedValue based on a csp::common::Vector3 type.
	/// @param InVector3Value csp::common::Vector3 : Initial value.
	ReplicatedValue(const csp::common::Vector3& InVector3Value);

	/// @brief Construct a ReplicatedValue based on an csp::common::Vector4 type.
	/// @param InVector4Value csp::common::Vector4 : Initial value.
	ReplicatedValue(const csp::common::Vector4& InVector4Value);

	/// @brief Copy constructor
	/// @param Other csp::multiplayer::ReplicatedValue& : The value to copy.
	ReplicatedValue(const ReplicatedValue& Other);

	/// @brief Destroys the replicated value instance.
	~ReplicatedValue();

	/// @brief Assignment operator overload.
	/// @param InValue ReplicatedValue : Other replicated value to set this one to.
	ReplicatedValue& operator=(const ReplicatedValue& InValue);

	/// @brief Equality operator overload.
	/// @param ReplicatedValue : Other value to compare to.
	bool operator==(const ReplicatedValue& OtherValue) const;

	/// @brief Non equality operator overload.
	/// @param ReplicatedValue : Other value to compare to.
	bool operator!=(const ReplicatedValue& OtherValue) const;

	/// @brief Gets the type of replicated value.
	/// @return ReplicatedValueType: Enum representing all supported replication base types.
	ReplicatedValueType GetReplicatedValueType() const
	{
		return ReplicatedType;
	}

	/// @brief Sets a bool value for this replicated value, will overwrite any previous value.
	/// @param InValue
	void SetBool(bool InValue);

	/// @brief Get a bool value from this replicated value, will assert if not a bool type.
	///
	/// Use ReplicatedValue::GetReplicatedValueType to ensure type before accessing.
	///
	/// @return bool
	bool GetBool() const;

	/// @brief Sets a float value for this replicated value, will overwrite any previous value.
	/// @param InValue
	void SetFloat(float InValue);

	/// @brief Get a float value from this replicated value, will assert if not a float type.
	///
	/// Use ReplicatedValue::GetReplicatedValueType to ensure type before accessing.
	///
	/// @return float value
	float GetFloat() const;

	/// @brief Sets a int64 value for this replicated value, will overwrite any previous value.
	/// @param InValue
	void SetInt(int64_t InValue);

	/// @brief Get a int64 value from this replicated value, will assert if not a int64 type.
	///
	/// Use ReplicatedValue::GetReplicatedValueType to ensure type before accessing.
	///
	/// @return int64 value
	int64_t GetInt() const;

	/// @brief Set a string value for this replicated value from a const char*, will overwrite and previous value.
	CSP_NO_EXPORT void SetString(const char* InValue);

	///@brief Set a string value for this replicated value from a csp::common::String&, will overwrite and previous value.
	void SetString(const csp::common::String& InValue);

	/// @brief Get a csp::common::String& value from this replicated value, will assert if not a csp::common::String type.
	///
	/// Use ReplicatedValue::GetReplicatedValueType to ensure type before accessing.
	///
	/// @return csp::common::String&
	const csp::common::String& GetString() const;

	/// @brief Get a generic default string.
	/// @return The default string.
	CSP_NO_EXPORT static const csp::common::String& GetDefaultString();

	/// @brief Set a Vector3 value for this replicated value from a csp::common::Vector3, will overwrite and previous value.
	void SetVector3(const csp::common::Vector3& InValue);

	/// @brief Get a csp::common::Vector3 value from this replicated value, will assert if not a csp::common::Vector3 type.
	///
	/// Use ReplicatedValue::GetReplicatedValueType to ensure type before accessing.
	///
	/// @return csp::common::Vector3
	const csp::common::Vector3& GetVector3() const;

	/// @brief Get a generic default Vector3.
	/// @return The default Vector3.
	CSP_NO_EXPORT static const csp::common::Vector3& GetDefaultVector3();

	/// @brief Set a Vector4 value for this replicated value from a csp::common::Vector4, will overwrite and previous value.
	void SetVector4(const csp::common::Vector4& InValue);

	/// @brief Get a csp::common::Vector4 value from this replicated value, will assert if not a csp::common::Vector4 type.
	///
	/// Use ReplicatedValue::GetReplicatedValueType to ensure type before accessing.
	///
	/// @return csp::common::Vector4
	const csp::common::Vector4& GetVector4() const;

	/// @brief Get a generic default Vector4.
	/// @return The default Vector4.
	CSP_NO_EXPORT static const csp::common::Vector4& GetDefaultVector4();

	/// @brief returns the size of the stored internal value.
	/// @return size_t size of the internal value.
	CSP_NO_EXPORT static size_t GetSizeOfInternalValue();

private:
	ReplicatedValueType ReplicatedType;

	CSP_START_IGNORE
	union InternalValue
	{
		InternalValue();
		~InternalValue();

		bool Bool;
		float Float;
		int64_t Int;
		csp::common::String String;
		csp::common::Vector3 Vector3;
		csp::common::Vector4 Vector4;
	};

	InternalValue Value;
	CSP_END_IGNORE
};

} // namespace csp::multiplayer
