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

#ifndef SKIP_INTERNAL_TESTS

#include "CSP/CSPFoundation.h"
#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"
#include "CSP/Multiplayer/Components/CustomSpaceComponent.h"
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Multiplayer/SignalRMsgPackEntitySerialiser.h"
#include "Multiplayer/SpaceEntityKeys.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::common;
using namespace csp::multiplayer;
using namespace csp::multiplayer::msgpack_typeids;

CSP_INTERNAL_TEST(CSPEngine, SerialisationTests, SpaceEntityUserSignalRSerialisationTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    SignalRMsgPackEntitySerialiser Serialiser;
    auto User = CSP_NEW SpaceEntity();
    User->Type = SpaceEntityType::Avatar;
    User->Id = 42;
    User->Name = "MyUser";
    User->Transform = { Vector3 { 1.2f, 2.34f, 3.45f }, Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, Vector3 { 1, 1, 1 } };
    User->OwnerId = 1337;
    User->IsTransferable = false;
    User->IsPersistant = false;
    User->ParentId = 9999;

    auto* AvatarComponent = (AvatarSpaceComponent*)User->AddComponent(ComponentType::AvatarData);

    AvatarComponent->SetAvatarId("MyCoolAvatar");
    AvatarComponent->SetState(AvatarState::Flying);
    AvatarComponent->SetUserId("0123456789ABCDEF");

    User->Serialise(Serialiser);
    auto SerialisedUser = Serialiser.Finalise();

    /*
     * SerialisedUser should look like this:
     *   [
     *     42,										-- Id
     *     1,										-- Type (PrefabId)
     *     false,									-- IsTransferable
     *     false,									-- IsPersistant
     *     1337,									-- OwnerId
     *     null,									-- ParentId
     *     {										-- Components
     *       1: [ 6, [ <raw> ] ],					---- Properties
     *		 2: [ 6, [ <raw> ] ],					---- AvatarSpaceComponent
     *       257: [ 26, [ [ 1f, 2f, 3f ] ] ],		---- Position
     *       258: [ 26, [ [ 4f, 5f, 6f, 7f ] ] ],	---- Rotation
     *       259: [ 26, [ [ 1f, 1f, 1f ] ] ],		---- Scale
     *		 260: 0									---- SelectedClientID
     *     }
     *   ]
     */

    EXPECT_TRUE(SerialisedUser.is_array() && SerialisedUser.as_array().size() == 7);

    auto& Array = SerialisedUser.as_array();

    EXPECT_TRUE(Array[0].is_uinteger() && Array[0].as_uinteger() == User->Id);
    EXPECT_TRUE(Array[1].is_uinteger() && Array[1].as_uinteger() == (int)SpaceEntityType::Avatar);
    EXPECT_TRUE(Array[2].is_bool() && !Array[2].as_bool()); // IsTransferable
    EXPECT_TRUE(Array[3].is_bool() && !Array[3].as_bool()); // IsPersistant
    EXPECT_TRUE(Array[4].is_uinteger() && Array[4].as_uinteger() == User->OwnerId);
    EXPECT_TRUE(Array[5].as_uinteger() && Array[5].as_uinteger() == *User->ParentId); // ParentId
    EXPECT_TRUE(Array[6].is_uint_map() && Array[6].as_uint_map().size() == 8); // Components

    auto& Components = Array[6].as_uint_map();

    for (auto& [Key, Component] : Components)
    {
        EXPECT_TRUE(Component.is_array() && Component.as_array().size() == 2);

        auto& ComponentArray = Component.as_array();

        EXPECT_TRUE(ComponentArray[0].is_uinteger());
        EXPECT_TRUE(ComponentArray[1].is_array() && ComponentArray[1].as_array().size() == 1);
        auto& ComponentValue = ComponentArray[1].as_array()[0];

        switch (ComponentArray[0].as_uinteger())
        {
        case ItemComponentData::UINT8_ARRAY:
        {
            EXPECT_TRUE(ComponentValue.is_raw());

            break;
        }
        case ItemComponentData::FLOAT_ARRAY:
        {
            EXPECT_TRUE(ComponentValue.is_array() && ComponentValue.as_array()[0].is_double());

            auto& Array = ComponentValue.as_array();

            if (Key == COMPONENT_KEY_VIEW_POSITION)
            {

                EXPECT_TRUE(Array.size() == 3 && Array[0].as_double() == User->Transform.Position.X
                    && Array[1].as_double() == User->Transform.Position.Y && Array[2].as_double() == User->Transform.Position.Z);
            }
            else if (Key == COMPONENT_KEY_VIEW_ROTATION)
            {
                EXPECT_TRUE(Array.size() == 4 && Array[0].as_double() == User->Transform.Rotation.X
                    && Array[1].as_double() == User->Transform.Rotation.Y && Array[2].as_double() == User->Transform.Rotation.Z
                    && Array[3].as_double() == User->Transform.Rotation.W);
            }
            else if (Key == COMPONENT_KEY_VIEW_SCALE)
            {
                EXPECT_TRUE(Array.size() == 3 && Array[0].as_double() == User->Transform.Scale.X && Array[1].as_double() == User->Transform.Scale.Y
                    && Array[2].as_double() == User->Transform.Scale.Z);
            }
            else
            {
                FAIL();
            }

            break;
        }
        case ItemComponentData::STRING:
        {
            EXPECT_TRUE(ComponentValue.is_string());

            break;
        }
        case ItemComponentData::INT64:
        {
            EXPECT_TRUE(ComponentValue.is_integer());
            break;
        }
        case ItemComponentData::UINT16_DICTIONARY:
        {
            EXPECT_TRUE(ComponentValue.is_uint_map());
            break;
        }
        default:
            FAIL();
        }
    }

    CSP_DELETE(User);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, SerialisationTests, SpaceEntityUserSignalRDeserialisationTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    SignalRMsgPackEntitySerialiser Serialiser;
    auto User = CSP_NEW SpaceEntity();
    User->Id = 42;
    User->Name = "MyUser";
    User->Transform = { Vector3 { 1.2f, 2.34f, 3.45f }, Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, Vector3 { 1, 1, 1 } };
    User->OwnerId = 1337;
    User->IsTransferable = false;
    User->IsPersistant = false;
    User->ParentId = 9999;

    auto* AvatarComponent = (AvatarSpaceComponent*)User->AddComponent(ComponentType::AvatarData);

    AvatarComponent->SetAvatarId("MyCoolAvatar");
    AvatarComponent->SetState(AvatarState::Flying);
    AvatarComponent->SetUserId("0123456789ABCDEF");

    User->Serialise(Serialiser);
    auto SerialisedUser = Serialiser.Finalise();

    SignalRMsgPackEntityDeserialiser Deserialiser(SerialisedUser);
    auto DeserialisedUser = CSP_NEW SpaceEntity();
    DeserialisedUser->Deserialise(Deserialiser);

    EXPECT_EQ(DeserialisedUser->Id, User->Id);
    EXPECT_EQ(DeserialisedUser->Name, User->Name);
    EXPECT_EQ(DeserialisedUser->Transform.Position, User->Transform.Position);
    EXPECT_EQ(DeserialisedUser->Transform.Rotation, User->Transform.Rotation);
    EXPECT_EQ(DeserialisedUser->OwnerId, User->OwnerId);
    EXPECT_EQ(*DeserialisedUser->ParentId, *User->ParentId);

    EXPECT_EQ(DeserialisedUser->Components.Size(), 1);

    auto* DeserialisedComponent = DeserialisedUser->GetComponent(COMPONENT_KEY_START_COMPONENTS);

    EXPECT_EQ(DeserialisedComponent->Type, ComponentType::AvatarData);

    auto* DeserialisedAvatarComponent = (AvatarSpaceComponent*)DeserialisedComponent;

    EXPECT_EQ(DeserialisedAvatarComponent->GetAvatarId(), AvatarComponent->GetAvatarId());
    EXPECT_EQ(DeserialisedAvatarComponent->GetState(), AvatarComponent->GetState());
    EXPECT_EQ(DeserialisedAvatarComponent->GetUserId(), AvatarComponent->GetUserId());

    CSP_DELETE(DeserialisedUser);
    CSP_DELETE(User);
}

CSP_INTERNAL_TEST(CSPEngine, SerialisationTests, SpaceEntityObjectSignalRSerialisationTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    SignalRMsgPackEntitySerialiser Serialiser;
    auto Object = CSP_NEW SpaceEntity();
    Object->Type = SpaceEntityType::Object;
    Object->Id = 1337;
    Object->IsTransferable = true;
    Object->IsPersistant = true;
    Object->Name = "MyObject";
    Object->Transform = { Vector3 { 1.2f, 2.34f, 3.45f }, Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, Vector3 { 1, 1, 1 } };
    Object->OwnerId = 42;
    Object->ParentId = 9999;

    auto Type = (ComponentType)((int)ComponentType::Custom + 1);
    Object->Components[COMPONENT_KEY_START_COMPONENTS] = CSP_NEW ComponentBase(Type, Object);
    auto* NewComponent = Object->Components[COMPONENT_KEY_START_COMPONENTS];
    NewComponent->Id = COMPONENT_KEY_START_COMPONENTS;
    NewComponent->Properties[0xDEAD] = true;
    NewComponent->Properties[0xC0DE] = 0x600DC0DEBADD00D5;
    NewComponent->Properties[0xBEEF] = "yummy yummy beef";

    Object->Serialise(Serialiser);
    auto SerialisedObject = Serialiser.Finalise();

    /*
     * SerialisedObject should look like this:
     *   [
     *     42,										-- Id
     *     2,										-- Type (PrefabId)
     *     false,									-- IsTransferable
     *     true,									-- IsPersistant
     *     1337,									-- OwnerId
     *     9999,									-- ParentId
     *     {										-- Components
     *       1: [ 6, [ <raw> ] ],					---- Properties
     *       2: [ 6, [ <raw> ] ],					---- Custom component
     *       257: [ 26, [ [ 1f, 2f, 3f ] ] ],		---- Position
     *       258: [ 26, [ [ 4f, 5f, 6f, 7f ] ] ],	---- Rotation
     *       259: [ 26, [ [ 1f, 1f, 1f ] ] ],		---- Scale
     *     }
     *   ]
     */

    EXPECT_TRUE(SerialisedObject.is_array() && SerialisedObject.as_array().size() == 7);

    auto& Array = SerialisedObject.as_array();

    EXPECT_TRUE(Array[0].is_uinteger() && Array[0].as_uinteger() == Object->Id);
    EXPECT_TRUE(Array[1].is_uinteger() && Array[1].as_uinteger() == 2); // PrefabId
    EXPECT_TRUE(Array[2].is_bool() && Array[2].as_bool()); // IsTransferable
    EXPECT_TRUE(Array[3].is_bool()); // IsPersistant
    EXPECT_TRUE(Array[4].is_uinteger() && Array[4].as_uinteger() == Object->OwnerId);
    EXPECT_TRUE(Array[5].is_uinteger() && Array[5].as_uinteger() == *Object->ParentId); // ParentId
    EXPECT_TRUE(Array[6].is_uint_map() && Array[6].as_uint_map().size() >= 4); // Components

    auto& Components = Array[6].as_uint_map();

    for (auto& [Key, Component] : Components)
    {
        EXPECT_TRUE(Component.is_array() && Component.as_array().size() == 2);

        auto& ComponentArray = Component.as_array();

        EXPECT_TRUE(ComponentArray[0].is_uinteger());
        EXPECT_TRUE(ComponentArray[1].is_array() && ComponentArray[1].as_array().size() == 1);
        auto& ComponentValue = ComponentArray[1].as_array()[0];

        switch (ComponentArray[0].as_uinteger())
        {
        case ItemComponentData::UINT8_ARRAY:
        {
            EXPECT_TRUE(ComponentValue.is_raw());

            break;
        }
        case ItemComponentData::FLOAT_ARRAY:
        {
            EXPECT_TRUE(ComponentValue.is_array() && ComponentValue.as_array()[0].is_double());

            auto& Array = ComponentValue.as_array();

            if (Key == COMPONENT_KEY_VIEW_POSITION)
            {

                EXPECT_TRUE(Array.size() == 3 && Array[0].as_double() == Object->Transform.Position.X
                    && Array[1].as_double() == Object->Transform.Position.Y && Array[2].as_double() == Object->Transform.Position.Z);
            }
            else if (Key == COMPONENT_KEY_VIEW_ROTATION)
            {
                EXPECT_TRUE(Array.size() == 4 && Array[0].as_double() == Object->Transform.Rotation.X
                    && Array[1].as_double() == Object->Transform.Rotation.Y && Array[2].as_double() == Object->Transform.Rotation.Z
                    && Array[3].as_double() == Object->Transform.Rotation.W);
            }
            else if (Key == COMPONENT_KEY_VIEW_SCALE)
            {
                EXPECT_TRUE(Array.size() == 3 && Array[0].as_double() == Object->Transform.Scale.X
                    && Array[1].as_double() == Object->Transform.Scale.Y && Array[2].as_double() == Object->Transform.Scale.Z);
            }
            else
            {
                FAIL();
            }

            break;
        }
        case ItemComponentData::STRING:
        {
            EXPECT_TRUE(ComponentValue.is_string());

            break;
        }
        case ItemComponentData::INT64:
        {
            EXPECT_TRUE(ComponentValue.is_integer());
            break;
        }
        default:
            FAIL();
        }
    }

    CSP_DELETE(Object);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, SerialisationTests, SpaceEntityObjectSignalRDeserialisationTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    // Current default properties:
    // - ComponentName
    const int DefaultComponentProps = 1;

    SignalRMsgPackEntitySerialiser Serialiser;
    auto Object = CSP_NEW SpaceEntity();
    Object->Type = SpaceEntityType::Object;
    Object->Id = 1337;
    Object->IsTransferable = true;
    Object->IsPersistant = true;
    Object->Name = "MyObject";
    Object->Transform = { Vector3 { 1.2f, 2.34f, 3.45f }, Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, Vector3 { 1, 1, 1 } };
    Object->OwnerId = 42;
    Object->ParentId = 9999;

    auto* NewComponent = (StaticModelSpaceComponent*)Object->AddComponent(ComponentType::StaticModel);
    NewComponent->SetExternalResourceAssetCollectionId("blah");
    NewComponent->SetIsVisible(true);

    Object->Serialise(Serialiser);
    auto SerialisedObject = Serialiser.Finalise();

    SignalRMsgPackEntityDeserialiser Deserialiser(SerialisedObject);
    auto DeserialisedObject = CSP_NEW SpaceEntity();
    DeserialisedObject->Deserialise(Deserialiser);

    EXPECT_EQ(DeserialisedObject->Id, Object->Id);
    EXPECT_EQ(DeserialisedObject->IsPersistant, Object->IsPersistant);
    EXPECT_EQ(DeserialisedObject->Name, Object->Name);
    EXPECT_EQ(DeserialisedObject->Transform.Position, Object->Transform.Position);
    EXPECT_EQ(DeserialisedObject->Transform.Rotation, Object->Transform.Rotation);
    EXPECT_EQ(DeserialisedObject->Transform.Scale, Object->Transform.Scale);
    EXPECT_EQ(DeserialisedObject->OwnerId, Object->OwnerId);
    EXPECT_EQ(*DeserialisedObject->ParentId, *Object->ParentId);

    EXPECT_EQ(DeserialisedObject->Components.Size(), 1);

    auto* DeserialisedComponent = (StaticModelSpaceComponent*)DeserialisedObject->GetComponent(COMPONENT_KEY_START_COMPONENTS);

    EXPECT_EQ(DeserialisedComponent->GetComponentType(), ComponentType::StaticModel);

    // -1 as Name_DEPRECATED was never in use
    EXPECT_EQ(DeserialisedComponent->Properties.Size(), ((size_t)StaticModelPropertyKeys::Num) - 1 + DefaultComponentProps);
    EXPECT_EQ(DeserialisedComponent->GetExternalResourceAssetCollectionId(), "blah");
    EXPECT_EQ(DeserialisedComponent->GetIsVisible(), true);

    CSP_DELETE(DeserialisedObject);
    CSP_DELETE(Object);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, SerialisationTests, MapDeserialisationTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    SpaceEntity* MySpaceEntity = new SpaceEntity();
    MySpaceEntity->Type = SpaceEntityType::Object;
    MySpaceEntity->Id = 1337;

    CustomSpaceComponent MyCustomComponent(MySpaceEntity);

    int64_t Prop1 = 10ll;
    MyCustomComponent.SetCustomProperty("Prop1", Prop1);

    // Create replicated maps
    csp::common::Map<csp::common::String, ReplicatedValue> Map1;
    Map1["Key1"] = "Test1";
    Map1["Key2"] = "Test2";
    Map1["Key3"] = "Test3";

    // Store map in a custom property
    MyCustomComponent.SetCustomProperty("MyMap1", Map1);

    csp::common::String Prop2 = "Test";
    MyCustomComponent.SetCustomProperty("Prop2", Prop2);

    // Serialize
    SignalRMsgPackEntitySerialiser Serialiser;
    MySpaceEntity->Serialise(Serialiser);
    auto SerialisedObject = Serialiser.Finalise();

    // Deserialize
    SignalRMsgPackEntityDeserialiser Deserialiser(SerialisedObject);
    auto DeserialisedObject = CSP_NEW SpaceEntity();
    DeserialisedObject->Deserialise(Deserialiser);

    auto* DeserializedComponent = static_cast<CustomSpaceComponent*>(DeserialisedObject->GetComponent(0));

    // Ensure deserialized values are correct
    EXPECT_EQ(DeserializedComponent->GetCustomProperty("Prop1"), Prop1);
    EXPECT_EQ(DeserializedComponent->GetCustomProperty("MyMap1"), Map1);
    EXPECT_EQ(DeserializedComponent->GetCustomProperty("Prop2"), Prop2);

    delete MySpaceEntity;

    csp::CSPFoundation::Shutdown();
}

#endif