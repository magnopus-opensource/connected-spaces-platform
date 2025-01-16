#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/String.h"
#include "Olympus/Common/Vector.h"
#include "Olympus/OlympusCommon.h"

#include <functional>

namespace oly_common
{

enum class VariantType
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
 * @brief Variant is an intermediate class that enables clients to pack data into types that are supported by Foundation replication systems.
 */
class OLY_API Variant
{
public:
    /**
     * @brief A default Variant will not have a valid type ("VariantType::InvalidType"), and will have no internal value associated.
     * Do not use this constructor unless you know what you are doing!
     */
    Variant();
    /**
     * @brief Construct a Variant based on a bool type.
     * @param InBoolValue bool : In Value.
     */
    Variant(bool InBoolValue);
    /**
     * @brief Construct a Variant based on a float type.
     * @param InFloatValue float : In Value.
     */
    Variant(float InFloatValue);
    /**
     * @brief Construct a Variant based on a Long (uint64_t) type.
     * @param InLongValue int64_t : In Value.
     */
    Variant(int64_t InLongValue);

    OLY_NO_EXPORT Variant(const char* InStringValue);

    /**
     * @brief Construct a Variant based on an oly_common::String type.
     * @param InStringValue oly_common::String : In Value.
     */
    Variant(oly_common::String InStringValue);

    /**
     * @brief Construct a Variant based on a oly_common::Vector3 type.
     * @param InVector3Value oly_common::Vector3 : In Value.
     */
    Variant(oly_common::Vector3 InVector3Value);

    /**
     * @brief Construct a Variant based on an oly_common::Vector4 type.
     * @param InVector4Value oly_common::Vector4 : In Value.
     */
    Variant(oly_common::Vector4 InVector4Value);

    /**
     * @brief Copy constructor
     * @param Other oly_multiplayer::Variant&
     */
    Variant(const Variant& Other);
    ~Variant();

    Variant& operator=(const Variant& InValue);
    bool operator==(const Variant& OtherValue) const;
    bool operator!=(const Variant& OtherValue) const;

    /**
     * @brief Gets the type of replicated value.
     * @return VariantType : Enum representing all supported replication base types.
     */
    VariantType GetValueType() const { return ValueType; }

    void SetBool(bool InValue);
    bool GetBool() const;

    void SetFloat(float InValue);
    float GetFloat() const;

    void SetInt(int64_t InValue);
    int64_t GetInt() const;

    OLY_NO_EXPORT void SetString(const char* InValue);
    void SetString(const oly_common::String& InValue);
    const oly_common::String& GetString() const;

    void SetVector3(oly_common::Vector3 InValue);
    oly_common::Vector3 GetVector3() const;

    void SetVector4(oly_common::Vector4 InValue);
    oly_common::Vector4 GetVector4() const;

    OLY_NO_EXPORT static size_t GetSizeOfInternalValue();

private:
    VariantType ValueType;

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

} // namespace oly_common
