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

#include "CSP/Multiplayer/IEntitySerialiser.h"
#include "Memory/Memory.h"
#include "MultiplayerConstants.h"

#include <msgpack/pack.hpp>
#include <msgpack/sbuffer.hpp>
#include <msgpack/unpack.hpp>
#include <signalrclient/signalr_value.h>

namespace csp::multiplayer
{

enum class SerialiserState
{
    Initial,
    InEntity,
    InComponents,
    InComponent,
    InArray,
};

/// <summary>
/// The serialiser is responsible for converting a SpaceEntity instance into a data structure that both SignalR and our
/// Servers can understand and use.
///     To use the serialiser, Start with BeginEntity(), then use the write(bool, int, etc) functions to write data at the
/// Entity level. Use BeginComponents() to start writing as the Server component data, with each BeginComponent and
/// EndComponent brace dictating information written into a server component.
///     Within a component, use WriteProperty() to write the individual data.
///     Ensure you finish by closing out the relevant sections with the 'End' functions,
/// then use Finalise() to give back a signalR value that represents the data structure you created.
/// </summary>
class SignalRMsgPackEntitySerialiser : public IEntitySerialiser
{
public:
    SignalRMsgPackEntitySerialiser();

    void BeginEntity() override;
    void EndEntity() override;
    void WriteBool(bool Value) override;
    void WriteByte(uint8_t Value) override;
    void WriteDouble(double Value) override;
    void WriteInt64(int64_t Value) override;
    void WriteUInt64(uint64_t Value) override;
    void WriteString(const csp::common::String& Value) override;
    void WriteNull() override;
    void BeginComponents() override;
    void EndComponents() override;
    void BeginComponent(uint16_t Id, uint64_t Type) override;
    void EndComponent() override;
    void BeginArray() override;
    void EndArray() override;
    void WriteProperty(uint64_t Id, const ReplicatedValue& Value) override;
    void AddViewComponent(uint16_t Id, const ReplicatedValue& Value) override;

    signalr::value Finalise();

private:
    SerialiserState CurrentState;
    std::vector<signalr::value> Fields;
    std::map<uint64_t, signalr::value> Components;
    std::vector<signalr::value> CurrentArray;
    uint64_t CurrentComponentId;

    std::map<uint16_t, std::pair<msgpack_typeids::ItemComponentData, signalr::value>> Properties;
};

/// <summary>
/// The deserialiser is used to take recieved signalr message data and turn it into values you can use to populate a SpaceEntity.
///     Note that while it works very similarly to the serialiser, and you should refer to the serialiser class
/// comment for more details, it does not have a Finalise() method at the end. Instead, it's expected that you will be
/// using the data as you read it to populate either a newly created or currently existing SpaceEntity.
/// </summary>
class SignalRMsgPackEntityDeserialiser : public IEntityDeserialiser
{
public:
    SignalRMsgPackEntityDeserialiser(const signalr::value& Object);

    void EnterEntity() override;
    void LeaveEntity() override;
    bool ReadBool() override;
    uint8_t ReadByte() override;
    double ReadDouble() override;
    int64_t ReadInt64() override;
    uint64_t ReadUInt64() override;
    csp::common::String ReadString() override;
    csp::common::Vector2 ReadVector2() override;
    csp::common::Vector3 ReadVector3() override;
    csp::common::Vector4 ReadVector4() override;
    bool NextValueIsNull() override;
    bool NextValueIsArray() override;
    void EnterComponents() override;
    void LeaveComponents() override;
    void EnterArray(CSP_OUT uint32_t& OutLength) override;
    void LeaveArray() override;
    /** Returns total number of components, including view components. If iterating components by this count, subtract number of view components. */
    uint64_t GetNumComponents() override;
    /** Returns number of components that are not view components. */
    uint64_t GetNumRealComponents() override;
    /** Ignores view components. */
    void EnterComponent(CSP_OUT uint16_t& OutId, CSP_OUT uint64_t& OutType) override;
    void LeaveComponent() override;
    uint64_t GetNumProperties() override;
    ReplicatedValue ReadProperty(CSP_OUT uint64_t& OutId) override;
    ReplicatedValue GetViewComponent(uint16_t Id) override;
    bool HasViewComponent(uint16_t Id) override;
    void Skip() override;

private:
    const signalr::value* Object;
    SerialiserState CurrentState;
    const std::vector<signalr::value>* Fields;
    std::vector<signalr::value>::const_iterator CurrentFieldIterator;
    const std::vector<signalr::value>* CurrentArray;
    std::vector<signalr::value>::const_iterator CurrentArrayIterator;
    const std::map<uint64_t, signalr::value>* Components;
    std::map<uint64_t, signalr::value>::const_iterator CurrentComponentIterator;
    size_t ComponentPropertyCount;

// These are variables used in the legacy serialisation path used to deserialise any existing MsgPacked components
#pragma region MsgPackVariables
    msgpack::unpacker ComponentUnpacker;
    msgpack::object_handle ComponentObjectHandle;
    msgpack::unpacker PropertyUnpacker;
    msgpack::object_handle PropertyObjectHandle;
#pragma endregion

    bool IsMsgPackSerialiser = false;
    std::map<uint64_t, std::pair<msgpack_typeids::ItemComponentData, signalr::value>> Properties;
    std::map<uint64_t, std::pair<msgpack_typeids::ItemComponentData, signalr::value>>::const_iterator CurrentPropertyIterator;
};

} // namespace csp::multiplayer
