/*
 * Copyright 2023 Magnopus LLC

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

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Multiplayer/ReplicatedValue.h"

namespace csp::multiplayer
{

/// @brief The serialiser is responsible for converting a SpaceEntity instance into a data structure that both SignalR
/// and our servers can understand and use.
///
/// To use the serialiser, Start with BeginEntity(), then use the write(bool, int, etc) functions to write data at the
/// Entity level. Use BeginComponents() to start writing as the Server component data, with each BeginComponent and
/// EndComponent brace dictating information written into a server component. Within a component, use WriteProperty()
/// to write the individual data. Ensure you finish by closing out the relevant sections with the 'End' functions.
CSP_INTERFACE class CSP_API IEntitySerialiser
{
public:
    /// @brief Start the serialisiation.
    virtual void BeginEntity() = 0;

    /// @brief End the serialisiation.
    virtual void EndEntity() = 0;

    /// @brief Write a boolean field of the entity.
    /// @param Value bool : The value to write.
    virtual void WriteBool(bool Value) = 0;

    /// @brief Write a byte field of the entity.
    /// @param Value uint8_t : The value to write.
    virtual void WriteByte(uint8_t Value) = 0;

    /// @brief Write a double field of the entity.
    /// @param Value double : The value to write.
    virtual void WriteDouble(double Value) = 0;

    /// @brief Write an int64_t field of the entity.
    /// @param Value int64_t : The value to write.
    virtual void WriteInt64(int64_t Value) = 0;

    /// @brief Write a uint64_t field of the entity.
    /// @param Value uint64_t : The value to write.
    virtual void WriteUInt64(uint64_t Value) = 0;

    /// @brief Write a string field of the entity.
    /// @param Value csp::common::String : The value to write.
    virtual void WriteString(const csp::common::String& Value) = 0;

    /// @brief Write a vector2 field of the entity.
    /// @param Value Vector2 : The value to write.
    virtual void WriteVector2(const csp::common::Vector2& Value) = 0;

    /// @brief Write a vector3 field of the entity.
    /// @param Value Vector3 : The value to write.
    virtual void WriteVector3(const csp::common::Vector3& Value) = 0;

    /// @brief Write a vector4 field of the entity.
    /// @param Value Vector4 : The value to write.
    virtual void WriteVector4(const csp::common::Vector4& Value) = 0;

    /// @brief Write a null field of the entity.
    virtual void WriteNull() = 0;

    /// @brief Start an array section.
    virtual void BeginArray() = 0;

    /// @brief Finish an array section.
    virtual void EndArray() = 0;

    /// @brief Start the components section.
    virtual void BeginComponents() = 0;

    /// @brief Finish the components section
    virtual void EndComponents() = 0;

    /// @brief Begin writing component with the given ID and type.
    /// @param Id uint16_t : The ID of the component.
    virtual void BeginComponent(uint16_t Id, uint64_t Type) = 0;

    /// @brief Finish a component section.
    virtual void EndComponent() = 0;

    /// @brief Write the given component property.
    /// @param Id uint64_t : ID of the component property.
    /// @param Value ReplicatedValue : The value to be written.
    virtual void WriteProperty(uint64_t Id, const ReplicatedValue& Value) = 0;

    /// @brief Specific handler for writing view components.
    ///
    /// View Components are data that is stored in specific keys on the server, it allows us to discretely update these
    /// singular data pieces, rather than replicating larger chunks of data, and also allows us to always know where
    /// in a data structure this data will be.
    ///
    /// @param Id uint16_t : The ID of the component
    /// @param Value ReplicatedValue : The value of the component data to add
    virtual void AddViewComponent(uint16_t Id, const ReplicatedValue& Value) = 0;

protected:
    virtual ~IEntitySerialiser() = default;
};

/// @brief The deserialiser is used to take recieved signalr message data and turn it into values you can use to populate a SpaceEntity.
///
/// This works similarly to the serialiser and you can refer to the serialiser for more details.
/// It is expected that you will be using the data as you read it to populate either a newly created or currently existing SpaceEntity.
CSP_INTERFACE class CSP_API IEntityDeserialiser
{
public:
    /// @brief Starts the deserialisation.
    virtual void EnterEntity() = 0;

    /// @brief Ends the deserialisation.
    virtual void LeaveEntity() = 0;

    /// @brief Reads a boolean from the deserialiser.
    /// @return The deserialised boolean.
    virtual bool ReadBool() = 0;

    /// @brief Reads a byte from the deserialiser.
    /// @return The deserialised byte.
    virtual uint8_t ReadByte() = 0;

    /// @brief Reads a double from the deserialiser.
    /// @return The deserialised double.
    virtual double ReadDouble() = 0;

    /// @brief Reads an int64_t from the deserialiser.
    /// @return The deserialised boolean.
    virtual int64_t ReadInt64() = 0;

    /// @brief Reads a uint64_t from the deserialiser.
    /// @return The deserialised uint64_t.
    virtual uint64_t ReadUInt64() = 0;

    /// @brief Reads a string from the deserialiser.
    /// @return The deserialised string.
    virtual csp::common::String ReadString() = 0;

    /// @brief Reads a vector2 from the deserialiser.
    /// @return The deserialised vector2.
    virtual csp::common::Vector2 ReadVector2() = 0;

    /// @brief Reads a vector3 from the deserialiser.
    /// @return The deserialised vector3.
    virtual csp::common::Vector3 ReadVector3() = 0;

    /// @brief Reads a vector4 from the deserialiser.
    /// @return The deserialised vector4.
    virtual csp::common::Vector4 ReadVector4() = 0;

    /// @brief Checks if the next value is null.
    /// @return True if the next value is null, false otherwise.
    virtual bool NextValueIsNull() = 0;

    /// @brief Checks if the next value is an array.
    /// @return True if the next value is null, false otherwise.
    virtual bool NextValueIsArray() = 0;

    /// @brief Puts the deserialiser into array processing mode to begin reading from an array.
    /// @param OutLength uint32_t : A reference to variable to store the length of the array.
    virtual void EnterArray(CSP_OUT uint32_t& OutLength) = 0;

    /// @brief Completes reading from and array and leaves the array processing mode.
    virtual void LeaveArray() = 0;

    /// @brief Puts the deserialiser into component processing mode to begin reading from the components section of the serialised entity.
    virtual void EnterComponents() = 0;

    /// @brief Completes reading the components and exits that mode.
    virtual void LeaveComponents() = 0;

    /// @brief Gets the total number of components, including view components.
    ///
    /// If iterating components by this count, subtract number of view components.
    ///
    /// @return The number of components.
    virtual uint64_t GetNumComponents() = 0;

    /// @brief Gets the number of components that are not view components.
    /// @return The number of non-view components.
    virtual uint64_t GetNumRealComponents() = 0;

    /// @brief Begins the processes of deserialising a single component that is not a view component.
    virtual void EnterComponent(CSP_OUT uint16_t& OutId, CSP_OUT uint64_t& OutType) = 0;

    /// @brief Completes the deserialisation of a single component.
    virtual void LeaveComponent() = 0;

    /// @brief Gets the number of properties in the component that is currently being deserialised.
    /// @return The number of properties in the component.
    virtual uint64_t GetNumProperties() = 0;

    /// @brief Reads a property from the deserialiser, returning it as a ReplicatedValue.
    /// @param OutId uint64_t : A reference to a variable to store the ID of the property.
    /// @return The property value.
    virtual ReplicatedValue ReadProperty(CSP_OUT uint64_t& OutId) = 0;

    /// @brief Reads a view component from the deserialiser, returning it as a ReplicatedValue.
    ///
    /// Since view components are handled differently in the serialiser, they are similarly deserialised in their own way.
    ///
    /// @return The view component value.
    virtual ReplicatedValue GetViewComponent(uint16_t Id) = 0;

    /// @brief Gets whether there is a view component with the given ID.
    /// @param Id uint64_t : The ID of the component.
    /// @return True if there is a view component, false otherwise.
    virtual bool HasViewComponent(uint16_t Id) = 0;

    /// @brief Skips a field when deserialising the SpaceEntity fields.
    virtual void Skip() = 0;

protected:
    virtual ~IEntityDeserialiser() = default;
};

} // namespace csp::multiplayer
