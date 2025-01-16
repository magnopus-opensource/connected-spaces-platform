#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/String.h"
#include "Olympus/Common/Vector.h"
#include "Olympus/OlympusCommon.h"

#include <functional>

namespace oly_multiplayer
{

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

/**
 * @brief ReplicatedValue is an intermediate class that enables clients to pack data into types that are supported by Foundation replication systems.
 */
class OLY_API ReplicatedValue
{
public:
    /**
     * @brief A default ReplicatedValue will not have a valid type ("ReplicatedValueType::InvalidType"), and will have no internal value associated.
     * Do not use this constructor unless you know what you are doing!
     */
    ReplicatedValue();
    /**
     * @brief Construct a ReplicatedValue based on a bool type.
     * @param InBoolValue bool : In Value.
     */
    ReplicatedValue(bool InBoolValue);
    /**
     * @brief Construct a ReplicatedValue based on a float type.
     * @param InFloatValue float : In Value.
     */
    ReplicatedValue(float InFloatValue);
    /**
     * @brief Construct a ReplicatedValue based on a Long (uint64_t) type.
     * @param InLongValue int64_t : In Value.
     */
    ReplicatedValue(int64_t InLongValue);

    OLY_NO_EXPORT ReplicatedValue(const char* InStringValue);

    /**
     * @brief Construct a ReplicatedValue based on an oly_common::String type.
     * @param InStringValue oly_common::String : In Value.
     */
    ReplicatedValue(const oly_common::String& InStringValue);

    /**
     * @brief Construct a ReplicatedValue based on a oly_common::Vector3 type.
     * @param InVector3Value oly_common::Vector3 : In Value.
     */
    ReplicatedValue(const oly_common::Vector3& InVector3Value);

    /**
     * @brief Construct a ReplicatedValue based on an oly_common::Vector4 type.
     * @param InVector4Value oly_common::Vector4 : In Value.
     */
    ReplicatedValue(const oly_common::Vector4& InVector4Value);

    /**
     * @brief Copy constructor
     * @param Other oly_multiplayer::ReplicatedValue&
     */
    ReplicatedValue(const ReplicatedValue& Other);
    ~ReplicatedValue();

    ReplicatedValue& operator=(const ReplicatedValue& InValue);
    bool operator==(const ReplicatedValue& OtherValue) const;
    bool operator!=(const ReplicatedValue& OtherValue) const;

    /**
     * @brief Gets the type of replicated value.
     * @return ReplicatedValueType: Enum representing all supported replication base types.
     */
    ReplicatedValueType GetReplicatedValueType() const { return ReplicatedType; }

    /**
     * @brief Sets a bool value for this replicated value, will overwrite any previous value.
     * @param InValue
     */
    void SetBool(bool InValue);
    /**
     * @brief Get a bool value from this replicated value, will assert if not a bool type.
     * Use ReplicatedValue::GetReplicatedValueType to ensure type before accessing.
     * @return bool value
     */
    bool GetBool() const;

    /**
     * @brief Sets a float value for this replicated value, will overwrite any previous value.
     * @param InValue
     */
    void SetFloat(float InValue);
    /**
     * @brief Get a float value from this replicated value, will assert if not a float type.
     * Use ReplicatedValue::GetReplicatedValueType to ensure type before accessing.
     * @return float value
     */
    float GetFloat() const;

    /**
     * @brief Sets a int64 value for this replicated value, will overwrite any previous value.
     * @param InValue
     */
    void SetInt(int64_t InValue);
    /**
     * @brief Get a int64 value from this replicated value, will assert if not a int64 type.
     * Use ReplicatedValue::GetReplicatedValueType to ensure type before accessing.
     * @return int64 value
     */
    int64_t GetInt() const;

    /**
     * @brief Set a string value for this replicated value from a const char*, will overwrite and previous value.
     */
    OLY_NO_EXPORT void SetString(const char* InValue);
    /**
     * @brief Set a string value for this replicated value from a oly_common::String&, will overwrite and previous value.
     */
    void SetString(const oly_common::String& InValue);
    /**
     * @brief Get a oly_common::String& value from this replicated value, will assert if not a oly_common::String type.
     * Use ReplicatedValue::GetReplicatedValueType to ensure type before accessing.
     * @return oly_common::String&
     */
    const oly_common::String& GetString() const;

    OLY_NO_EXPORT static const oly_common::String& GetDefaultString();

    /**
     * @brief Set a Vector3 value for this replicated value from a oly_common::Vector3, will overwrite and previous value.
     */
    void SetVector3(const oly_common::Vector3& InValue);
    /**
     * @brief Get a oly_common::Vector3 value from this replicated value, will assert if not a oly_common::Vector3 type.
     * Use ReplicatedValue::GetReplicatedValueType to ensure type before accessing.
     * @return oly_common::Vector3
     */
    const oly_common::Vector3& GetVector3() const;

    OLY_NO_EXPORT static const oly_common::Vector3& GetDefaultVector3();

    /**
     * @brief Set a Vector4 value for this replicated value from a oly_common::Vector4, will overwrite and previous value.
     */
    void SetVector4(const oly_common::Vector4& InValue);
    /**
     * @brief Get a oly_common::Vector4 value from this replicated value, will assert if not a oly_common::Vector4 type.
     * Use ReplicatedValue::GetReplicatedValueType to ensure type before accessing.
     * @return oly_common::Vector4
     */
    const oly_common::Vector4& GetVector4() const;

    OLY_NO_EXPORT static const oly_common::Vector4& GetDefaultVector4();

    /**
     * @brief returns the size of the stored internal value.
     * @return size_t size of the internal value.
     */
    OLY_NO_EXPORT static size_t GetSizeOfInternalValue();

private:
    ReplicatedValueType ReplicatedType;

    OLY_START_IGNORE
    union InternalValue
    {
        InternalValue();
        ~InternalValue();

        bool Bool;
        float Float;
        int64_t Int;
        oly_common::String String;
        oly_common::Vector3 Vector3;
        oly_common::Vector4 Vector4;
    };

    InternalValue Value;
    OLY_END_IGNORE
};

} // namespace oly_multiplayer
