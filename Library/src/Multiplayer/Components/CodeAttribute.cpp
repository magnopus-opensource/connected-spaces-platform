#include "CSP/Multiplayer/Components/CodeAttribute.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "Debug/Logging.h"
#include <string>

namespace csp::multiplayer
{

CodeAttribute::CodeAttribute()
    : Type(CodePropertyType::NUMBER)
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
        case CodePropertyType::STRING:
            result += StringValue;
            break;
        case CodePropertyType::NUMBER:
        case CodePropertyType::SLIDER:
            result += csp::common::String(std::to_string(FloatValue).c_str());
            break;
        case CodePropertyType::BOOLEAN:
            result += BoolValue ? "true" : "false";
            break;
        case CodePropertyType::VECTOR2:
            result += csp::common::String(std::to_string(Vector2Value.X).c_str()) + "," +
                      csp::common::String(std::to_string(Vector2Value.Y).c_str());
            break;
        case CodePropertyType::VECTOR3:
        case CodePropertyType::COLOR3:
            result += csp::common::String(std::to_string(Vector3Value.X).c_str()) + "," +
                      csp::common::String(std::to_string(Vector3Value.Y).c_str()) + "," +
                      csp::common::String(std::to_string(Vector3Value.Z).c_str());
            break;
        case CodePropertyType::VECTOR4:
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
    CSP_LOG_FORMAT(csp::systems::LogLevel::Debug, "Serialized CodeAttribute: %s", result.c_str());
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
        attribute.Type = static_cast<CodePropertyType>(std::stoi(parts[0].c_str()));

        switch(attribute.Type)
        {
            case CodePropertyType::STRING:
                attribute.StringValue = parts[1];
                break;
            case CodePropertyType::NUMBER:
            case CodePropertyType::SLIDER:
                attribute.FloatValue = std::stof(parts[1].c_str());
                break;
            case CodePropertyType::BOOLEAN:
                attribute.BoolValue = (parts[1] == "true");
                break;
            case CodePropertyType::VECTOR2:
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
            case CodePropertyType::VECTOR3:
            case CodePropertyType::COLOR3:
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
            case CodePropertyType::VECTOR4:
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
    // log attribute contents
    CSP_LOG_FORMAT(csp::systems::LogLevel::Debug, "Deserialized CodeAttribute: Type=%d, StringValue=%s, FloatValue=%f, IntValue=%u, BoolValue=%s, Vector2Value=(%f, %f), Vector3Value=(%f, %f, %f), Vector4Value=(%f, %f, %f, %f), Min=%f, Max=%f",
                   static_cast<int>(attribute.Type),
                   attribute.StringValue.c_str(),
                   attribute.FloatValue,
                   attribute.IntValue,
                   attribute.BoolValue ? "true" : "false",
                   attribute.Vector2Value.X, attribute.Vector2Value.Y,
                   attribute.Vector3Value.X, attribute.Vector3Value.Y, attribute.Vector3Value.Z,
                   attribute.Vector4Value.X, attribute.Vector4Value.Y, attribute.Vector4Value.Z, attribute.Vector4Value.W,
                   attribute.Min,
                   attribute.Max);
    return attribute;
}

// Getters and Setters
CodePropertyType CodeAttribute::GetType() const { return Type; }
void CodeAttribute::SetType(CodePropertyType type) { Type = type; }

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
