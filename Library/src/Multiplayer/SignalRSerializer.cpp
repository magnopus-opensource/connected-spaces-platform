#include "SignalRSerializer.h"

namespace csp::multiplayer
{
void SignalRSerializer::StartArray() { Stack.push(std::vector<signalr::value> {}); }

void SignalRSerializer::EndArray()
{
    if (Stack.size() == 0 || std::holds_alternative<std::vector<signalr::value>>(Stack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Serializer was not in an array");
    }

    auto Current = std::move(Stack.top());
    Stack.pop();
    Pop(signalr::value { std::get<std::vector<signalr::value>>(Current) });
}

void SignalRSerializer::StartStringMap() { Stack.push(std::map<std::string, signalr::value> {}); }

void SignalRSerializer::EndStringMap()
{
    if (Stack.size() == 0 || std::holds_alternative<std::map<std::string, signalr::value>>(Stack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Serializer was not in a string map");
    }

    auto Current = std::move(Stack.top());
    Stack.pop();
    Pop(signalr::value { std::get<std::map<std::string, signalr::value>>(Current) });
}

void SignalRSerializer::StartUintMap() { Stack.push(std::map<uint64_t, signalr::value> {}); }

void SignalRSerializer::EndUintMap()
{
    if (Stack.size() == 0 || std::holds_alternative<std::map<std::uint64_t, signalr::value>>(Stack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Serializer was not in a uint map");
    }

    auto Current = std::move(Stack.top());
    Stack.pop();
    Pop(signalr::value { std::get<std::map<uint64_t, signalr::value>>(Current) });
}

signalr::value SignalRSerializer::Get() const
{
    if (Stack.size() == 0)
    {
        return signalr::value();
    }

    if (Stack.size() > 1)
    {
        throw std::runtime_error("Invalid call: Serializer is not at the root");
    }

    const Container& Root = Stack.top();

    if (std::holds_alternative<signalr::value>(Root))
    {
        return std::get<signalr::value>(Root);
    }
    else if (std::holds_alternative<std::vector<signalr::value>>(Root))
    {
        return signalr::value { std::get<std::vector<signalr::value>>(Root) };
    }
    else if (std::holds_alternative<std::map<std::string, signalr::value>>(Root))
    {
        return signalr::value { std::get<std::map<std::string, signalr::value>>(Root) };
    }
    else if (std::holds_alternative<std::map<uint64_t, signalr::value>>(Root))
    {
        return signalr::value { std::get<std::map<uint64_t, signalr::value>>(Root) };
    }
    else
    {
        throw std::runtime_error("Unexpected serializer state");
    }

    return signalr::value {};
}

void SignalRSerializer::Pop(signalr::value&& Last)
{
    // We dont check for maps in this function because key-values are handled seperatly
    if (Stack.size() == 0)
    {
        // If the last element has been popped, push back a root value
        Stack.push(Last);
        return;
    }
    else if (std::holds_alternative<std::vector<signalr::value>>(Stack.top()))
    {
        std::get<std::vector<signalr::value>>(Stack.top()).push_back(Last);
    }
    else if (std::holds_alternative<std::pair<uint64_t, signalr::value>>(Stack.top()))
    {
        std::get<std::pair<uint64_t, signalr::value>>(Stack.top()).second = Last;
    }
    else if (std::holds_alternative<std::pair<std::string, signalr::value>>(Stack.top()))
    {
        std::get<std::pair<std::string, signalr::value>>(Stack.top()).second = Last;
    }
    else
    {
        throw std::runtime_error("Unexpected serializer state");
    }
}

void SignalRSerializer::WriteValueInternal(const SignalRSerializableValue& Value)
{
    std::visit([this](auto&& ValType) { WriteValueInternal(ValType); }, Value);
}

SignalRDeserializer::SignalRDeserializer(const signalr::value& Object)
    : Root { Object }
{
    ObjectStack.push(nullptr);
}

SignalRDeserializer::SignalRDeserializer(signalr::value&& Object)
    : Root { std::move(Object) }
{
    ObjectStack.push(nullptr);
}

void SignalRDeserializer::EnterArray(size_t& Size)
{
    const signalr::value& ArrayObject = ReadNextValue();

    if (ArrayObject.is_array() == false)
    {
        throw std::runtime_error("Unexpected value: Value isn't an array");
    }

    ObjectStack.push(ArrayObject.as_array().begin());

    Size = ArrayObject.as_array().size();
}

void SignalRDeserializer::ExitArray()
{
    if (std::holds_alternative<std::vector<signalr::value>::const_iterator>(ObjectStack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Deserializer was not in an array");
    }

    ObjectStack.pop();
}

void SignalRDeserializer::EnterUintMap(size_t& Size)
{
    const signalr::value& MapObject = ReadNextValue();

    if (MapObject.is_uint_map() == false)
    {
        throw std::runtime_error("Unexpected value: Value isn't a uint map");
    }

    ObjectStack.push(MapObject.as_uint_map().begin());

    Size = MapObject.as_uint_map().size();
}
void SignalRDeserializer::ExitUintMap()
{
    if (std::holds_alternative<std::map<uint64_t, signalr::value>::const_iterator>(ObjectStack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Deserializer was not in a uint map");
    }

    ObjectStack.pop();
}

void SignalRDeserializer::EnterStringMap(size_t& Size)
{
    const signalr::value& MapObject = ReadNextValue();

    if (MapObject.is_string_map() == false)
    {
        throw std::runtime_error("Unexpected value: Value isn't a string map");
    }

    ObjectStack.push(MapObject.as_string_map().begin());

    Size = MapObject.as_string_map().size();
}

void SignalRDeserializer::ExitStringMap()
{
    if (std::holds_alternative<std::map<std::string, signalr::value>::const_iterator>(ObjectStack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Deserializer was not in a string map");
    }

    ObjectStack.pop();
}

const signalr::value& SignalRDeserializer::ReadNextValue()
{
    if (std::holds_alternative<std::vector<signalr::value>::const_iterator>(ObjectStack.top()))
    {
        return *std::get<std::vector<signalr::value>::const_iterator>(ObjectStack.top());
    }
    else if (std::holds_alternative<nullptr_t>(ObjectStack.top()))
    {
        return Root;
    }
    else
    {
        throw std::runtime_error("Unexpected deserializer state");
    }
}
const std::pair<std::uint64_t, signalr::value> SignalRDeserializer::ReadNextUintKeyValue() const
{
    if (std::holds_alternative<std::map<uint64_t, signalr::value>::const_iterator>(ObjectStack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Deserializer was not in a uint map");
    }

    return *std::get<std::map<uint64_t, signalr::value>::const_iterator>(ObjectStack.top());
}

const std::pair<std::string, signalr::value> SignalRDeserializer::ReadNextStringKeyValue() const
{
    if (std::holds_alternative<std::map<std::string, signalr::value>::const_iterator>(ObjectStack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Deserializer was not in a string map");
    }

    return *std::get<std::map<std::string, signalr::value>::const_iterator>(ObjectStack.top());
}

void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, int64_t& OutVal) const
{
    if (Object.is_integer() == false)
    {
        throw std::runtime_error("Invalid call: Value was not an integer");
    }

    OutVal = Object.as_integer();
}

void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, uint64_t& OutVal) const
{
    if (Object.is_uinteger() == false)
    {
        throw std::runtime_error("Invalid call: Value was not a uinteger");
    }

    OutVal = Object.as_uinteger();
}

void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, double& OutVal) const
{
    if (Object.is_double() == false)
    {
        throw std::runtime_error("Invalid call: Value was not a double");
    }

    OutVal = Object.as_double();
}
void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, bool& OutVal) const
{
    if (Object.is_bool() == false)
    {
        throw std::runtime_error("Invalid call: Value was not a bool");
    }

    OutVal = Object.as_bool();
}

void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, std::string& OutVal) const
{
    if (Object.is_string() == false)
    {
        throw std::runtime_error("Invalid call: Value was not a string");
    }

    OutVal = Object.as_string();
}

void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, nullptr_t&) const
{
    if (Object.is_null() == false)
    {
        throw std::runtime_error("Invalid call: Value was not null");
    }
}
void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, SignalRSerializableValue& OutVal) const
{
    if (Object.is_integer())
    {
        OutVal = Object.as_integer();
    }
    else if (Object.is_uinteger())
    {
        OutVal = Object.as_uinteger();
    }
    else if (Object.is_double())
    {
        OutVal = Object.as_double();
    }
    else if (Object.is_bool())
    {
        OutVal = Object.as_bool();
    }
    else if (Object.is_string())
    {
        OutVal = Object.as_string();
    }
    else if (Object.is_null())
    {
        OutVal = nullptr;
    }
    else
    {
        throw std::runtime_error("IUnexpected value: Value wasn't a supported variant type");
    }
}

void SignalRDeserializer::IncrementIterator()
{
    if (std::holds_alternative<std::vector<signalr::value>::const_iterator>(ObjectStack.top()))
    {
        std::get<std::vector<signalr::value>::const_iterator>(ObjectStack.top())++;
    }
    else if (std::holds_alternative<std::map<uint64_t, signalr::value>::const_iterator>(ObjectStack.top()))
    {
        std::get<std::map<uint64_t, signalr::value>::const_iterator>(ObjectStack.top())++;
    }
    else if (std::holds_alternative<std::map<std::string, signalr::value>::const_iterator>(ObjectStack.top()))
    {
        std::get<std::map<std::string, signalr::value>::const_iterator>(ObjectStack.top())++;
    }
}
}
