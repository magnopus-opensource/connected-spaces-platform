#include "SignalRSerializer.h"

namespace csp::multiplayer
{
void SignalRSerializer::StartWriteArray() { Stack.push(std::vector<signalr::value> {}); }

void SignalRSerializer::EndWriteArray()
{
    if (Stack.size() == 0 || std::holds_alternative<std::vector<signalr::value>>(Stack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Serializer was not in an array");
    }

    auto ArrayObject = std::move(Stack.top());
    Stack.pop();
    FinalizeContainerSerialization(signalr::value { std::get<std::vector<signalr::value>>(ArrayObject) });
}

void SignalRSerializer::StartWriteStringMap() { Stack.push(std::map<std::string, signalr::value> {}); }

void SignalRSerializer::EndWriteStringMap()
{
    if (Stack.size() == 0 || std::holds_alternative<std::map<std::string, signalr::value>>(Stack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Serializer was not in a string map");
    }

    auto StringMapObject = std::move(Stack.top());
    Stack.pop();
    FinalizeContainerSerialization(signalr::value { std::get<std::map<std::string, signalr::value>>(StringMapObject) });
}

void SignalRSerializer::StartWriteUintMap() { Stack.push(std::map<uint64_t, signalr::value> {}); }

void SignalRSerializer::EndWriteUintMap()
{
    if (Stack.size() == 0 || std::holds_alternative<std::map<std::uint64_t, signalr::value>>(Stack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Serializer was not in a uint map");
    }

    auto UintMapObject = std::move(Stack.top());
    Stack.pop();
    FinalizeContainerSerialization(signalr::value { std::get<std::map<uint64_t, signalr::value>>(UintMapObject) });
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

    if (std::holds_alternative<signalr::value>(Stack.top()))
    {
        return std::get<signalr::value>(Stack.top());
    }
    else if (std::holds_alternative<std::vector<signalr::value>>(Stack.top()))
    {
        return signalr::value { std::get<std::vector<signalr::value>>(Stack.top()) };
    }
    else if (std::holds_alternative<std::map<uint64_t, signalr::value>>(Stack.top()))
    {
        return signalr::value { std::get<std::map<uint64_t, signalr::value>>(Stack.top()) };
    }
    else if (std::holds_alternative<std::map<std::string, signalr::value>>(Stack.top()))
    {
        return signalr::value { std::get<std::map<std::string, signalr::value>>(Stack.top()) };
    }
    else
    {
        throw std::runtime_error("Unexpected serializer state");
    }
}

void SignalRSerializer::FinalizeContainerSerialization(signalr::value&& SerializedContainer)
{
    // We dont check for maps in this function because key-values are handled separately.
    if (Stack.size() == 0)
    {
        // If the last element has been popped, push back a root value.
        Stack.push(SerializedContainer);
        return;
    }

    // Dispatch internal variant type to the correct FinalizeContainerSerializationInternal call.
    std::visit([this, &SerializedContainer](auto& ValType) { this->FinalizeContainerSerializationInternal(ValType, std::move(SerializedContainer)); },
        Stack.top());
}

void SignalRSerializer::FinalizeContainerSerializationInternal(std::vector<signalr::value>& Container, signalr::value&& SerializedContainer)
{
    Container.push_back(SerializedContainer);
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

void SignalRDeserializer::StartReadArray(size_t& Size)
{
    const signalr::value& ArrayObject = ReadNextValue();

    if (ArrayObject.is_array() == false)
    {
        throw std::runtime_error("Unexpected value: Value isn't an array");
    }

    ObjectStack.push(ArrayObject.as_array().begin());

    Size = ArrayObject.as_array().size();
}

void SignalRDeserializer::EndReadArray()
{
    EndReadArrayInternal();
    IncrementIterator();
}

void SignalRDeserializer::StartReadUintMap(size_t& Size)
{
    const signalr::value& MapObject = ReadNextValue();

    if (MapObject.is_uint_map() == false)
    {
        throw std::runtime_error("Unexpected value: Value isn't a uint map");
    }

    ObjectStack.push(MapObject.as_uint_map().begin());

    Size = MapObject.as_uint_map().size();
}
void SignalRDeserializer::EndReadUintMap()
{
    EndReadUintMapInternal();
    IncrementIterator();
}

void SignalRDeserializer::StartReadStringMap(size_t& Size)
{
    const signalr::value& MapObject = ReadNextValue();

    if (MapObject.is_string_map() == false)
    {
        throw std::runtime_error("Unexpected value: Value isn't a string map");
    }

    ObjectStack.push(MapObject.as_string_map().begin());

    Size = MapObject.as_string_map().size();
}

void SignalRDeserializer::EndReadStringMap()
{
    EndReadStringMapInternal();
    IncrementIterator();
}

void SignalRDeserializer::Skip() { IncrementIterator(); }

bool SignalRDeserializer::NextValueIsInt() const
{
    const signalr::value& Next = ReadNextValue();
    return Next.is_integer();
}

bool SignalRDeserializer::NextValueIsUint() const
{
    const signalr::value& Next = ReadNextValue();
    return Next.is_uinteger();
}

bool SignalRDeserializer::NextValueIsNull() const
{
    const signalr::value& Next = ReadNextValue();
    return Next.is_null();
}

const signalr::value& SignalRDeserializer::ReadNextValue() const
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

void SignalRDeserializer::EndReadArrayInternal()
{
    if (std::holds_alternative<std::vector<signalr::value>::const_iterator>(ObjectStack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Deserializer was not in an array");
    }

    ObjectStack.pop();
}

void SignalRDeserializer::EndReadUintMapInternal()
{
    if (std::holds_alternative<std::map<uint64_t, signalr::value>::const_iterator>(ObjectStack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Deserializer was not in a uint map");
    }

    ObjectStack.pop();
}

void SignalRDeserializer::EndReadStringMapInternal()
{
    if (std::holds_alternative<std::map<std::string, signalr::value>::const_iterator>(ObjectStack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Deserializer was not in a string map");
    }

    ObjectStack.pop();
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

void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, double& OutVal)
{
    if (Object.is_double() == false)
    {
        throw std::runtime_error("Invalid call: Value was not a double");
    }

    OutVal = Object.as_double();
}

void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, float& OutVal)
{
    if (Object.is_double() == false)
    {
        throw std::runtime_error("Invalid call: Value was not a double");
    }

    OutVal = static_cast<float>(Object.as_double());
}

void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, bool& OutVal)
{
    if (Object.is_bool() == false)
    {
        throw std::runtime_error("Invalid call: Value was not a bool");
    }

    OutVal = Object.as_bool();
}

void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, std::string& OutVal)
{
    if (Object.is_string() == false)
    {
        throw std::runtime_error("Invalid call: Value was not a string");
    }

    OutVal = Object.as_string();
}

void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, nullptr_t)
{
    if (Object.is_null() == false)
    {
        throw std::runtime_error("Invalid call: Value was not null");
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
