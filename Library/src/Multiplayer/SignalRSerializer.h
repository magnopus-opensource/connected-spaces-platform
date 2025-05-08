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
    template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> WriteValue(const T& Value);

    /// @brief Writes a uint key-value pair to the current uint map.
    /// @pre StartUintMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <typename K, typename T>
    std::enable_if_t<IsUnsignedIntegerV<K> && IsSupportedSignalRType<T>::value> WriteKeyValue(K Key, const T& Value);

    /// @brief Writes a string key-value pair to the current map.
    /// @pre StartStringMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> WriteKeyValue(std::string Key, const T& Value);

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
    void FinalizeContainerSerialization(signalr::value&& SerializedContainer);

    // Case where we fallback to an exception is we are trying to serialize to an unsupported container.
    template <typename T> void FinalizeContainerSerializationInternal(T&, signalr::value&& SerializedContainer);
    // Case for serializing value into a vector.
    void FinalizeContainerSerializationInternal(std::vector<signalr::value>& Container, signalr::value&& SerializedContainer);
    // Case for serializing value into a pair for a map.
    template <typename K> void FinalizeContainerSerializationInternal(std::pair<K, signalr::value>& Pair, signalr::value&& SerializedContainer);

    // Writes all of our primitive types to a signalr value.
    template <typename T> void WriteValueInternal(const T& Value);

    // Case for writing container types to a signalr value.
    // These cases are recursive.
    template <typename T> void WriteValueInternal(const std::optional<T>& Value);
    template <typename T> void WriteValueInternal(const std::vector<T>& Value);
    template <typename K, typename T> void WriteValueInternal(const std::map<K, T>& Value);
    template <typename T> void WriteValueInternal(const std::map<std::string, T>& Value);

    // Converts primitive value to a signalr object.
    template <typename T> signalr::value CreateSignalRObject(const T& Value);

    using Container = std::variant<signalr::value, std::vector<signalr::value>, std::map<std::string, signalr::value>,
        std::map<uint64_t, signalr::value>, std::pair<uint64_t, signalr::value>, std::pair<std::string, signalr::value>>;

    std::stack<Container> Stack;
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
    SignalRDeserializer(const signalr::value& Object);

    /// @brief Constructor used to move object to deserialize.
    /// @param Object signalr::value&& : The value to deserialize
    /// This should match the structure generated by the SignalRSerializer.
    SignalRDeserializer(signalr::value&& Object);

    /// @brief Reads a value from the internal signalr array.
    /// @pre This function should be used if this serializer represents a single value,
    /// or if StartReadArray is called first to write to the array.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> ReadValue(T& OutVal);

    /// @brief Reads a uint key-value pair to the current uint map.
    /// @pre EnterUintMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <typename K, typename T>
    std::enable_if_t<IsUnsignedIntegerV<K> && IsSupportedSignalRType<T>::value> ReadKeyValue(std::pair<K, T>& OutVal);

    /// @brief Reads a string key-value pair to the current string map.
    /// @pre EnterStringMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> ReadKeyValue(std::pair<std::string, T>& OutVal);

    /// @brief Enters the internal signalr value as an array
    /// @details Once this function has been called, ReadValue should be used to read elements from the array.
    /// @param Size size_t& Output parameter specifying the size of the array.
    /// ExitArray should be used to exit the array once elements have been read.
    void StartReadArray(size_t& Size);

    /// @brief Exits the internal signalr array.
    /// @pre StartReadArray should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    void EndReadArray();

    /// @brief Enters the internal signalr value as an uint map
    /// @details Once this function has been called, ReadKeyValue should be used to read elements from the map.
    /// @param Size size_t& Output parameter specifying the size of the map.
    /// ExitUintMap should be used to exit the map once elements have been read.
    void StartReadUintMap(size_t& Size);

    /// @brief Exits the internal signalr uint map.
    /// @pre StartReadUintMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    void EndReadUintMap();

    /// @brief Enters the internal signalr value as an string map
    /// @details Once this function has been called, ReadKeyValue should be used to read elements from the map.
    /// @param Size size_t& Output parameter specifying the size of the map.
    /// ExitStringMap should be used to exit the map once elements have been read.
    void StartReadStringMap(size_t& Size);

    /// @brief Exits the internal signalr string map.
    /// @pre StartReadStringMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    void EndReadStringMap();

private:
    // Reads the next signalr value using the current iterator.
    const signalr::value& ReadNextValue();

    void EndReadArrayInternal();
    void EndReadUintMapInternal();
    void EndReadStringMapInternal();

    const std::pair<std::uint64_t, signalr::value> ReadNextUintKeyValue() const;
    const std::pair<std::string, signalr::value> ReadNextStringKeyValue() const;

    // Reads the specified type from the given signalr object.
    // This internally calls the ReadValueFromObjectInternal for the given type.
    template <typename T> void ReadValueFromObject(const signalr::value& Object, T& OutVal);

    // Case for reading signed integers.
    template <typename T> std::enable_if_t<IsSignedIntegerV<T>> ReadValueFromObjectInternal(const signalr::value& Object, T& OutVal);
    // Case for reading unsigned integers.
    template <typename T> std::enable_if_t<IsUnsignedIntegerV<T>> ReadValueFromObjectInternal(const signalr::value& Object, T& OutVal);
    // Case for other primitive types.
    void ReadValueFromObjectInternal(const signalr::value& Object, double& OutVal);
    void ReadValueFromObjectInternal(const signalr::value& Object, float& OutVal);
    void ReadValueFromObjectInternal(const signalr::value& Object, bool& OutVal);
    void ReadValueFromObjectInternal(const signalr::value& Object, std::string& OutVal);
    void ReadValueFromObjectInternal(const signalr::value& Object, nullptr_t);

    // Case for other container values.
    template <typename T> void ReadValueFromObjectInternal(const signalr::value& Object, std::optional<T>& OutVal);
    template <typename T> void ReadValueFromObjectInternal(const signalr::value& Object, std::vector<T>& OutVal);
    template <typename K, typename T> void ReadValueFromObjectInternal(const signalr::value& Object, std::map<K, T>& OutVal);
    template <typename T> void ReadValueFromObjectInternal(const signalr::value& Object, std::map<std::string, T>& OutVal);

    void IncrementIterator();

    using Iterator = std::variant<nullptr_t, std::vector<signalr::value>::const_iterator, std::map<uint64_t, signalr::value>::const_iterator,
        std::map<std::string, signalr::value>::const_iterator>;

    signalr::value Root;
    std::stack<Iterator> ObjectStack;
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

template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> SignalRSerializer::WriteValue(const T& Value) { WriteValueInternal(Value); }

template <typename K, typename T>
std::enable_if_t<IsUnsignedIntegerV<K> && IsSupportedSignalRType<T>::value> SignalRSerializer::WriteKeyValue(K Key, const T& Value)
{
    if (Stack.size() == 0 || std ::holds_alternative<std::map<uint64_t, signalr::value>>(Stack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Serializer was not in a uint map");
    }

    Stack.push(std::pair<uint64_t, signalr::value> {});
    std::get<std::pair<uint64_t, signalr::value>>(Stack.top()).first = static_cast<uint64_t>(Key);

    WriteValue(Value);

    // Get our pair from the top of the stack and pop
    std::pair<uint64_t, signalr::value> Pair = std::get<std::pair<uint64_t, signalr::value>>(Stack.top());
    Stack.pop();

    // Get our map from the top of the stack and append the pair
    std::map<uint64_t, signalr::value>& Map = std::get<std::map<uint64_t, signalr::value>>(Stack.top());
    Map[Pair.first] = Pair.second;
}

template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> SignalRSerializer::WriteKeyValue(std::string Key, const T& Value)
{
    if (Stack.size() == 0 || std ::holds_alternative<std::map<std::string, signalr::value>>(Stack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Serializer was not in a string map");
    }

    Stack.push(std::pair<std::string, signalr::value> {});
    std::get<std::pair<std::string, signalr::value>>(Stack.top()).first = Key;

    WriteValue(Value);

    // Get our pair from the top of the stack and pop.
    std::pair<std::string, signalr::value> Pair = std::get<std::pair<std::string, signalr::value>>(Stack.top());
    Stack.pop();

    // Get our map from the top of the stack and append the pair.
    std::map<std::string, signalr::value>& Map = std::get<std::map<std::string, signalr::value>>(Stack.top());
    Map[Pair.first] = Pair.second;
}

template <typename T> inline void SignalRSerializer::FinalizeContainerSerializationInternal(T&, signalr::value&&)
{
    throw std::runtime_error("Unexpected serializer state");
}

template <typename K>
inline void SignalRSerializer::FinalizeContainerSerializationInternal(std::pair<K, signalr::value>& Pair, signalr::value&& SerializedContainer)
{
    Pair.second = SerializedContainer;
}

template <typename T> inline void SignalRSerializer::WriteValueInternal(const T& Value)
{
    signalr::value SerializedValue;

    if constexpr (IsCustomSerializableV<T>)
    {
        Value.Serialize(*this);
    }
    else
    {
        if (Stack.size() == 0)
        {
            // Case where a user only want to serialize a single value
            Stack.push(CreateSignalRObject(Value));
        }
        else if (std::holds_alternative<std::vector<signalr::value>>(Stack.top()))
        {
            std::get<std::vector<signalr::value>>(Stack.top()).push_back(CreateSignalRObject(Value));
        }
        else if (std::holds_alternative<std::pair<uint64_t, signalr::value>>(Stack.top()))
        {
            std::get<std::pair<uint64_t, signalr::value>>(Stack.top()).second = CreateSignalRObject(Value);
        }
        else if (std::holds_alternative<std::pair<std::string, signalr::value>>(Stack.top()))
        {
            std::get<std::pair<std::string, signalr::value>>(Stack.top()).second = CreateSignalRObject(Value);
        }
        else
        {
            throw std::runtime_error("Invalid call: Serializer was not in an array or at the root");
        }
    }
}

template <typename T> inline void SignalRSerializer::WriteValueInternal(const std::optional<T>& Value)
{
    if (Value.has_value())
    {
        WriteValueInternal(*Value);
    }
    else
    {
        WriteValueInternal<nullptr_t>(nullptr);
    }
}

template <typename T> inline void SignalRSerializer::WriteValueInternal(const std::vector<T>& Value)
{
    StartWriteArray();

    for (const auto& Element : Value)
    {
        WriteValue(Element);
    }

    EndWriteArray();
}

template <typename K, typename T> inline void SignalRSerializer::WriteValueInternal(const std::map<K, T>& Value)
{
    StartWriteUintMap();

    for (const auto& Pair : Value)
    {
        WriteKeyValue(Pair.first, Pair.second);
    }

    EndWriteUintMap();
}

template <typename T> inline void SignalRSerializer::WriteValueInternal(const std::map<std::string, T>& Value)
{
    StartWriteStringMap();

    for (const auto& Pair : Value)
    {
        WriteKeyValue(Pair.first, Pair.second);
    }

    EndWriteStringMap();
}

template <typename T> inline signalr::value SignalRSerializer::CreateSignalRObject(const T& Value)
{
    // Allows us to support different integer types, as signalr only supports uint64 and int64.
    if constexpr (IsSignedIntegerV<T>)
    {
        return signalr::value { static_cast<int64_t>(Value) };
    }
    else if constexpr (IsUnsignedIntegerV<T>)
    {
        return signalr::value { static_cast<uint64_t>(Value) };
    }
    else
    {
        return signalr::value { Value };
    }
}

template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> SignalRDeserializer::ReadValue(T& OutVal)
{
    const signalr::value& Next = ReadNextValue();
    ReadValueFromObject(Next, OutVal);

    IncrementIterator();
}

template <typename K, typename T>
std::enable_if_t<IsUnsignedIntegerV<K> && IsSupportedSignalRType<T>::value> SignalRDeserializer::ReadKeyValue(std::pair<K, T>& OutVal)
{
    const std::pair<uint64_t, signalr::value>& Next = ReadNextUintKeyValue();

    OutVal.first = static_cast<K>(Next.first);
    ReadValueFromObject(Next.second, OutVal.second);

    IncrementIterator();
}

template <typename T> std::enable_if_t<IsSupportedSignalRType<T>::value> SignalRDeserializer::ReadKeyValue(std::pair<std::string, T>& OutVal)
{
    const std::pair<std::string, signalr::value>& Next = ReadNextStringKeyValue();

    OutVal.first = Next.first;
    ReadValueFromObject(Next.second, OutVal.second);

    IncrementIterator();
}

template <typename T> void SignalRDeserializer::ReadValueFromObject(const signalr::value& Object, T& OutVal)
{
    if constexpr (IsCustomDeserializableV<T>)
    {
        SignalRDeserializer Deserializer { Object };
        OutVal.Deserialize(Deserializer);
    }
    else if constexpr (IsSupportedSignalRType<T>::value)
    {
        ReadValueFromObjectInternal(Object, OutVal);
    }
    else
    {
        static_assert(IsSupportedSignalRType<T>::value, "Type passed to ReadValueFromObject is not supported by SignalR deserialization.");
    }
}

template <typename T> std::enable_if_t<IsSignedIntegerV<T>> SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, T& OutVal)
{
    if (Object.is_integer() == false)
    {
        throw std::runtime_error("Invalid call: Value was not an integer");
    }

    OutVal = static_cast<T>(Object.as_integer());
}

template <typename T>
std::enable_if_t<IsUnsignedIntegerV<T>> SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, T& OutVal)
{
    if (Object.is_uinteger() == false)
    {
        throw std::runtime_error("Invalid call: Value was not a uinteger");
    }

    OutVal = static_cast<T>(Object.as_uinteger());
}

template <typename T> inline void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, std::optional<T>& OutVal)
{
    if (Object.is_null())
    {
        OutVal = std::nullopt;
    }
    else
    {
        OutVal = T {};
        ReadValueFromObject(Object, *OutVal);
    }
}

template <typename T> inline void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value&, std::vector<T>& OutVal)
{
    size_t ArraySize = 0;
    StartReadArray(ArraySize);

    OutVal.resize(ArraySize);

    for (size_t i = 0; i < ArraySize; ++i)
    {
        ReadValue(OutVal[i]);
    }

    EndReadArrayInternal();
}

template <typename K, typename T> inline void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value&, std::map<K, T>& OutVal)
{
    size_t MapSize = 0;
    StartReadUintMap(MapSize);

    for (size_t i = 0; i < MapSize; ++i)
    {
        std::pair<K, T> Pair;
        ReadKeyValue(Pair);
        OutVal[Pair.first] = Pair.second;
    }

    EndReadUintMapInternal();
}

template <typename T> inline void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value&, std::map<std::string, T>& OutVal)
{
    size_t MapSize = 0;
    StartReadStringMap(MapSize);

    for (size_t i = 0; i < MapSize; ++i)
    {
        std::pair<std::string, T> Pair;
        ReadKeyValue(Pair);
        OutVal[Pair.first] = Pair.second;
    }

    EndReadStringMapInternal();
}
}
