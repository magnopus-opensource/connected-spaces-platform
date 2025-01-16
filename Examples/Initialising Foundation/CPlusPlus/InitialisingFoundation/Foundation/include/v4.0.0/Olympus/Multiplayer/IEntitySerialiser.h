#pragma once

#include "Olympus/Common/String.h"
#include "Olympus/Common/Vector.h"
#include "Olympus/Multiplayer/ReplicatedValue.h"
#include "Olympus/OlympusCommon.h"

namespace oly_multiplayer
{

OLY_INTERFACE class OLY_API IEntitySerialiser
{
public:
    virtual void BeginEntity() = 0;
    virtual void EndEntity() = 0;
    virtual void WriteBool(bool Value) = 0;
    virtual void WriteByte(uint8_t Value) = 0;
    virtual void WriteDouble(double Value) = 0;
    virtual void WriteInt64(int64_t Value) = 0;
    virtual void WriteUInt64(uint64_t Value) = 0;
    virtual void WriteString(const oly_common::String& Value) = 0;
    virtual void WriteVector3(const oly_common::Vector3& Value) = 0;
    virtual void WriteVector4(const oly_common::Vector4& Value) = 0;
    virtual void WriteNull() = 0;
    virtual void BeginArray() = 0;
    virtual void EndArray() = 0;
    virtual void BeginComponents() = 0;
    virtual void EndComponents() = 0;
    virtual void BeginComponent(uint16_t Id, uint64_t Type) = 0;
    virtual void EndComponent() = 0;
    virtual void WriteProperty(uint64_t Id, const ReplicatedValue& Value) = 0;
    virtual void AddViewComponent(uint16_t Id, const ReplicatedValue& Value) = 0;

protected:
    virtual ~IEntitySerialiser() = default;
};

OLY_INTERFACE class OLY_API IEntityDeserialiser
{
public:
    virtual void EnterEntity() = 0;
    virtual void LeaveEntity() = 0;
    virtual bool ReadBool() = 0;
    virtual uint8_t ReadByte() = 0;
    virtual double ReadDouble() = 0;
    virtual int64_t ReadInt64() = 0;
    virtual uint64_t ReadUInt64() = 0;
    virtual oly_common::String ReadString() = 0;
    virtual oly_common::Vector3 ReadVector3() = 0;
    virtual oly_common::Vector4 ReadVector4() = 0;
    virtual bool NextValueIsNull() = 0;
    virtual void EnterArray(OLY_OUT uint32_t& OutLength) = 0;
    virtual void LeaveArray() = 0;
    virtual void EnterComponents() = 0;
    virtual void LeaveComponents() = 0;
    /** Returns total number of components, including view components. If iterating components by this count, subtract number of view components. */
    virtual uint64_t GetNumComponents() = 0;
    /** Returns number of components that are not view components. */
    virtual uint64_t GetNumRealComponents() = 0;
    /** Ignores view components. */
    virtual void EnterComponent(OLY_OUT uint16_t& OutId, OLY_OUT uint64_t& OutType) = 0;
    virtual void LeaveComponent() = 0;
    virtual uint64_t GetNumProperties() = 0;
    virtual ReplicatedValue ReadProperty(OLY_OUT uint64_t& OutId) = 0;
    virtual ReplicatedValue GetViewComponent(uint16_t Id) = 0;
    virtual bool HasViewComponent(uint16_t Id) = 0;
    virtual void Skip() = 0;

protected:
    virtual ~IEntityDeserialiser() = default;
};

} // namespace oly_multiplayer
