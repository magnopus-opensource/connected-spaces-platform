/*
 * Copyright 2025 Magnopus LLC

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

#include "SignalRSerializerTypeTraits.h"

#include <limits>
#include <map>
#include <stack>
#include <stdexcept>

namespace csp::multiplayer
{

class ISignalRSerializable;

/// @brief A serializer which allows for custom class serialization using ISignalRSerializable interface.
/// @details Writing single values is supported. However, if you want to serialize multiple values,
/// you will need to add a container, by calling WriteValue on one of the supported containers,
/// or calling StartWrite[X].
///
/// Currently supported types:
///     - Unsigned integers
///     - Signed integers
///     - Doubles
///     - Floats
///     - String
///     - Null pointers (represents null)
///     - Optionals
///     - Vectors
///     - Unsigned integer key maps
///     - string key maps
///     - Types with ISignalRSerializable interface
class SignalRSerializer
{
public:
    /// @brief Writes a value to the current container of the serializer.
    /// @pre This function should be used if this serializer represents a single value,
    /// or if StartArray is called first to write to the array.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> WriteValue(const T& value);

    /// @brief Writes a uint key-value pair to the current uint map.
    /// @pre StartUintMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <typename K, typename T>
    std::enable_if_t<IsUnsignedIntegerV<K> && IsSupportedSignalRType<T>::value> WriteKeyValue(K key, const T& value);

    /// @brief Writes a string key-value pair to the current map.
    /// @pre StartStringMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> WriteKeyValue(std::string key, const T& value);

    /// @brief Starts writing an array into the serializer.
    /// @details Once this function has been called, WriteValue should be used to add elements to the array.
    /// Start/End functions should be used if you need custom serialization logic.
    /// PopArray should be used to finalize the array.
    void StartWriteArray();

    /// @brief Ends the current array in the serializer.
    /// @pre StartWriteArray should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    void EndWriteArray();

    /// @brief Starts writing a string map (std::map<std::string, T> into the serializer.
    /// @details Once this function has been called, WriteKeyValue should be used to add elements to the map.
    /// Start/End functions should be used if you need custom serialization logic.
    /// PopStringMap should be used to finalize the map.
    void StartWriteStringMap();

    /// @brief Ends the current string map in the serializer.
    /// @pre StartWriteStringMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    void EndWriteStringMap();

    /// @brief Starts writing a uint map (std::map<uint64_t, T> into the serializer.
    /// @details Once this function has been called, WriteKeyValue should be used to add elements to the map.
    /// Start/End functions should be used if you need custom serialization logic.
    /// PopUintMap should be used to finalize the map.
    void StartWriteUintMap();

    /// @brief Ends the current uint map in the serializer.
    /// @pre StartWriteUintMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    void EndWriteUintMap();

    /// @brief Gets the serialized singnal r value.
    /// @return signalr::value
    /// @pre The serializer should be at the root (array and maps should all be ended using EndWrite[X]).
    signalr::value Get() const;

private:
    // Adds the serialized container object to the previous object, or the root.
    // This will internally call one of the FinalizeContainerSerializationInternal overrides.
    void FinalizeContainerSerialization(signalr::value&& serializedContainer);

    // Case where we fallback to an exception is we are trying to serialize to an unsupported container.
    template <typename T> void FinalizeContainerSerializationInternal(T&, signalr::value&& serializedContainer);
    // Case for serializing value into a vector.
    void FinalizeContainerSerializationInternal(std::vector<signalr::value>& container, signalr::value&& serializedContainer);
    // Case for serializing value into a pair for a map.
    template <typename K> void FinalizeContainerSerializationInternal(std::pair<K, signalr::value>& pair, signalr::value&& serializedContainer);

    // Writes all of our primitive types to a signalr value.
    template <typename T> void WriteValueInternal(const T& value);

    // Case for writing container types to a signalr value.
    // These cases are recursive.
    template <typename T> void WriteValueInternal(const std::optional<T>& value);
    template <typename T> void WriteValueInternal(const std::vector<T>& value);
    template <typename K, typename T> void WriteValueInternal(const std::map<K, T>& value);
    template <typename T> void WriteValueInternal(const std::map<std::string, T>& value);

    // Converts primitive value to a signalr object.
    template <typename T> signalr::value CreateSignalRObject(const T& value);

    using Container = std::variant<signalr::value, std::vector<signalr::value>, std::map<std::string, signalr::value>,
        std::map<uint64_t, signalr::value>, std::pair<uint64_t, signalr::value>, std::pair<std::string, signalr::value>>;

    std::stack<Container> m_stack;
};

/// @brief A signalr deserializer which allows for custom class deserialization using ISignalRDeserializable interface.
/// @details This supports all types outlined in the SignalRSerializer, except ISignalRDeserializable is needed for deserialization.
/// If ISignalRDeserializable types are being deserialized from a container, then need to be default constructable.
class SignalRDeserializer
{
public:
    /// @brief Constructor used to copy object to deserialize.
    /// @param Object const signalr::value& : The value to deserialize
    /// This should match the structure generated by the SignalRSerializer.
    SignalRDeserializer(const signalr::value& object);

    /// @brief Constructor used to move object to deserialize.
    /// @param Object signalr::value&& : The value to deserialize
    /// This should match the structure generated by the SignalRSerializer.
    SignalRDeserializer(signalr::value&& object);

    /// @brief Reads a value from the internal signalr array.
    /// @pre This function should be used if this serializer represents a single value,
    /// or if StartReadArray is called first to write to the array.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> ReadValue(T& outVal);

    /// @brief Reads a uint key-value pair to the current uint map.
    /// @pre EnterUintMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <typename K, typename T>
    std::enable_if_t<IsUnsignedIntegerV<K> && IsSupportedSignalRType<T>::value> ReadKeyValue(std::pair<K, T>& outVal);

    /// @brief Reads a string key-value pair to the current string map.
    /// @pre EnterStringMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> ReadKeyValue(std::pair<std::string, T>& outVal);

    /// @brief Enters the internal signalr value as an array
    /// @details Once this function has been called, ReadValue should be used to read elements from the array.
    /// @param Size size_t& Output parameter specifying the size of the array.
    /// ExitArray should be used to exit the array once elements have been read.
    void StartReadArray(size_t& size);

    /// @brief Exits the internal signalr array.
    /// @pre StartReadArray should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    void EndReadArray();

    /// @brief Enters the internal signalr value as an uint map
    /// @details Once this function has been called, ReadKeyValue should be used to read elements from the map.
    /// @param Size size_t& Output parameter specifying the size of the map.
    /// ExitUintMap should be used to exit the map once elements have been read.
    void StartReadUintMap(size_t& size);

    /// @brief Exits the internal signalr uint map.
    /// @pre StartReadUintMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    void EndReadUintMap();

    /// @brief Enters the internal signalr value as an string map
    /// @details Once this function has been called, ReadKeyValue should be used to read elements from the map.
    /// @param Size size_t& Output parameter specifying the size of the map.
    /// ExitStringMap should be used to exit the map once elements have been read.
    void StartReadStringMap(size_t& size);

    /// @brief Exits the internal signalr string map.
    /// @pre StartReadStringMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    void EndReadStringMap();

    /// @brief Skips the next value to be read.
    void Skip();

    /// @brief Returns true if the next value to read is a signed integer.
    /// @return bool
    bool NextValueIsInt() const;

    /// @brief Returns true if the next value to read is an unsigned integer.
    /// @return bool
    bool NextValueIsUint() const;

    /// @brief Returns true if the next value to read is null.
    /// @return bool
    bool NextValueIsNull() const;

private:
    // Reads the next signalr value using the current iterator.
    const signalr::value& ReadNextValue() const;

    void EndReadArrayInternal();
    void EndReadUintMapInternal();
    void EndReadStringMapInternal();

    const std::pair<std::uint64_t, signalr::value> ReadNextUintKeyValue() const;
    const std::pair<std::string, signalr::value> ReadNextStringKeyValue() const;

    // Reads the specified type from the given signalr object.
    // This internally calls the ReadValueFromObjectInternal for the given type.
    template <typename T> void ReadValueFromObject(const signalr::value& object, T& outVal);

    // Case for reading signed integers.
    template <typename T> std::enable_if_t<IsSignedIntegerV<T>> ReadValueFromObjectInternal(const signalr::value& object, T& outVal);
    // Case for reading unsigned integers.
    template <typename T> std::enable_if_t<IsUnsignedIntegerV<T>> ReadValueFromObjectInternal(const signalr::value& object, T& outVal);
    // Case for other primitive types.
    void ReadValueFromObjectInternal(const signalr::value& object, double& outVal);
    void ReadValueFromObjectInternal(const signalr::value& object, float& outVal);
    void ReadValueFromObjectInternal(const signalr::value& object, bool& outVal);
    void ReadValueFromObjectInternal(const signalr::value& object, std::string& outVal);
    void ReadValueFromObjectInternal(const signalr::value& object, std::nullptr_t);

    // Case for other container values.
    template <typename T> void ReadValueFromObjectInternal(const signalr::value& object, std::optional<T>& outVal);
    template <typename T> void ReadValueFromObjectInternal(const signalr::value& object, std::vector<T>& outVal);
    template <typename K, typename T> void ReadValueFromObjectInternal(const signalr::value& object, std::map<K, T>& outVal);
    template <typename T> void ReadValueFromObjectInternal(const signalr::value& object, std::map<std::string, T>& outVal);

    void IncrementIterator();

    using Iterator = std::variant<std::nullptr_t, std::vector<signalr::value>::const_iterator, std::map<uint64_t, signalr::value>::const_iterator,
        std::map<std::string, signalr::value>::const_iterator>;

    signalr::value m_root;
    std::stack<Iterator> m_objectStack;
};

/// @brief A serializer interface to allow classes to be serialized.
class ISignalRSerializable
{
public:
    /// @brief Function internally called when serializing this class using SignalRSerializer.
    virtual void Serialize(SignalRSerializer&) const = 0;

    virtual ~ISignalRSerializable() = default;

protected:
    ISignalRSerializable() = default;

    ISignalRSerializable(const ISignalRSerializable&) = default;
    ISignalRSerializable(ISignalRSerializable&&) = default;

    ISignalRSerializable& operator=(const ISignalRSerializable&) = default;
    ISignalRSerializable& operator=(ISignalRSerializable&&) = default;
};

/// @brief A deserializer interace to allow classes to be deserialized.
class ISignalRDeserializable
{
public:
    /// @brief Function internally called when serializing this class using SignalRDeserializer.
    virtual void Deserialize(SignalRDeserializer&) = 0;

    virtual ~ISignalRDeserializable() = default;

protected:
    ISignalRDeserializable() = default;

    ISignalRDeserializable(const ISignalRDeserializable&) = default;
    ISignalRDeserializable(ISignalRDeserializable&&) = default;

    ISignalRDeserializable& operator=(const ISignalRDeserializable&) = default;
    ISignalRDeserializable& operator=(ISignalRDeserializable&&) = default;
};

template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> SignalRSerializer::WriteValue(const T& value) { WriteValueInternal(value); }

template <typename K, typename T>
std::enable_if_t<IsUnsignedIntegerV<K> && IsSupportedSignalRType<T>::value> SignalRSerializer::WriteKeyValue(K key, const T& value)
{
    if (m_stack.size() == 0 || std ::holds_alternative<std::map<uint64_t, signalr::value>>(m_stack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Serializer was not in a uint map");
    }

    m_stack.push(std::pair<uint64_t, signalr::value> {});
    std::get<std::pair<uint64_t, signalr::value>>(m_stack.top()).first = static_cast<uint64_t>(key);

    WriteValue(value);

    // Get our pair from the top of the stack and pop
    std::pair<uint64_t, signalr::value> pair = std::get<std::pair<uint64_t, signalr::value>>(m_stack.top());
    m_stack.pop();

    // Get our map from the top of the stack and append the pair
    std::map<uint64_t, signalr::value>& map = std::get<std::map<uint64_t, signalr::value>>(m_stack.top());
    map[pair.first] = pair.second;
}

template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> SignalRSerializer::WriteKeyValue(std::string key, const T& value)
{
    if (m_stack.size() == 0 || std ::holds_alternative<std::map<std::string, signalr::value>>(m_stack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Serializer was not in a string map");
    }

    m_stack.push(std::pair<std::string, signalr::value> {});
    std::get<std::pair<std::string, signalr::value>>(m_stack.top()).first = key;

    WriteValue(value);

    // Get our pair from the top of the stack and pop.
    std::pair<std::string, signalr::value> pair = std::get<std::pair<std::string, signalr::value>>(m_stack.top());
    m_stack.pop();

    // Get our map from the top of the stack and append the pair.
    std::map<std::string, signalr::value>& map = std::get<std::map<std::string, signalr::value>>(m_stack.top());
    map[pair.first] = pair.second;
}

template <typename T> inline void SignalRSerializer::FinalizeContainerSerializationInternal(T&, signalr::value&&)
{
    throw std::runtime_error("Unexpected serializer state");
}

template <typename K>
inline void SignalRSerializer::FinalizeContainerSerializationInternal(std::pair<K, signalr::value>& pair, signalr::value&& serializedContainer)
{
    pair.second = serializedContainer;
}

template <typename T> inline void SignalRSerializer::WriteValueInternal(const T& value)
{
    signalr::value serializedValue;

    if constexpr (IsCustomSerializableV<T>)
    {
        value.Serialize(*this);
    }
    else
    {
        if (m_stack.size() == 0)
        {
            // Case where a user only want to serialize a single value
            m_stack.push(CreateSignalRObject(value));
        }
        else if (std::holds_alternative<std::vector<signalr::value>>(m_stack.top()))
        {
            std::get<std::vector<signalr::value>>(m_stack.top()).push_back(CreateSignalRObject(value));
        }
        else if (std::holds_alternative<std::pair<uint64_t, signalr::value>>(m_stack.top()))
        {
            std::get<std::pair<uint64_t, signalr::value>>(m_stack.top()).second = CreateSignalRObject(value);
        }
        else if (std::holds_alternative<std::pair<std::string, signalr::value>>(m_stack.top()))
        {
            std::get<std::pair<std::string, signalr::value>>(m_stack.top()).second = CreateSignalRObject(value);
        }
        else
        {
            throw std::runtime_error("Invalid call: Serializer was not in an array or at the root");
        }
    }
}

template <typename T> inline void SignalRSerializer::WriteValueInternal(const std::optional<T>& value)
{
    if (value.has_value())
    {
        WriteValueInternal(*value);
    }
    else
    {
        WriteValueInternal<std::nullptr_t>(nullptr);
    }
}

template <typename T> inline void SignalRSerializer::WriteValueInternal(const std::vector<T>& value)
{
    StartWriteArray();

    for (const auto& element : value)
    {
        WriteValue(element);
    }

    EndWriteArray();
}

template <typename K, typename T> inline void SignalRSerializer::WriteValueInternal(const std::map<K, T>& value)
{
    StartWriteUintMap();

    for (const auto& pair : value)
    {
        WriteKeyValue(pair.first, pair.second);
    }

    EndWriteUintMap();
}

template <typename T> inline void SignalRSerializer::WriteValueInternal(const std::map<std::string, T>& value)
{
    StartWriteStringMap();

    for (const auto& pair : value)
    {
        WriteKeyValue(pair.first, pair.second);
    }

    EndWriteStringMap();
}

template <typename T> inline signalr::value SignalRSerializer::CreateSignalRObject(const T& value)
{
    // Allows us to support different integer types, as signalr only supports uint64 and int64.
    if constexpr (IsSignedIntegerV<T>)
    {
        return signalr::value { static_cast<int64_t>(value) };
    }
    else if constexpr (IsUnsignedIntegerV<T>)
    {
        return signalr::value { static_cast<uint64_t>(value) };
    }
    else
    {
        return signalr::value { value };
    }
}

template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> SignalRDeserializer::ReadValue(T& outVal)
{
    const signalr::value& next = ReadNextValue();
    ReadValueFromObject(next, outVal);

    IncrementIterator();
}

template <typename K, typename T>
std::enable_if_t<IsUnsignedIntegerV<K> && IsSupportedSignalRType<T>::value> SignalRDeserializer::ReadKeyValue(std::pair<K, T>& outVal)
{
    const std::pair<uint64_t, signalr::value>& next = ReadNextUintKeyValue();

    outVal.first = static_cast<K>(next.first);
    ReadValueFromObject(next.second, outVal.second);

    IncrementIterator();
}

template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> SignalRDeserializer::ReadKeyValue(std::pair<std::string, T>& outVal)
{
    const std::pair<std::string, signalr::value>& next = ReadNextStringKeyValue();

    outVal.first = next.first;
    ReadValueFromObject(next.second, outVal.second);

    IncrementIterator();
}

template <typename T> void SignalRDeserializer::ReadValueFromObject(const signalr::value& object, T& outVal)
{
    if constexpr (IsCustomDeserializableV<T>)
    {
        SignalRDeserializer deserializer { object };
        outVal.Deserialize(deserializer);
    }
    else if constexpr (IsSupportedSignalRType<T>::value)
    {
        ReadValueFromObjectInternal(object, outVal);
    }
    else
    {
        static_assert(IsSupportedSignalRType<T>::value, "Type passed to ReadValueFromObject is not supported by SignalR deserialization.");
    }
}

template <typename T> std::enable_if_t<IsSignedIntegerV<T>> SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& object, T& outVal)
{
    if (object.is_integer() == false)
    {
        throw std::runtime_error("Invalid call: Value was not an integer");
    }

    if (object.as_integer() > std::numeric_limits<T>::max() || object.as_integer() < std::numeric_limits<T>::min())
    {
        throw std::runtime_error("Invalid uinteger type: Value being deserialized is larger than the maximum value of the input type");
    }

    outVal = static_cast<T>(object.as_integer());
}

template <typename T>
std::enable_if_t<IsUnsignedIntegerV<T>> SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& object, T& outVal)
{
    if (object.is_uinteger() == false)
    {
        throw std::runtime_error("Invalid call: Value was not a uinteger");
    }

    if (object.as_uinteger() > std::numeric_limits<T>::max())
    {
        throw std::runtime_error("Invalid uinteger type: Value being deserialized is larger than the maximum value of the input type");
    }

    outVal = static_cast<T>(object.as_uinteger());
}

template <typename T> inline void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& object, std::optional<T>& outVal)
{
    if (object.is_null())
    {
        outVal = std::nullopt;
    }
    else
    {
        outVal = T {};
        ReadValueFromObject(object, *outVal);
    }
}

template <typename T> inline void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value&, std::vector<T>& outVal)
{
    size_t arraySize = 0;
    StartReadArray(arraySize);

    outVal.resize(arraySize);

    for (size_t i = 0; i < arraySize; ++i)
    {
        ReadValue(outVal[i]);
    }

    EndReadArrayInternal();
}

template <typename K, typename T> inline void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value&, std::map<K, T>& outVal)
{
    size_t mapSize = 0;
    StartReadUintMap(mapSize);

    for (size_t i = 0; i < mapSize; ++i)
    {
        std::pair<K, T> pair;
        ReadKeyValue(pair);
        outVal[pair.first] = pair.second;
    }

    EndReadUintMapInternal();
}

template <typename T> inline void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value&, std::map<std::string, T>& outVal)
{
    size_t mapSize = 0;
    StartReadStringMap(mapSize);

    for (size_t i = 0; i < mapSize; ++i)
    {
        std::pair<std::string, T> pair;
        ReadKeyValue(pair);
        outVal[pair.first] = pair.second;
    }

    EndReadStringMapInternal();
}
}
