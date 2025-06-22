#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/Vector.h"
#include "CSP/Common/String.h"
#include <string>

namespace csp::multiplayer
{

// Define enum for Type values, these relate to the code component ui attributes
// not just data types
enum class PropertyType
{
    NUMBER = 0,
    STRING,
    VECTOR2,
    VECTOR3,
    ROTATION,
    COLOR3,
    BOOLEAN,
    SLIDER,
    NUM
};

using Vector2 = csp::common::Vector2;
using Vector3 = csp::common::Vector3;
using Vector4 = csp::common::Vector4;

/**
 * Class to hold multiple properties for a replicated value
 */
class CSP_API CodeAttribute
{
public:
    // Default constructor
    CodeAttribute();
    
    // Serialize CodeAttribute to a comma-separated string
    CSP_NO_EXPORT csp::common::String Serialize() const;
    
    // Deserialize a comma-separated string to CodeAttribute
    CSP_NO_EXPORT static CodeAttribute Deserialize(const csp::common::String& serialized);
    
    // Getter and Setter methods for Type
    PropertyType GetType() const;
    void SetType(PropertyType type);
    
    // Getter and Setter methods for StringValue
    const csp::common::String& GetStringValue() const;
    void SetStringValue(const csp::common::String& stringValue);
    
    // Getter and Setter methods for FloatValue
    float GetFloatValue() const;
    void SetFloatValue(float floatValue);
    
    // Getter and Setter methods for IntValue
    uint32_t GetIntValue() const;
    void SetIntValue(uint32_t intValue);
    
    // Getter and Setter methods for BoolValue
    bool GetBoolValue() const;
    void SetBoolValue(bool boolValue);
    
    // Getter and Setter methods for Vector2Value
    const Vector2& GetVector2Value() const;
    void SetVector2Value(const Vector2& vector2Value);
    
    // Getter and Setter methods for Vector3Value
    const Vector3& GetVector3Value() const;
    void SetVector3Value(const Vector3& vector3Value);
    
    // Getter and Setter methods for Vector4Value
    const Vector4& GetVector4Value() const;
    void SetVector4Value(const Vector4& vector4Value);
    
    // Getter and Setter methods for Min
    float GetMin() const;
    void SetMin(float min);
    
    // Getter and Setter methods for Max
    float GetMax() const;
    void SetMax(float max);
    
private:
    PropertyType Type;
    csp::common::String StringValue;
    float FloatValue;
    uint32_t IntValue;
    bool BoolValue;
    Vector2 Vector2Value;
    Vector3 Vector3Value;
    Vector4 Vector4Value;
    float Min;
    float Max;
};

} // namespace csp::multiplayer
   