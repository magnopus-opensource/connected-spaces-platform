#pragma once

#include "CSP/Common/String.h"
#include "CSP/Common/Array.h"
#include "CSP/Multiplayer/ReplicatedValue.h"
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
    using Vector2 = std::vector<float>;
    using Vector3 = std::vector<float>;
    using Vector4 = std::vector<float>;
/**
 * Class to hold multiple properties for a replicated value
 */
class CodeAttribute
{
public:
    // Default constructor with initialized values
    CodeAttribute()
    {
    }
    // // Copy constructor
    // CodeAttribute(const CodeAttribute& other)
    //     : Type(other.Type)
    //     , StringValue(other.StringValue)
    //     , FloatValue(other.FloatValue)
    //     , IntValue(other.IntValue)
    //     , BoolValue(other.BoolValue)
    //     , Vector2Value(other.Vector2Value)
    //     , Vector3Value(other.Vector3Value)
    //     , Vector4Value(other.Vector4Value)
    //     , Min(other.Min)
    //     , Max(other.Max)
    // {
    // }

    // // Copy constructor
    // CodeAttribute(const CodeAttribute* other)
    //     : Type(other->Type)
    //     , StringValue(other->StringValue)
    //     , FloatValue(other->FloatValue)
    //     , IntValue(other->IntValue)
    //     , BoolValue(other->BoolValue)
    //     , Vector2Value(other->Vector2Value)
    //     , Vector3Value(other->Vector3Value)
    //     , Vector4Value(other->Vector4Value)
    //     , Min(other->Min)
    //     , Max(other->Max)
    // {
    // }
    
    // // Constructor from uintptr_t (for wrapper compatibility)
    // CodeAttribute(uintptr_t ptr)
    // {
    //     if(ptr != 0)
    //     {
    //         CodeAttribute* other = reinterpret_cast<CodeAttribute*>(ptr);
    //         Type = other->Type;
    //         StringValue = other->StringValue;
    //         FloatValue = other->FloatValue;
    //         IntValue = other->IntValue;
    //         BoolValue = other->BoolValue;
    //         Vector2Value = other->Vector2Value;
    //         Vector3Value = other->Vector3Value;
    //         Vector4Value = other->Vector4Value;
    //         Min = other->Min;
    //         Max = other->Max;
    //     }
    // }
    
    // // Conversion operators for wrapper compatibility
    // operator uintptr_t() const { return reinterpret_cast<uintptr_t>(this); }
    // operator void*() const { return reinterpret_cast<void*>(const_cast<CodeAttribute*>(this)); }
    
    // // Assignment operator
    // CodeAttribute& operator=(const CodeAttribute& other)
    // {
    //     if (this != &other)
    //     {
    //         Type = other.Type;
    //         StringValue = other.StringValue;
    //         FloatValue = other.FloatValue;
    //         IntValue = other.IntValue;
    //         BoolValue = other.BoolValue;
    //         Vector2Value = other.Vector2Value;
    //         Vector3Value = other.Vector3Value;
    //         Vector4Value = other.Vector4Value;
    //         Min = other.Min;
    //         Max = other.Max;
    //     }
    //     return *this;
    // }
    

    
    // Serialize CodeAttribute to a comma-separated string
    CSP_NO_EXPORT csp::common::String Serialize() const
    {
        // Format: Type,Value,Min,Max
        csp::common::String result = csp::common::String(std::to_string(static_cast<int>(Type)).c_str()) + ",";
        
        // Append the appropriate value based on the type
        switch(Type)
        {
            case PropertyType::STRING:
                result += StringValue;
                break;
            case PropertyType::NUMBER:
            case PropertyType::SLIDER:
                result += csp::common::String(std::to_string(FloatValue).c_str());
                break;
            case PropertyType::BOOLEAN:
                result += BoolValue ? "true" : "false";
                break;
            case PropertyType::VECTOR2:
                result += csp::common::String(std::to_string(Vector2Value[0]).c_str()) + "," +
                          csp::common::String(std::to_string(Vector2Value[1]).c_str());
                break;
            case PropertyType::VECTOR3:
            case PropertyType::COLOR3:
                result += csp::common::String(std::to_string(Vector3Value[0]).c_str()) + "," +
                          csp::common::String(std::to_string(Vector3Value[1]).c_str()) + "," +
                          csp::common::String(std::to_string(Vector3Value[2]).c_str());
                break;
            case PropertyType::ROTATION:
                result += csp::common::String(std::to_string(Vector4Value[0]).c_str()) + "," +
                          csp::common::String(std::to_string(Vector4Value[1]).c_str()) + "," +
                          csp::common::String(std::to_string(Vector4Value[2]).c_str()) + "," +
                          csp::common::String(std::to_string(Vector4Value[3]).c_str());
                break;
            default:
                // Fallback to string value for unknown types
                result += StringValue;
                break;
        }
        
        // Append Min and Max values
        result += "," + csp::common::String(std::to_string(Min).c_str()) + "," + 
                  csp::common::String(std::to_string(Max).c_str());
        
        return result;
    }
    
    // Deserialize a comma-separated string to CodeAttribute
    CSP_NO_EXPORT static CodeAttribute Deserialize(const csp::common::String& serialized)
    {
        CodeAttribute attribute;

        // Split the serialized string by commas
        csp::common::List<csp::common::String> parts = serialized.Split(',');
        
        // Make sure we have enough parts
        if (parts.Size() >= 4)
        {
            // Convert Type from string to enum
            attribute.Type = static_cast<PropertyType>(std::stoi(parts[0].c_str()));

            switch(attribute.Type)
            {
                case PropertyType::STRING:
                    attribute.StringValue = parts[1];
                    break;
                case PropertyType::NUMBER:
                case PropertyType::SLIDER:
                    attribute.FloatValue = std::stof(parts[1].c_str());
                    break;
                case PropertyType::BOOLEAN:
                    attribute.BoolValue = (parts[1] == "true");
                    break;
                case PropertyType::VECTOR2:
                    if (parts.Size() >= 5) // Ensure we have enough parts
                    {
                        attribute.Vector2Value = std::vector<float>{
                            std::stof(parts[1].c_str()), 
                            std::stof(parts[2].c_str())
                        };
                        // Adjust indexes for Min and Max since we consumed more parts
                        attribute.Min = std::stof(parts[3].c_str());
                        attribute.Max = std::stof(parts[4].c_str());
                        return attribute;
                    }
                    break;
                case PropertyType::VECTOR3:
                case PropertyType::COLOR3:
                    if (parts.Size() >= 6) // Ensure we have enough parts
                    {
                        attribute.Vector3Value = std::vector<float>{
                            std::stof(parts[1].c_str()), 
                            std::stof(parts[2].c_str()), 
                            std::stof(parts[3].c_str())
                        };
                        // Adjust indexes for Min and Max
                        attribute.Min = std::stof(parts[4].c_str());
                        attribute.Max = std::stof(parts[5].c_str());
                        return attribute;
                    }
                    break;
                case PropertyType::ROTATION:
                    if (parts.Size() >= 7) // Ensure we have enough parts
                    {
                        attribute.Vector4Value = std::vector<float>{
                            std::stof(parts[1].c_str()), 
                            std::stof(parts[2].c_str()), 
                            std::stof(parts[3].c_str()), 
                            std::stof(parts[4].c_str())
                        };
                        // Adjust indexes for Min and Max
                        attribute.Min = std::stof(parts[5].c_str());
                        attribute.Max = std::stof(parts[6].c_str());
                        return attribute;
                    }
                    break;
                default:
                    // Handle unknown type
                    break;
            }
            
            // For scalar types (number, string, boolean), Min and Max are at standard positions
            if (parts.Size() >= 4)
            {
                attribute.Min = std::stof(parts[2].c_str());
                attribute.Max = std::stof(parts[3].c_str());
            }
        }
        
        return attribute;
    }
    
    // Getter and Setter methods for Type
    PropertyType GetType() const { return Type; }
    void SetType(PropertyType type) { Type = type; }
    
    // Getter and Setter methods for StringValue
    const csp::common::String& GetStringValue() const { return StringValue; }
    void SetStringValue(const csp::common::String& stringValue) { StringValue = stringValue; }
    
    // Getter and Setter methods for FloatValue
    float GetFloatValue() const { return FloatValue; }
    void SetFloatValue(float floatValue) { FloatValue = floatValue; }
    
    // Getter and Setter methods for IntValue
    uint32_t GetIntValue() const { return IntValue; }
    void SetIntValue(uint32_t intValue) { IntValue = intValue; }
    
    // Getter and Setter methods for BoolValue
    bool GetBoolValue() const { return BoolValue; }
    void SetBoolValue(bool boolValue) { BoolValue = boolValue; }
    
    // Getter and Setter methods for Vector2Value
    const Vector2& GetVector2Value() const { return Vector2Value; }
    void SetVector2Value(const Vector2& vector2Value) { Vector2Value = vector2Value; }
    
    // Getter and Setter methods for Vector3Value
    const Vector3& GetVector3Value() const { return Vector3Value; }
    void SetVector3Value(const Vector3& vector3Value) { Vector3Value = vector3Value; }
    
    // Getter and Setter methods for Vector4Value
    const Vector4& GetVector4Value() const { return Vector4Value; }
    void SetVector4Value(const Vector4& vector4Value) { Vector4Value = vector4Value; }
    
    // Getter and Setter methods for Min
    float GetMin() const { return Min; }
    void SetMin(float min) { Min = min; }
    
    // Getter and Setter methods for Max
    float GetMax() const { return Max; }
    void SetMax(float max) { Max = max; }
    
    // Member variables - explicitly public
private:
    PropertyType Type;
    csp::common::String StringValue;
    float FloatValue;
    uint32_t IntValue;
    bool BoolValue;
    std::vector<float> Vector2Value;
    std::vector<float> Vector3Value;
    std::vector<float> Vector4Value;
    float Min;
    float Max;
};

} // namespace csp::multiplayer
