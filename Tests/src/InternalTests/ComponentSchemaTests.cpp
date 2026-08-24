/*
 * Copyright 2026 Magnopus LLC

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

#include "TestHelpers.h"

#include "CSP/CSPFoundation.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/Components/AudioSpaceComponent.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "Multiplayer/ComponentSchema.h"
#include "Multiplayer/ComponentSchemaRegistry.h"
#include "Multiplayer/MCS/MCSTypes.h"
#include "Multiplayer/SpaceEntityKeys.h"

#include <limits>
#include <memory>

namespace
{

using Schema = csp::multiplayer::ComponentSchema;

template <typename T> using PlainValue = csp::multiplayer::PlainValue<T>;
template <typename T> using BoundedValue = csp::multiplayer::BoundedValue<T>;
template <typename T> using EnumeratedValue = csp::multiplayer::EnumeratedValue<T>;
template <typename T> using SchemaOption = csp::multiplayer::SchemaOption<T>;

class TestFixture final
{
public:
    TestFixture(const csp::common::Array<Schema>& Schemas)
        : ScriptSystem(csp::systems::ScriptSystem::MakeInitialised())
        , Engine(LogSystem, *ScriptSystem, Schemas)
    {
    }

    csp::multiplayer::SpaceEntity* MakeEntity(const csp::common::String& Name)
    {
        return std::get<0>(AWAIT(&Engine, CreateEntity, Name, csp::multiplayer::SpaceTransform {}, csp::common::Optional<uint64_t> {}));
    }

private:
    csp::common::LogSystem LogSystem;
    std::shared_ptr<csp::systems::ScriptSystem> ScriptSystem;
    csp::multiplayer::OfflineRealtimeEngine Engine;
};

} // namespace

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, GetTypeIdMatchesComponentTypeForHardcodedComponent)
{
    auto Fixture = TestFixture({});

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto* Component = Entity->AddComponent(csp::multiplayer::ComponentType::Audio);
    ASSERT_NE(Component, nullptr);

    EXPECT_EQ(Component->GetTypeId(), static_cast<uint64_t>(csp::multiplayer::ComponentType::Audio));
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, GetTypeIdReturnsRegisteredTypeIdForSchemaDrivenComponent)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    EXPECT_EQ(Component->GetTypeId(), uint64_t { 123 });
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, GetTypeIdPreservesFullUint64)
{
    constexpr uint64_t LargeTypeId = std::numeric_limits<uint64_t>::max();

    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { LargeTypeId },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto* Component = Entity->AddComponentByTypeId(LargeTypeId);
    ASSERT_NE(Component, nullptr);

    EXPECT_EQ(Component->GetTypeId(), LargeTypeId);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, GetComponentTypeReturnsInvalidForTypeIdAboveLegacyRange)
{
    using Underlying = std::underlying_type_t<csp::multiplayer::ComponentType>;
    constexpr auto WrapOffset = uint64_t { std::numeric_limits<Underlying>::max() } + 1;
    constexpr auto WrappedTypeId = WrapOffset + static_cast<uint64_t>(csp::multiplayer::ComponentType::Audio);
    static_assert(static_cast<csp::multiplayer::ComponentType>(WrappedTypeId) == csp::multiplayer::ComponentType::Audio);

    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { WrappedTypeId },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto* Component = Entity->AddComponentByTypeId(WrappedTypeId);
    ASSERT_NE(Component, nullptr);

    EXPECT_EQ(Component->GetComponentType(), csp::multiplayer::ComponentType::Invalid);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, AddComponentByTypeIdCreatesComponent)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });

    EXPECT_NE(Component, nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, AddComponentWithUnregisteredTypeIdReturnsNullptr)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto* Component = Entity->AddComponentByTypeId(uint64_t { 999 });

    EXPECT_EQ(Component, nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, AddComponentByTypeIdReturnsNullptrWhenEntityIsLocked)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    Entity->Lock();

    const auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });

    EXPECT_EQ(Component, nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, AddComponentByTypeIdWithHardcodedTypeIdCreatesConcreteType)
{
    auto Fixture = TestFixture({});

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto* Component = Entity->AddComponentByTypeId(static_cast<uint64_t>(csp::multiplayer::ComponentType::Audio));
    ASSERT_NE(Component, nullptr);

    EXPECT_NE(dynamic_cast<const csp::multiplayer::AudioSpaceComponent*>(Component), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, AddComponentFromItemComponentDataWithLargeTypeIdCreatesSchemaComponent)
{
    constexpr uint64_t LargeTypeId = std::numeric_limits<uint64_t>::max();

    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { LargeTypeId },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto ComponentData = std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData> {
        { csp::multiplayer::COMPONENT_KEY_COMPONENTTYPE, csp::multiplayer::mcs::ItemComponentData { LargeTypeId } },
    };

    Entity->AddComponentFromItemComponentData(0, csp::multiplayer::mcs::ItemComponentData { ComponentData });

    const auto* Component = Entity->GetComponent(0);

    EXPECT_NE(Component, nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, AddComponentFromItemComponentDataCreatesSchemaComponent)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto ComponentData = std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData> {
        { csp::multiplayer::COMPONENT_KEY_COMPONENTTYPE, csp::multiplayer::mcs::ItemComponentData { uint64_t { 123 } } },
        { 0, csp::multiplayer::mcs::ItemComponentData { std::string { "OverriddenValue" } } },
    };

    Entity->AddComponentFromItemComponentData(0, csp::multiplayer::mcs::ItemComponentData { ComponentData });

    const auto* Component = Entity->GetComponent(0);

    EXPECT_NE(Component, nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, AddComponentFromItemComponentDataWithHardcodedTypeIdCreatesConcreteType)
{
    auto Fixture = TestFixture({});

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto ComponentData = std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData> {
        { csp::multiplayer::COMPONENT_KEY_COMPONENTTYPE, csp::multiplayer::mcs::ItemComponentData { uint64_t { 17 } } },
    };

    Entity->AddComponentFromItemComponentData(0, csp::multiplayer::mcs::ItemComponentData { ComponentData });

    const auto* Component = Entity->GetComponent(0);
    ASSERT_NE(Component, nullptr);

    EXPECT_NE(dynamic_cast<const csp::multiplayer::AudioSpaceComponent*>(Component), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, AddComponentFromItemComponentDataPatchAddsSchemaComponent)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto ComponentData = std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData> {
        { csp::multiplayer::COMPONENT_KEY_COMPONENTTYPE, csp::multiplayer::mcs::ItemComponentData { uint64_t { 123 } } },
    };

    Entity->AddComponentFromItemComponentDataPatch(0, csp::multiplayer::mcs::ItemComponentData { ComponentData });

    const auto* Component = Entity->GetComponent(0);

    EXPECT_NE(Component, nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, AddComponentFromItemComponentDataPatchWithHardcodedTypeIdCreatesConcreteType)
{
    auto Fixture = TestFixture({});

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto ComponentData = std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData> {
        { csp::multiplayer::COMPONENT_KEY_COMPONENTTYPE, csp::multiplayer::mcs::ItemComponentData { uint64_t { 17 } } },
    };

    Entity->AddComponentFromItemComponentDataPatch(0, csp::multiplayer::mcs::ItemComponentData { ComponentData });

    const auto* Component = Entity->GetComponent(0);
    ASSERT_NE(Component, nullptr);

    EXPECT_NE(dynamic_cast<const csp::multiplayer::AudioSpaceComponent*>(Component), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, AddComponentFromItemComponentDataSetsPropertyValues)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "DefaultValue" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto ComponentData = std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData> {
        { csp::multiplayer::COMPONENT_KEY_COMPONENTTYPE, csp::multiplayer::mcs::ItemComponentData { uint64_t { 123 } } },
        { 0, csp::multiplayer::mcs::ItemComponentData { std::string { "OverriddenValue" } } },
    };

    Entity->AddComponentFromItemComponentData(0, csp::multiplayer::mcs::ItemComponentData { ComponentData });

    const auto* Component = Entity->GetComponent(0);
    ASSERT_NE(Component, nullptr);

    const auto* Value = Component->GetProperty(0);
    ASSERT_NE(Value, nullptr);

    EXPECT_EQ(Value->GetString(), csp::common::String { "OverriddenValue" });
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, AddComponentFromItemComponentDataPatchSetsPropertyValues)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "DefaultValue" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto ComponentData = std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData> {
        { csp::multiplayer::COMPONENT_KEY_COMPONENTTYPE, csp::multiplayer::mcs::ItemComponentData { uint64_t { 123 } } },
        { 0, csp::multiplayer::mcs::ItemComponentData { std::string { "OverriddenValue" } } },
    };

    Entity->AddComponentFromItemComponentDataPatch(0, csp::multiplayer::mcs::ItemComponentData { ComponentData });

    const auto* Component = Entity->GetComponent(0);
    ASSERT_NE(Component, nullptr);

    const auto* Value = Component->GetProperty(0);
    ASSERT_NE(Value, nullptr);

    EXPECT_EQ(Value->GetString(), csp::common::String { "OverriddenValue" });
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, AddComponentWithTypeIdAboveLegacyRangeNotTreatedAsLegacy)
{
    using Underlying = std::underlying_type_t<csp::multiplayer::ComponentType>;
    constexpr auto WrapOffset = uint64_t { std::numeric_limits<Underlying>::max() } + 1;
    constexpr auto WrappedTypeId = WrapOffset + static_cast<uint64_t>(csp::multiplayer::ComponentType::Audio);
    static_assert(static_cast<csp::multiplayer::ComponentType>(WrappedTypeId) == csp::multiplayer::ComponentType::Audio);

    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { WrappedTypeId },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto* Component = Entity->AddComponentByTypeId(WrappedTypeId);
    ASSERT_NE(Component, nullptr);

    EXPECT_EQ(dynamic_cast<const csp::multiplayer::AudioSpaceComponent*>(Component), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, AddComponentAboveLegacyRangeNotMisinterpretedAsScript)
{
    using Underlying = std::underlying_type_t<csp::multiplayer::ComponentType>;
    constexpr auto WrapOffset = uint64_t { std::numeric_limits<Underlying>::max() } + 1;
    constexpr auto WrappedTypeId = WrapOffset + static_cast<uint64_t>(csp::multiplayer::ComponentType::ScriptData);
    static_assert(static_cast<csp::multiplayer::ComponentType>(WrappedTypeId) == csp::multiplayer::ComponentType::ScriptData);

    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { WrappedTypeId },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto* ScriptComponent = Entity->AddComponent(csp::multiplayer::ComponentType::ScriptData);
    ASSERT_NE(ScriptComponent, nullptr);

    const auto* Component = Entity->AddComponentByTypeId(WrappedTypeId);
    ASSERT_NE(Component, nullptr);
    EXPECT_NE(Component, ScriptComponent);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, AddComponentFromItemDataAboveLegacyRangeNotTreatedAsLegacy)
{
    using Underlying = std::underlying_type_t<csp::multiplayer::ComponentType>;
    constexpr auto WrapOffset = uint64_t { std::numeric_limits<Underlying>::max() } + 1;
    constexpr auto WrappedTypeId = WrapOffset + static_cast<uint64_t>(csp::multiplayer::ComponentType::Audio);
    static_assert(static_cast<csp::multiplayer::ComponentType>(WrappedTypeId) == csp::multiplayer::ComponentType::Audio);

    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { WrappedTypeId },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto ComponentData = std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData> {
        { csp::multiplayer::COMPONENT_KEY_COMPONENTTYPE, csp::multiplayer::mcs::ItemComponentData { WrappedTypeId } },
    };

    Entity->AddComponentFromItemComponentData(0, csp::multiplayer::mcs::ItemComponentData { ComponentData });

    const auto* Component = Entity->GetComponent(0);
    ASSERT_NE(Component, nullptr);

    EXPECT_EQ(dynamic_cast<const csp::multiplayer::AudioSpaceComponent*>(Component), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, AddComponentFromItemDataAboveLegacyRangeNotRejectedAsInvalid)
{
    using Underlying = std::underlying_type_t<csp::multiplayer::ComponentType>;
    constexpr auto WrapOffset = uint64_t { std::numeric_limits<Underlying>::max() } + 1;
    constexpr auto WrappedTypeId = WrapOffset + static_cast<uint64_t>(csp::multiplayer::ComponentType::Invalid);
    static_assert(static_cast<csp::multiplayer::ComponentType>(WrappedTypeId) == csp::multiplayer::ComponentType::Invalid);

    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { WrappedTypeId },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto ComponentData = std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData> {
        { csp::multiplayer::COMPONENT_KEY_COMPONENTTYPE, csp::multiplayer::mcs::ItemComponentData { WrappedTypeId } },
    };

    Entity->AddComponentFromItemComponentData(0, csp::multiplayer::mcs::ItemComponentData { ComponentData });

    EXPECT_NE(Entity->GetComponent(0), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, GetPropertyReturnsDefaultValue)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    const auto* Value = Component->GetProperty(0);
    ASSERT_NE(Value, nullptr);

    EXPECT_EQ(*Value, "Value");
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, GetPropertyReturnsNullptrForUnknownKey)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    EXPECT_EQ(Component->GetProperty(999), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, SetPropertyWithMatchingTypeSucceeds)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    Component->SetProperty(0, "NewValue");

    const auto* Value = Component->GetProperty(0);
    ASSERT_NE(Value, nullptr);

    EXPECT_EQ(*Value, "NewValue");
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, SetPropertyWithMismatchedTypeLeavesValueUnchanged)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
                {
                    1,
                    "mapProperty",
                    PlainValue<std::unordered_map<std::string, std::string>> {
                        {
                            { "modelA", "materialA" },
                        },
                    },
                },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    {
        Component->SetProperty(0, int64_t { 42 });

        const auto* Value = Component->GetProperty(0);
        ASSERT_NE(Value, nullptr);

        EXPECT_EQ(*Value, "Value");
    }

    {
        Component->SetProperty(1,
            csp::common::Map<csp::common::String, csp::common::ReplicatedValue> {
                { "modelA", csp::common::ReplicatedValue { int64_t { 42 } } },
            });

        const auto* Value = Component->GetProperty(1);
        ASSERT_NE(Value, nullptr);

        const auto Expected = csp::common::ReplicatedValue {
            csp::common::Map<csp::common::String, csp::common::ReplicatedValue> {
                { "modelA", csp::common::ReplicatedValue { "materialA" } },
            },
        };

        EXPECT_EQ(*Value, Expected);
    }
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, SetPropertyWithUnknownKeyHasNoEffect)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    Component->SetProperty(999, "SomeValue");

    EXPECT_EQ(Component->GetProperty(999), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, SetPropertyWhenParentIsLockedHasNoEffect)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", PlainValue<std::string> { "Value" } },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    Entity->Lock();
    Component->SetProperty(0, "NewValue");

    const auto* Value = Component->GetProperty(0);
    ASSERT_NE(Value, nullptr);

    EXPECT_EQ(*Value, "Value");
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, SetPropertyIgnoresValueOutsideRange)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "floatProp",
                    BoundedValue<float> {
                        0.5f,
                        { 0.0f, 1.0f },
                    },
                },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    Component->SetProperty(0, 2.0f);

    const auto* Value = Component->GetProperty(0);
    ASSERT_NE(Value, nullptr);

    EXPECT_EQ(*Value, 0.5f);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, SetPropertyAppliesValueInsideRange)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "floatProp",
                    BoundedValue<float> {
                        0.5f,
                        { 0.0f, 1.0f },
                    },
                },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    {
        Component->SetProperty(0, 0.75f);

        const auto* Value = Component->GetProperty(0);
        ASSERT_NE(Value, nullptr);

        EXPECT_EQ(*Value, 0.75f);
    }

    // The bounds themselves are inclusive.
    {
        Component->SetProperty(0, 0.0f);

        const auto* Value = Component->GetProperty(0);
        ASSERT_NE(Value, nullptr);

        EXPECT_EQ(*Value, 0.0f);
    }

    {
        Component->SetProperty(0, 1.0f);

        const auto* Value = Component->GetProperty(0);
        ASSERT_NE(Value, nullptr);

        EXPECT_EQ(*Value, 1.0f);
    }
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, SetPropertyIgnoresValueNotInOptions)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "intProp",
                    EnumeratedValue<int64_t> {
                        int64_t { 0 },
                        {
                            SchemaOption<int64_t> { "Off", int64_t { 0 } },
                            SchemaOption<int64_t> { "On", int64_t { 1 } },
                        },
                    },
                },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    Component->SetProperty(0, int64_t { 2 });

    const auto* Value = Component->GetProperty(0);
    ASSERT_NE(Value, nullptr);

    EXPECT_EQ(*Value, int64_t { 0 });
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, SetPropertyAppliesValueInOptions)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "intProp",
                    EnumeratedValue<int64_t> {
                        int64_t { 0 },
                        {
                            SchemaOption<int64_t> { "Off", int64_t { 0 } },
                            SchemaOption<int64_t> { "On", int64_t { 1 } },
                        },
                    },
                },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    {
        Component->SetProperty(0, int64_t { 1 });

        const auto* Value = Component->GetProperty(0);
        ASSERT_NE(Value, nullptr);

        EXPECT_EQ(*Value, int64_t { 1 });
    }

    {
        Component->SetProperty(0, int64_t { 0 });

        const auto* Value = Component->GetProperty(0);
        ASSERT_NE(Value, nullptr);

        EXPECT_EQ(*Value, int64_t { 0 });
    }
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, SetPropertyReflectsInTypedGetter)
{
    auto Fixture = TestFixture({});

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(static_cast<uint64_t>(csp::multiplayer::ComponentType::Audio));
    ASSERT_NE(Component, nullptr);

    const auto* AudioComponent = dynamic_cast<const csp::multiplayer::AudioSpaceComponent*>(Component);
    ASSERT_NE(AudioComponent, nullptr);

    const auto NewPosition = csp::common::Vector3 { 1.0f, 2.0f, 3.0f };
    Component->SetProperty(static_cast<uint16_t>(csp::multiplayer::AudioPropertyKeys::Position), NewPosition);

    EXPECT_EQ(AudioComponent->GetPosition(), NewPosition);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, TypedSetterReflectsInGetProperty)
{
    auto Fixture = TestFixture({});

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(static_cast<uint64_t>(csp::multiplayer::ComponentType::Audio));
    ASSERT_NE(Component, nullptr);

    auto* AudioComponent = dynamic_cast<csp::multiplayer::AudioSpaceComponent*>(Component);
    ASSERT_NE(AudioComponent, nullptr);

    const auto NewPosition = csp::common::Vector3 { 4.0f, 5.0f, 6.0f };
    AudioComponent->SetPosition(NewPosition);

    const auto* SchemaValue = Component->GetProperty(static_cast<uint16_t>(csp::multiplayer::AudioPropertyKeys::Position));
    ASSERT_NE(SchemaValue, nullptr);

    EXPECT_EQ(SchemaValue->GetVector3(), NewPosition);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, ComponentPropertyEquality)
{
    using Property = csp::multiplayer::ComponentProperty;

    {
        const auto A = Property {
            0,
            "name",
            PlainValue<std::string> { "value" },
        };
        const auto B = Property {
            0,
            "name",
            PlainValue<std::string> { "value" },
        };
        EXPECT_EQ(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            PlainValue<std::string> { "value" },
        };
        const auto B = Property {
            1,
            "name",
            PlainValue<std::string> { "value" },
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            PlainValue<std::string> { "value" },
        };
        const auto B = Property {
            0,
            "other",
            PlainValue<std::string> { "value" },
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            PlainValue<std::string> { "value" },
        };
        const auto B = Property {
            0,
            "name",
            PlainValue<std::string> { "other" },
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            PlainValue<float> { 1.0f },
        };
        const auto B = Property {
            0,
            "name",
            BoundedValue<float> {
                1.0f,
                { 0.0f, 1.0f },
            },
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            BoundedValue<float> {
                1.0f,
                { 0.0f, 1.0f },
            },
        };
        const auto B = Property {
            0,
            "name",
            BoundedValue<float> {
                1.0f,
                { 0.0f, 2.0f },
            },
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            EnumeratedValue<int64_t> {
                int64_t { 0 },
                {
                    SchemaOption<int64_t> { "Off", int64_t { 0 } },
                    SchemaOption<int64_t> { "On", int64_t { 1 } },
                },
            },
        };
        const auto B = Property {
            0,
            "name",
            EnumeratedValue<int64_t> {
                int64_t { 0 },
                {},
            },
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            PlainValue<std::string> { "value" },
        };
        const auto B = Property {
            0,
            "name",
            PlainValue<std::string> { "value" },
            /*.IsScriptable =*/true,
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            PlainValue<std::string> { "value" },
            /*.IsScriptable =*/true,
        };
        const auto B = Property {
            0,
            "name",
            PlainValue<std::string> { "value" },
            /*.IsScriptable =*/false,
        };
        EXPECT_NE(A, B);
    }
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, ComponentSchemaEquality)
{
    {
        const auto A = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "prop",
                    PlainValue<std::string> { "value" },
                },
            },
        };
        const auto B = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "prop",
                    PlainValue<std::string> { "value" },
                },
            },
        };
        EXPECT_EQ(A, B);
    }
    {
        const auto A = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "prop",
                    PlainValue<std::string> { "value" },
                },
            },
        };
        const auto B = Schema {
            Schema::TypeIdType { 456 },
            "Example",
            {
                {
                    0,
                    "prop",
                    PlainValue<std::string> { "value" },
                },
            },
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "prop",
                    PlainValue<std::string> { "value" },
                },
            },
        };
        const auto B = Schema {
            Schema::TypeIdType { 123 },
            "Other",
            {
                {
                    0,
                    "prop",
                    PlainValue<std::string> { "value" },
                },
            },
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "prop",
                    PlainValue<std::string> { "value" },
                },
            },
        };
        const auto B = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {},
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "prop",
                    PlainValue<std::string> { "value" },
                },
            },
        };
        const auto B = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "prop",
                    PlainValue<std::string> { "other" },
                },
            },
        };
        EXPECT_NE(A, B);
    }
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonParsesAllPropertyTypes)
{
    constexpr auto RawJson = R"({
        "typeId": 123,
        "name": "Example",
        "properties": [
            {
                "key": 0,
                "name": "stringProperty",
                "type": "string",
                "defaultValue": "hello"
            },
            {
                "key": 1,
                "name": "floatProperty",
                "type": "float",
                "defaultValue": 1.5
            },
            {
                "key": 2,
                "name": "intProperty",
                "type": "int",
                "defaultValue": 42
            },
            {
                "key": 3,
                "name": "boolProperty",
                "type": "bool",
                "defaultValue": true
            },
            {
                "key": 4,
                "name": "vec2Property",
                "type": "vec2",
                "defaultValue": [1.0, 2.0]
            },
            {
                "key": 5,
                "name": "vec3Property",
                "type": "vec3",
                "defaultValue": [1.0, 2.0, 3.0]
            },
            {
                "key": 6,
                "name": "vec4Property",
                "type": "vec4",
                "defaultValue": [1.0, 2.0, 3.0, 4.0]
            },
            {
                "key": 7,
                "name": "mapProperty",
                "type": "stringToStringMap",
                "defaultValue": { "modelA": "materialA", "modelB": "materialB" }
            }
        ]
    })";

    const auto Expected = Schema {
        Schema::TypeIdType { 123 },
        "Example",
        {
            {
                0,
                "stringProperty",
                PlainValue<std::string> { std::string { "hello" } },
            },
            {
                1,
                "floatProperty",
                PlainValue<float> { 1.5f },
            },
            {
                2,
                "intProperty",
                PlainValue<int64_t> { int64_t { 42 } },
            },
            {
                3,
                "boolProperty",
                PlainValue<bool> { true },
            },
            {
                4,
                "vec2Property",
                PlainValue<csp::common::Vector2> { csp::common::Vector2 { 1.0f, 2.0f } },
            },
            {
                5,
                "vec3Property",
                PlainValue<csp::common::Vector3> { csp::common::Vector3 { 1.0f, 2.0f, 3.0f } },
            },
            {
                6,
                "vec4Property",
                PlainValue<csp::common::Vector4> { csp::common::Vector4 { 1.0f, 2.0f, 3.0f, 4.0f } },
            },
            {
                7,
                "mapProperty",
                PlainValue<std::unordered_map<std::string, std::string>> {
                    {
                        { "modelA", "materialA" },
                        { "modelB", "materialB" },
                    },
                },
            },
        },
    };

    const auto Result = Schema::FromJson(csp::common::String { RawJson });

    ASSERT_TRUE(Result.HasValue());
    EXPECT_EQ(*Result, Expected);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, JsonSerializationRoundTrip)
{
    const auto Original = Schema {
        Schema::TypeIdType { 123 },
        "Example",
        {
            {
                0,
                "stringProperty",
                PlainValue<std::string> { "value" },
            },
            {
                1,
                "boundedProperty",
                BoundedValue<float> {
                    0.5f,
                    { 0.0f, 1.0f },
                },
            },
            {
                2,
                "enumeratedProperty",
                EnumeratedValue<int64_t> {
                    int64_t { 0 },
                    {
                        SchemaOption<int64_t> { "Off", int64_t { 0 } },
                        SchemaOption<int64_t> { "On", int64_t { 1 } },
                    },
                },
            },
            {
                3,
                "mapProperty",
                PlainValue<std::unordered_map<std::string, std::string>> {
                    {
                        { "modelA", "materialA" },
                    },
                },
            },
            {
                4,
                "scriptableProperty",
                PlainValue<std::string> { "value" },
                /*.IsScriptable =*/true,
            },
            {
                5,
                "nonScriptableProperty",
                PlainValue<std::string> { "value" },
                /*.IsScriptable =*/false,
            },
        },
        /*.IsScriptable =*/false,
    };

    const auto Json = Schema::ToJson(Original);
    const auto Result = Schema::FromJson(Json);

    ASSERT_TRUE(Result.HasValue());
    EXPECT_EQ(*Result, Original);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonAcceptsSchemaWithNoProperties)
{
    constexpr auto RawJson = R"({
        "typeId": 123,
        "name": "Example",
        "properties": []
    })";

    const auto Expected = Schema {
        Schema::TypeIdType { 123 },
        "Example",
        {},
    };

    const auto Result = Schema::FromJson(csp::common::String { RawJson });

    ASSERT_TRUE(Result.HasValue());
    EXPECT_EQ(*Result, Expected);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsNonJsonInput)
{
    constexpr auto RawJson = R"(not json)";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsEmptyString)
{
    const auto Result = Schema::FromJson(csp::common::String { "" });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsMissingTypeId)
{
    constexpr auto RawJson = R"({ "name": "Example", "properties": [] })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsMissingName)
{
    constexpr auto RawJson = R"({ "typeId": 123, "properties": [] })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsMissingProperties)
{
    constexpr auto RawJson = R"({ "typeId": 123, "name": "Example" })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsNonNumericTypeId)
{
    constexpr auto RawJson = R"({ "typeId": "123", "name": "Example", "properties": [] })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsNonStringName)
{
    constexpr auto RawJson = R"({ "typeId": 123, "name": 42, "properties": [] })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsNonArrayProperties)
{
    constexpr auto RawJson = R"({ "typeId": 123, "name": "Example", "properties": {} })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsPropertyWithMissingKey)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "name": "prop", "type": "string", "defaultValue": "hello" }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsPropertyWithMissingName)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "type": "string", "defaultValue": "hello" }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsPropertyWithMissingType)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "defaultValue": "hello" }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsPropertyWithMissingDefaultValue)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "type": "string" }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsNonBoolPropertyScriptable)
{
    constexpr auto RawJson = R"({
        "typeId": 123,
        "name": "Example",
        "properties": [
            {
                "key": 0,
                "name": "prop",
                "type": "string",
                "defaultValue": "",
                "scriptable": "yes"
            }
        ]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsNonBoolSchemaScriptable)
{
    constexpr auto RawJson = R"({
        "typeId": 123,
        "name": "Example",
        "scriptable": "yes",
        "properties": []
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsPropertyWithUnrecognisedType)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "type": "colour", "defaultValue": "red" }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsStringPropertyWithMismatchedDefaultValue)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "type": "string", "defaultValue": 42 }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsFloatPropertyWithMismatchedDefaultValue)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "type": "float", "defaultValue": "hello" }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsIntPropertyWithMismatchedDefaultValue)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "type": "int", "defaultValue": "hello" }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsBoolPropertyWithMismatchedDefaultValue)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "type": "bool", "defaultValue": "hello" }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsVec2PropertyWithMismatchedDefaultValue)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "type": "vec2", "defaultValue": 42 }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsVec3PropertyWithMismatchedDefaultValue)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "type": "vec3", "defaultValue": 42 }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsVec4PropertyWithMismatchedDefaultValue)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "type": "vec4", "defaultValue": 42 }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsVec2PropertyWithWrongLengthDefaultValue)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "type": "vec2", "defaultValue": [1.0] }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsVec3PropertyWithWrongLengthDefaultValue)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "type": "vec3", "defaultValue": [1.0, 2.0] }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsVec4PropertyWithWrongLengthDefaultValue)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "type": "vec4", "defaultValue": [1.0, 2.0, 3.0] }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsVec2PropertyWithNonNumericElements)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "type": "vec2", "defaultValue": [1.0, "hello"] }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsVec3PropertyWithNonNumericElements)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "type": "vec3", "defaultValue": [1.0, "hello", 3.0] }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsVec4PropertyWithNonNumericElements)
{
    constexpr auto RawJson = R"({
        "typeId": 123, "name": "Example",
        "properties": [{ "key": 0, "name": "prop", "type": "vec4", "defaultValue": [1.0, "hello", 3.0, 4.0] }]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsStringToStringMapPropertyWithMismatchedDefaultValue)
{
    constexpr auto RawJson = R"({
        "typeId": 123,
        "name": "Example",
        "properties": [
            {
                "key": 0,
                "name": "prop",
                "type": "stringToStringMap",
                "defaultValue": 42
            }
        ]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsStringToStringMapPropertyWithNonStringValues)
{
    constexpr auto RawJson = R"({
        "typeId": 123,
        "name": "Example",
        "properties": [
            {
                "key": 0,
                "name": "prop",
                "type": "stringToStringMap",
                "defaultValue": { "modelA": 42 }
            }
        ]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonParsesConstrainedProperties)
{
    constexpr auto RawJson = R"({
        "typeId": 123,
        "name": "Example",
        "properties": [
            {
                "key": 0,
                "name": "intProp",
                "type": "int",
                "defaultValue": 0,
                "options": [
                    { "name": "Off", "value": 0 },
                    { "name": "On", "value": 1 }
                ]
            },
            {
                "key": 1,
                "name": "floatProp",
                "type": "float",
                "defaultValue": 0.5,
                "range": { "min": 0.0, "max": 1.0 }
            }
        ]
    })";

    const auto Expected = Schema {
        Schema::TypeIdType { 123 },
        "Example",
        {
            {
                0,
                "intProp",
                EnumeratedValue<int64_t> {
                    int64_t { 0 },
                    {
                        SchemaOption<int64_t> { "Off", int64_t { 0 } },
                        SchemaOption<int64_t> { "On", int64_t { 1 } },
                    },
                },
            },
            {
                1,
                "floatProp",
                BoundedValue<float> {
                    0.5f,
                    { 0.0f, 1.0f },
                },
            },
        },
    };

    const auto Result = Schema::FromJson(csp::common::String { RawJson });

    ASSERT_TRUE(Result.HasValue());
    EXPECT_EQ(*Result, Expected);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsPropertyWithMalformedRange)
{
    {
        // "range" is not an object
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "float",
                    "defaultValue": 1.0,
                    "range": [0.0, 1.0]
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        // "range" missing "min"
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "float",
                    "defaultValue": 1.0,
                    "range": { "max": 1.0 }
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        // "range" missing "max"
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "float",
                    "defaultValue": 1.0,
                    "range": { "min": 0.0 }
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        // "min" and "max" are not numbers
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "float",
                    "defaultValue": 1.0,
                    "range": { "min": "low", "max": "high" }
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsPropertyWithMalformedOption)
{
    {
        // "options" is not an array
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "int",
                    "defaultValue": 0,
                    "options": {}
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        // option is not an object
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "int",
                    "defaultValue": 0,
                    "options": [
                        0
                    ]
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        // option missing "name"
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "int",
                    "defaultValue": 0,
                    "options": [
                        { "value": 0 }
                    ]
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        // option missing "value"
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "int",
                    "defaultValue": 0,
                    "options": [
                        { "name": "Off" }
                    ]
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        // option "value" has the wrong type
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "int",
                    "defaultValue": 0,
                    "options": [
                        { "name": "Off", "value": "zero" }
                    ]
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsRangeOnUnsupportedPropertyTypes)
{
    {
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "bool",
                    "defaultValue": false,
                    "range": { "min": 0, "max": 1 }
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "string",
                    "defaultValue": "",
                    "range": { "min": "a", "max": "z" }
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "vec2",
                    "defaultValue": [0.0, 0.0],
                    "range": { "min": 0.0, "max": 1.0 }
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "vec3",
                    "defaultValue": [0.0, 0.0, 0.0],
                    "range": { "min": 0.0, "max": 1.0 }
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "vec4",
                    "defaultValue": [0.0, 0.0, 0.0, 0.0],
                    "range": { "min": 0.0, "max": 1.0 }
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "stringToStringMap",
                    "defaultValue": { "modelA": "materialA" },
                    "range": { "min": "a", "max": "z" }
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsOptionsOnUnsupportedPropertyTypes)
{
    {
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "bool",
                    "defaultValue": false,
                    "options": [
                        { "name": "Off", "value": false },
                        { "name": "On", "value": true }
                    ]
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "vec2",
                    "defaultValue": [0.0, 0.0],
                    "options": [
                        { "name": "Zero", "value": [0.0, 0.0] }
                    ]
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "vec3",
                    "defaultValue": [0.0, 0.0, 0.0],
                    "options": [
                        { "name": "Zero", "value": [0.0, 0.0, 0.0] }
                    ]
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "vec4",
                    "defaultValue": [0.0, 0.0, 0.0, 0.0],
                    "options": [
                        { "name": "Identity", "value": [0.0, 0.0, 0.0, 1.0] }
                    ]
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
    {
        constexpr auto RawJson = R"({
            "typeId": 123,
            "name": "Example",
            "properties": [
                {
                    "key": 0,
                    "name": "prop",
                    "type": "stringToStringMap",
                    "defaultValue": { "modelA": "materialA" },
                    "options": [
                        { "name": "Default", "value": { "modelA": "materialA" } }
                    ]
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsBothRangeAndOptionsOnProperty)
{
    constexpr auto RawJson = R"({
        "typeId": 123,
        "name": "Example",
        "properties": [
            {
                "key": 0,
                "name": "prop",
                "type": "float",
                "defaultValue": 0.5,
                "range": { "min": 0.0, "max": 1.0 },
                "options": [
                    { "name": "Low", "value": 0.0 },
                    { "name": "High", "value": 1.0 }
                ]
            }
        ]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, IsCompatibleReturnsTrueForValidUpdate)
{
    const auto BuiltIn = Schema {
        Schema::TypeIdType { 808 },
        "DistortionAudioEffect",
        {
            { 0, "gain", PlainValue<float> { 0.25f } },
            { 1, "level", PlainValue<float> { 0.5f } },
        },
    };

    const auto Updated = Schema {
        Schema::TypeIdType { 808 },
        "DistortionAudioEffect",
        {
            { 0, "gain", PlainValue<float> { 0.25f } },
            { 1, "level", PlainValue<float> { 0.5f } },
            { 2, "tone", PlainValue<float> { 0.5f } },
        },
    };

    EXPECT_TRUE(csp::multiplayer::IsCompatible(BuiltIn, Updated));
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, IsCompatibleReturnsFalseForNameMismatch)
{
    const auto BuiltIn = Schema {
        Schema::TypeIdType { 808 },
        "DistortionAudioEffect",
        {
            { 0, "gain", PlainValue<float> { 0.25f } },
            { 1, "level", PlainValue<float> { 0.5f } },
        },
    };

    const auto Updated = Schema {
        Schema::TypeIdType { 808 },
        "MegaDistortionAudioEffect",
        {
            { 0, "gain", PlainValue<float> { 0.25f } },
            { 1, "level", PlainValue<float> { 0.5f } },
            { 2, "tone", PlainValue<float> { 0.5f } },
        },
    };

    EXPECT_FALSE(csp::multiplayer::IsCompatible(BuiltIn, Updated));
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, IsCompatibleReturnsFalseForMissingProperty)
{
    const auto BuiltIn = Schema {
        Schema::TypeIdType { 808 },
        "DistortionAudioEffect",
        {
            { 0, "gain", PlainValue<float> { 0.25f } },
            { 1, "level", PlainValue<float> { 0.5f } },
        },
    };

    const auto Updated = Schema {
        Schema::TypeIdType { 808 },
        "DistortionAudioEffect",
        {
            { 0, "gain", PlainValue<float> { 0.25f } },
            { 2, "tone", PlainValue<float> { 0.5f } },
        },
    };

    EXPECT_FALSE(csp::multiplayer::IsCompatible(BuiltIn, Updated));
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, IsCompatibleReturnsFalseForPropertyDefaultMismatch)
{
    const auto BuiltIn = Schema {
        Schema::TypeIdType { 808 },
        "DistortionAudioEffect",
        {
            { 0, "gain", PlainValue<float> { 0.25f } },
            { 1, "level", PlainValue<float> { 0.5f } },
        },
    };

    const auto Updated = Schema {
        Schema::TypeIdType { 808 },
        "DistortionAudioEffect",
        {
            { 0, "gain", PlainValue<float> { 0.5f } },
            { 1, "level", PlainValue<float> { 0.5f } },
            { 2, "tone", PlainValue<float> { 0.5f } },
        },
    };

    EXPECT_FALSE(csp::multiplayer::IsCompatible(BuiltIn, Updated));
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, IsCompatibleReturnsFalseForPropertyNameMismatch)
{
    const auto BuiltIn = Schema {
        Schema::TypeIdType { 808 },
        "DistortionAudioEffect",
        {
            { 0, "gain", PlainValue<float> { 0.25f } },
            { 1, "level", PlainValue<float> { 0.5f } },
        },
    };

    const auto Updated = Schema {
        Schema::TypeIdType { 808 },
        "DistortionAudioEffect",
        {
            { 0, "drive", PlainValue<float> { 1.0f } },
            { 1, "level", PlainValue<float> { 0.5f } },
            { 2, "tone", PlainValue<float> { 0.5f } },
        },
    };

    EXPECT_FALSE(csp::multiplayer::IsCompatible(BuiltIn, Updated));
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, BuiltInSchemasRoundTripThroughJson)
{
    auto LogSystem = csp::common::LogSystem {};

    const auto Original = csp::multiplayer::GetBuiltInComponentSchemas();
    const auto Parsed = csp::multiplayer::ComponentSchemasFromJson({ csp::GetComponentSchemasJson() }, LogSystem);

    ASSERT_EQ(Parsed.Size(), Original.Size());

    for (size_t i = 0; i < Original.Size(); ++i)
    {
        EXPECT_EQ(Parsed[i], Original[i]) << "schema " << i << " (" << Original[i].Name.c_str() << ") did not round trip";
    }
}
