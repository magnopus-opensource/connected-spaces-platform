#include "CSP/Multiplayer/Components/CodeAttribute.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include <string>

namespace csp::multiplayer
{

CodeAttribute::CodeAttribute()
    : Type(PropertyType::NUMBER)
    , StringValue("")
    , FloatValue(0.0f)
    , IntValue(0)
    , BoolValue(false)
    , Min(0.0f)
    , Max(0.0f)
{
}

csp::common::String CodeAttribute::Serialize() const
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
            result += csp::common::String(std::to_string(Vector2Value.X).c_str()) + "," +
                      csp::common::String(std::to_string(Vector2Value.Y).c_str());
            break;
        case PropertyType::VECTOR3:
        case PropertyType::COLOR3:
            result += csp::common::String(std::to_string(Vector3Value.X).c_str()) + "," +
                      csp::common::String(std::to_string(Vector3Value.Y).c_str()) + "," +
                      csp::common::String(std::to_string(Vector3Value.Z).c_str());
            break;
        case PropertyType::ROTATION:
            result += csp::common::String(std::to_string(Vector4Value.X).c_str()) + "," +
                      csp::common::String(std::to_string(Vector4Value.Y).c_str()) + "," +
                      csp::common::String(std::to_string(Vector4Value.Z).c_str()) + "," +
                      csp::common::String(std::to_string(Vector4Value.W).c_str());
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

CodeAttribute CodeAttribute::Deserialize(const csp::common::String& serialized)
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
                    attribute.Vector2Value = csp::common::Vector2(
                        std::stof(parts[1].c_str()), 
                        std::stof(parts[2].c_str())
                    );
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
                    attribute.Vector3Value = csp::common::Vector3(
                        std::stof(parts[1].c_str()), 
                        std::stof(parts[2].c_str()), 
                        std::stof(parts[3].c_str())
                    );
                    // Adjust indexes for Min and Max
                    attribute.Min = std::stof(parts[4].c_str());
                    attribute.Max = std::stof(parts[5].c_str());
                    return attribute;
                }
                break;
            case PropertyType::ROTATION:
                if (parts.Size() >= 7) // Ensure we have enough parts
                {
                    attribute.Vector4Value = csp::common::Vector4(
                        std::stof(parts[1].c_str()), 
                        std::stof(parts[2].c_str()), 
                        std::stof(parts[3].c_str()), 
                        std::stof(parts[4].c_str())
                    );
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

// Getters and Setters
PropertyType CodeAttribute::GetType() const { return Type; }
void CodeAttribute::SetType(PropertyType type) { Type = type; }

const csp::common::String& CodeAttribute::GetStringValue() const { return StringValue; }
void CodeAttribute::SetStringValue(const csp::common::String& stringValue) { StringValue = stringValue; }

float CodeAttribute::GetFloatValue() const { return FloatValue; }
void CodeAttribute::SetFloatValue(float floatValue) { FloatValue = floatValue; }

uint32_t CodeAttribute::GetIntValue() const { return IntValue; }
void CodeAttribute::SetIntValue(uint32_t intValue) { IntValue = intValue; }

bool CodeAttribute::GetBoolValue() const { return BoolValue; }
void CodeAttribute::SetBoolValue(bool boolValue) { BoolValue = boolValue; }

const csp::common::Vector2& CodeAttribute::GetVector2Value() const { return Vector2Value; }
void CodeAttribute::SetVector2Value(const csp::common::Vector2& vector2Value) { Vector2Value = vector2Value; }

const csp::common::Vector3& CodeAttribute::GetVector3Value() const { return Vector3Value; }
void CodeAttribute::SetVector3Value(const csp::common::Vector3& vector3Value) { Vector3Value = vector3Value; }

const csp::common::Vector4& CodeAttribute::GetVector4Value() const { return Vector4Value; }
void CodeAttribute::SetVector4Value(const csp::common::Vector4& vector4Value) { Vector4Value = vector4Value; }

float CodeAttribute::GetMin() const { return Min; }
void CodeAttribute::SetMin(float min) { Min = min; }

float CodeAttribute::GetMax() const { return Max; }
void CodeAttribute::SetMax(float max) { Max = max; }

} // namespace csp::multiplayer
