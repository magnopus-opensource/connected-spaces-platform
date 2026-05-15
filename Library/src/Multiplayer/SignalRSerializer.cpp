#include "SignalRSerializer.h"

namespace csp::multiplayer
{
void SignalRSerializer::StartWriteArray() { m_stack.push(std::vector<signalr::value> {}); }

void SignalRSerializer::EndWriteArray()
{
    if (m_stack.size() == 0 || std::holds_alternative<std::vector<signalr::value>>(m_stack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Serializer was not in an array");
    }

    auto arrayObject = std::move(m_stack.top());
    m_stack.pop();
    FinalizeContainerSerialization(signalr::value { std::get<std::vector<signalr::value>>(arrayObject) });
}

void SignalRSerializer::StartWriteStringMap() { m_stack.push(std::map<std::string, signalr::value> {}); }

void SignalRSerializer::EndWriteStringMap()
{
    if (m_stack.size() == 0 || std::holds_alternative<std::map<std::string, signalr::value>>(m_stack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Serializer was not in a string map");
    }

    auto stringMapObject = std::move(m_stack.top());
    m_stack.pop();
    FinalizeContainerSerialization(signalr::value { std::get<std::map<std::string, signalr::value>>(stringMapObject) });
}

void SignalRSerializer::StartWriteUintMap() { m_stack.push(std::map<uint64_t, signalr::value> {}); }

void SignalRSerializer::EndWriteUintMap()
{
    if (m_stack.size() == 0 || std::holds_alternative<std::map<std::uint64_t, signalr::value>>(m_stack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Serializer was not in a uint map");
    }

    auto uintMapObject = std::move(m_stack.top());
    m_stack.pop();
    FinalizeContainerSerialization(signalr::value { std::get<std::map<uint64_t, signalr::value>>(uintMapObject) });
}

signalr::value SignalRSerializer::Get() const
{
    if (m_stack.size() == 0)
    {
        return signalr::value();
    }

    if (m_stack.size() > 1)
    {
        throw std::runtime_error("Invalid call: Serializer is not at the root");
    }

    if (std::holds_alternative<signalr::value>(m_stack.top()))
    {
        return std::get<signalr::value>(m_stack.top());
    }
    else if (std::holds_alternative<std::vector<signalr::value>>(m_stack.top()))
    {
        return signalr::value { std::get<std::vector<signalr::value>>(m_stack.top()) };
    }
    else if (std::holds_alternative<std::map<uint64_t, signalr::value>>(m_stack.top()))
    {
        return signalr::value { std::get<std::map<uint64_t, signalr::value>>(m_stack.top()) };
    }
    else if (std::holds_alternative<std::map<std::string, signalr::value>>(m_stack.top()))
    {
        return signalr::value { std::get<std::map<std::string, signalr::value>>(m_stack.top()) };
    }
    else
    {
        throw std::runtime_error("Unexpected serializer state");
    }
}

void SignalRSerializer::FinalizeContainerSerialization(signalr::value&& serializedContainer)
{
    // We dont check for maps in this function because key-values are handled separately.
    if (m_stack.size() == 0)
    {
        // If the last element has been popped, push back a root value.
        m_stack.push(serializedContainer);
        return;
    }

    // Dispatch internal variant type to the correct FinalizeContainerSerializationInternal call.
    std::visit([this, &serializedContainer](auto& valType) { this->FinalizeContainerSerializationInternal(valType, std::move(serializedContainer)); },
        m_stack.top());
}

void SignalRSerializer::FinalizeContainerSerializationInternal(std::vector<signalr::value>& container, signalr::value&& serializedContainer)
{
    container.push_back(serializedContainer);
}

SignalRDeserializer::SignalRDeserializer(const signalr::value& object)
    : m_root { object }
{
    m_objectStack.push(nullptr);
}

SignalRDeserializer::SignalRDeserializer(signalr::value&& object)
    : m_root { std::move(object) }
{
    m_objectStack.push(nullptr);
}

void SignalRDeserializer::StartReadArray(size_t& size)
{
    const signalr::value& arrayObject = ReadNextValue();

    if (arrayObject.is_array() == false)
    {
        throw std::runtime_error("Unexpected value: Value isn't an array");
    }

    m_objectStack.push(arrayObject.as_array().begin());

    size = arrayObject.as_array().size();
}

void SignalRDeserializer::EndReadArray()
{
    EndReadArrayInternal();
    IncrementIterator();
}

void SignalRDeserializer::StartReadUintMap(size_t& size)
{
    const signalr::value& mapObject = ReadNextValue();

    if (mapObject.is_uint_map() == false)
    {
        throw std::runtime_error("Unexpected value: Value isn't a uint map");
    }

    m_objectStack.push(mapObject.as_uint_map().begin());

    size = mapObject.as_uint_map().size();
}
void SignalRDeserializer::EndReadUintMap()
{
    EndReadUintMapInternal();
    IncrementIterator();
}

void SignalRDeserializer::StartReadStringMap(size_t& size)
{
    const signalr::value& mapObject = ReadNextValue();

    if (mapObject.is_string_map() == false)
    {
        throw std::runtime_error("Unexpected value: Value isn't a string map");
    }

    m_objectStack.push(mapObject.as_string_map().begin());

    size = mapObject.as_string_map().size();
}

void SignalRDeserializer::EndReadStringMap()
{
    EndReadStringMapInternal();
    IncrementIterator();
}

void SignalRDeserializer::Skip() { IncrementIterator(); }

bool SignalRDeserializer::NextValueIsInt() const
{
    const signalr::value& next = ReadNextValue();
    return next.is_integer();
}

bool SignalRDeserializer::NextValueIsUint() const
{
    const signalr::value& next = ReadNextValue();
    return next.is_uinteger();
}

bool SignalRDeserializer::NextValueIsNull() const
{
    const signalr::value& next = ReadNextValue();
    return next.is_null();
}

const signalr::value& SignalRDeserializer::ReadNextValue() const
{
    if (std::holds_alternative<std::vector<signalr::value>::const_iterator>(m_objectStack.top()))
    {
        return *std::get<std::vector<signalr::value>::const_iterator>(m_objectStack.top());
    }
    else if (std::holds_alternative<std::nullptr_t>(m_objectStack.top()))
    {
        return m_root;
    }
    else
    {
        throw std::runtime_error("Unexpected deserializer state");
    }
}

void SignalRDeserializer::EndReadArrayInternal()
{
    if (std::holds_alternative<std::vector<signalr::value>::const_iterator>(m_objectStack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Deserializer was not in an array");
    }

    m_objectStack.pop();
}

void SignalRDeserializer::EndReadUintMapInternal()
{
    if (std::holds_alternative<std::map<uint64_t, signalr::value>::const_iterator>(m_objectStack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Deserializer was not in a uint map");
    }

    m_objectStack.pop();
}

void SignalRDeserializer::EndReadStringMapInternal()
{
    if (std::holds_alternative<std::map<std::string, signalr::value>::const_iterator>(m_objectStack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Deserializer was not in a string map");
    }

    m_objectStack.pop();
}

const std::pair<std::uint64_t, signalr::value> SignalRDeserializer::ReadNextUintKeyValue() const
{
    if (std::holds_alternative<std::map<uint64_t, signalr::value>::const_iterator>(m_objectStack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Deserializer was not in a uint map");
    }

    return *std::get<std::map<uint64_t, signalr::value>::const_iterator>(m_objectStack.top());
}

const std::pair<std::string, signalr::value> SignalRDeserializer::ReadNextStringKeyValue() const
{
    if (std::holds_alternative<std::map<std::string, signalr::value>::const_iterator>(m_objectStack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Deserializer was not in a string map");
    }

    return *std::get<std::map<std::string, signalr::value>::const_iterator>(m_objectStack.top());
}

void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& object, double& outVal)
{
    if (object.is_double() == false)
    {
        throw std::runtime_error("Invalid call: Value was not a double");
    }

    outVal = object.as_double();
}

void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& object, float& outVal)
{
    if (object.is_double() == false)
    {
        throw std::runtime_error("Invalid call: Value was not a double");
    }

    outVal = static_cast<float>(object.as_double());
}

void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& object, bool& outVal)
{
    if (object.is_bool() == false)
    {
        throw std::runtime_error("Invalid call: Value was not a bool");
    }

    outVal = object.as_bool();
}

void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& object, std::string& outVal)
{
    if (object.is_string() == false)
    {
        throw std::runtime_error("Invalid call: Value was not a string");
    }

    outVal = object.as_string();
}

void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& object, std::nullptr_t)
{
    if (object.is_null() == false)
    {
        throw std::runtime_error("Invalid call: Value was not null");
    }
}

void SignalRDeserializer::IncrementIterator()
{
    if (std::holds_alternative<std::vector<signalr::value>::const_iterator>(m_objectStack.top()))
    {
        std::get<std::vector<signalr::value>::const_iterator>(m_objectStack.top())++;
    }
    else if (std::holds_alternative<std::map<uint64_t, signalr::value>::const_iterator>(m_objectStack.top()))
    {
        std::get<std::map<uint64_t, signalr::value>::const_iterator>(m_objectStack.top())++;
    }
    else if (std::holds_alternative<std::map<std::string, signalr::value>::const_iterator>(m_objectStack.top()))
    {
        std::get<std::map<std::string, signalr::value>::const_iterator>(m_objectStack.top())++;
    }
}
}
