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

#include <signalrclient/signalr_value.h>

#include <map>
#include <memory>
#include <optional>
#include <stack>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace csp::multiplayer
{
class SignalRSerializableValue;
/// @brief A variant reresenting all possible basic types serializable to a signalr value.
struct SignalRSerializableValue : std::variant<int64_t, uint64_t, double, bool, std::string, nullptr_t, std::vector<SignalRSerializableValue>,
                                      std::map<uint64_t, SignalRSerializableValue>, std::map<std::string, SignalRSerializableValue>>
{
    using variant::variant;
};

/// @brief A stack-based signalr serializer which allows for custom class serialization using ISignalRSerializable
class SignalRSerializer
{
public:
    /// @brief Writes a value to the current container of the serializer.
    /// @pre This function should be used if this serializer represents a single value,
    /// or if StartArray is called first to write to the array
    /// or if this serializer represents a single value.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <class T> void WriteValue(const T& Value) { WriteValueInternal(Value); }

    /// @brief Writes a uint key-value pair to the current uint map.
    /// @pre StartUintMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <class T> void WriteKeyValue(uint64_t Key, const T& Value);

    /// @brief Writes a string key-value pair to the current map.
    /// @pre StartStringMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <class T> void WriteKeyValue(std::string Key, const T& Value);

    /// @brief Starts writing an array into the serializer.
    /// @details Once this function has been called, WriteValue should be used to add elements to the array.
    /// Favour writing the array directly to the serializer using the write functions over this.
    /// Start/End functions should be used if you need custom serialization logic.
    /// PopArray should be used to finalize the array.
    void StartWriteArray();

    /// @brief Ends the current array in the serializer.
    /// @pre StartWriteArray should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    void EndWriteArray();

    /// @brief Starts writing a string map (std::map<std::string, T> into the serializer.
    /// @details Once this function has been called, WriteKeyValue should be used to add elements to the map.
    /// Favour writing the map directly to the serializer using the write functions over this.
    /// Start/End functions should be used if you need custom serialization logic.
    /// PopStringMap should be used to finalize the map.
    void StartWriteStringMap();

    /// @brief Ends the current string map in the serializer.
    /// @pre StartWriteStringMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    void EndWriteStringMap();

    /// @brief Starts writing a uint map (std::map<uint64_t, T> into the serializer.
    /// @details Once this function has been called, WriteKeyValue should be used to add elements to the map.
    /// Favour writing the map directly to the serializer using the write functions over this.
    /// Start/End functions should be used if you need custom serialization logic.
    /// PopUintMap should be used to finalize the map.
    void StartWriteUintMap();

    /// @brief Ends the current uint map in the serializer.
    /// @pre StartWriteUintMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    void EndWriteUintMap();

    /// @brief Gets the serialized singnal r value.
    /// @return signalr::value
    /// @pre The serializer should be at the root (array and maps should all be popped).
    signalr::value Get() const;

private:
    // Adds the serialized container object to the previous object, or the root
    void FinalizeContainerSerializaiton(signalr::value&& SerializedContainer);

    template <class T> void FinalizeContainerSerializaitonInternal(T&, signalr::value&& SerializedContainer);
    void FinalizeContainerSerializaitonInternal(std::vector<signalr::value>& Container, signalr::value&& SerializedContainer);
    template <class K> void FinalizeContainerSerializaitonInternal(std::pair<K, signalr::value>& Pair, signalr::value&& SerializedContainer);

    template <class T> void WriteValueInternal(const T& Value);
    template <class T> void WriteValueInternal(const std::optional<T>& Value);
    void WriteValueInternal(const SignalRSerializableValue& Value);
    template <class T> void WriteValueInternal(const std::vector<T>& Value);
    template <class T> void WriteValueInternal(const std::map<uint64_t, T>& Value);
    template <class T> void WriteValueInternal(const std::map<std::string, T>& Value);

    template <class T> signalr::value GetInternal(const T& Object);

    signalr::value GetInternal(const signalr::value& Object) const;
    signalr::value GetInternal(const std::pair<uint64_t, signalr::value>& Object) const;
    signalr::value GetInternal(const std::pair<std::string, signalr::value>& Object) const;

    using Container = std::variant<signalr::value, std::vector<signalr::value>, std::map<std::string, signalr::value>,
        std::map<uint64_t, signalr::value>, std::pair<uint64_t, signalr::value>, std::pair<std::string, signalr::value>>;

    std::stack<Container> Stack;
};

/// @brief A stack-based signalr deserializer which allows for custom class deserialization using ISignalRDeserializable
class SignalRDeserializer
{
public:
    /// @brief Constructor used to copy object to deserialize.
    /// @param Object const signalr::value& : The value to deserialize
    /// This should match the structure generated by
    SignalRDeserializer(const signalr::value& Object);

    /// @brief Constructor used to move object to deserialize.
    /// @param Object signalr::value&& : The value to deserialize
    /// This should match the structure generated by
    SignalRDeserializer(signalr::value&& Object);

    /// @brief Reads a value from the internal signalr array.
    /// @pre EnterArray should be called before this function,
    /// or if this deserializer represents a single value.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <class T> void ReadValue(T& OutVal);

    /// @brief Reads a uint key-value pair to the current uint map.
    /// @pre EnterUintMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <class T> void ReadKeyValue(std::pair<uint64_t, T>& OutVal);

    /// @brief Reads a string key-value pair to the current string map.
    /// @pre EnterStringMap should be called before this function.
    /// A std::runtime_error will be thrown if this condition is not met.
    template <class T> void ReadKeyValue(std::pair<std::string, T>& OutVal);

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
    const signalr::value& ReadNextValue();

    const std::pair<std::uint64_t, signalr::value> ReadNextUintKeyValue() const;
    const std::pair<std::string, signalr::value> ReadNextStringKeyValue() const;

    template <class T> void ReadValueFromObject(const signalr::value& Object, T& OutVal);

    void ReadValueFromObjectInternal(const signalr::value& Object, int64_t& OutVal) const;
    void ReadValueFromObjectInternal(const signalr::value& Object, uint64_t& OutVal) const;
    void ReadValueFromObjectInternal(const signalr::value& Object, double& OutVal) const;
    void ReadValueFromObjectInternal(const signalr::value& Object, bool& OutVal) const;
    void ReadValueFromObjectInternal(const signalr::value& Object, std::string& OutVal) const;
    void ReadValueFromObjectInternal(const signalr::value& Object, nullptr_t&) const;
    void ReadValueFromObjectInternal(const signalr::value& Object, SignalRSerializableValue& OutVal);

    template <class T> void ReadValueFromObjectInternal(const signalr::value& Object, std::optional<T>& OutVal) const;
    template <class T> void ReadValueFromObjectInternal(const signalr::value& Object, std::vector<T>& OutVal);
    template <class T> void ReadValueFromObjectInternal(const signalr::value& Object, std::map<uint64_t, T>& OutVal);
    template <class T> void ReadValueFromObjectInternal(const signalr::value& Object, std::map<std::string, T>& OutVal);

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
template <class T> inline void SignalRSerializer::WriteKeyValue(uint64_t Key, const T& Value)
{
    if (Stack.size() == 0 || std ::holds_alternative<std::map<uint64_t, signalr::value>>(Stack.top()) == false)
    {
        throw std::runtime_error("Invalid call: Serializer was not in a uint map");
    }

    Stack.push(std::pair<uint64_t, signalr::value> {});
    std::get<std::pair<uint64_t, signalr::value>>(Stack.top()).first = Key;

    WriteValue(Value);

    // Get our pair form the top of the stack and pop
    std::pair<uint64_t, signalr::value> Pair = std::get<std::pair<uint64_t, signalr::value>>(Stack.top());
    Stack.pop();

    // Get our map from the top of the stack and append the pair
    std::map<uint64_t, signalr::value>& Map = std::get<std::map<uint64_t, signalr::value>>(Stack.top());
    Map[Pair.first] = Pair.second;
}
template <class T> inline void SignalRSerializer::WriteKeyValue(std::string Key, const T& Value)
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
template <class T> inline void SignalRSerializer::FinalizeContainerSerializaitonInternal(T&, signalr::value&& SerializedContainer)
{
    throw std::runtime_error("Unexpected serializer state");
}
template <class K>
inline void SignalRSerializer::FinalizeContainerSerializaitonInternal(std::pair<K, signalr::value>& Pair, signalr::value&& SerializedContainer)
{
    Pair.second = SerializedContainer;
}
template <class T> inline void SignalRSerializer::WriteValueInternal(const T& Value)
{
    signalr::value SerializedValue;

    if constexpr (std::is_base_of<ISignalRSerializable, T>::value)
    {
        Value.Serialize(*this);
    }
    else
    {
        SerializedValue = signalr::value(Value);

        if (Stack.size() == 0)
        {
            // Case where a user only want to serialize a single value
            Stack.push(signalr::value(Value));
        }
        else if (std::holds_alternative<std::vector<signalr::value>>(Stack.top()))
        {
            std::get<std::vector<signalr::value>>(Stack.top()).push_back(SerializedValue);
        }
        else if (std::holds_alternative<std::pair<uint64_t, signalr::value>>(Stack.top()))
        {
            std::get<std::pair<uint64_t, signalr::value>>(Stack.top()).second = SerializedValue;
        }
        else if (std::holds_alternative<std::pair<std::string, signalr::value>>(Stack.top()))
        {
            std::get<std::pair<std::string, signalr::value>>(Stack.top()).second = SerializedValue;
        }
        else
        {
            throw std::runtime_error("Invalid call: Serializer was not in an array or at the root");
        }
    }
}
template <class T> signalr::value SignalRSerializer::GetInternal(const T& Object) { return signalr::value(Object); }
template <class T> inline void SignalRSerializer::WriteValueInternal(const std::optional<T>& Value)
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
template <class T> inline void SignalRSerializer::WriteValueInternal(const std::vector<T>& Value)
{
    StartWriteArray();

    for (const auto& Element : Value)
    {
        WriteValue(Element);
    }

    EndWriteArray();
}

template <class T> inline void SignalRSerializer::WriteValueInternal(const std::map<uint64_t, T>& Value)
{
    StartWriteUintMap();

    for (const auto& Pair : Value)
    {
        WriteKeyValue(Pair.first, Pair.second);
    }

    EndWriteUintMap();
}

template <class T> inline void SignalRSerializer::WriteValueInternal(const std::map<std::string, T>& Value)
{
    StartWriteStringMap();

    for (const auto& Pair : Value)
    {
        WriteKeyValue(Pair.first, Pair.second);
    }

    EndWriteStringMap();
}

template <class T> inline void SignalRDeserializer::ReadValue(T& OutVal)
{
    const signalr::value& Next = ReadNextValue();
    ReadValueFromObject(Next, OutVal);

    IncrementIterator();
}

template <class T> void SignalRDeserializer::ReadKeyValue(std::pair<uint64_t, T>& OutVal)
{
    const std::pair<uint64_t, signalr::value>& Next = ReadNextUintKeyValue();

    OutVal.first = Next.first;
    ReadValueFromObject(Next.second, OutVal.second);

    IncrementIterator();
}

template <class T> void SignalRDeserializer::ReadKeyValue(std::pair<std::string, T>& OutVal)
{
    const std::pair<std::string, signalr::value>& Next = ReadNextStringKeyValue();

    OutVal.first = Next.first;
    ReadValueFromObject(Next.second, OutVal.second);

    IncrementIterator();
}

template <class T> void SignalRDeserializer::ReadValueFromObject(const signalr::value& Object, T& OutVal)
{
    if constexpr (std::is_base_of<ISignalRDeserializable, T>::value)
    {
        SignalRDeserializer Deserializer { Object };
        OutVal.Deserialize(Deserializer);
        return;
    }
    else
    {
        ReadValueFromObjectInternal(Object, OutVal);
    }
}
template <class T> inline void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, std::optional<T>& OutVal) const
{
    if (Object.is_null())
    {
        OutVal = std::nullopt;
    }
    else
    {
        OutVal = T {};
        ReadValueFromObjectInternal(Object, *OutVal);
    }
}
template <class T> inline void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, std::vector<T>& OutVal)
{
    size_t ArraySize = 0;
    StartReadArray(ArraySize);

    OutVal.resize(ArraySize);

    for (size_t i = 0; i < ArraySize; ++i)
    {
        ReadValue(OutVal[i]);
    }

    EndReadArray();
}
template <class T> inline void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, std::map<uint64_t, T>& OutVal)
{
    size_t MapSize = 0;
    StartReadUintMap(MapSize);

    for (size_t i = 0; i < MapSize; ++i)
    {
        std::pair<uint64_t, T> Pair;
        ReadKeyValue(Pair);
        OutVal[Pair.first] = Pair.second;
    }

    EndReadUintMap();
}
template <class T> inline void SignalRDeserializer::ReadValueFromObjectInternal(const signalr::value& Object, std::map<std::string, T>& OutVal)
{
    size_t MapSize = 0;
    StartReadStringMap(MapSize);

    for (size_t i = 0; i < MapSize; ++i)
    {
        std::pair<std::string, T> Pair;
        ReadKeyValue(Pair);
        OutVal[Pair.first] = Pair.second;
    }

    EndReadStringMap();
}
}
