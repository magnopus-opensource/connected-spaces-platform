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

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/Components/AudioSpaceComponent.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "Multiplayer/MCS/MCSTypes.h"
#include "Multiplayer/SpaceEntityKeys.h"

#include <limits>
#include <memory>

namespace
{

using Schema = csp::multiplayer::ComponentSchema;

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
                { 0, "stringProperty", "Value" },
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
                { 0, "stringProperty", "Value" },
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
                { 0, "stringProperty", "Value" },
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
                { 0, "stringProperty", "Value" },
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
                { 0, "stringProperty", "Value" },
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
                { 0, "stringProperty", "Value" },
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
                { 0, "stringProperty", "Value" },
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
                { 0, "stringProperty", "Value" },
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
                { 0, "stringProperty", "Value" },
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
                { 0, "stringProperty", "DefaultValue" },
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

    const auto* Value = Component->GetSchemaProperty(0);
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
                { 0, "stringProperty", "DefaultValue" },
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

    const auto* Value = Component->GetSchemaProperty(0);
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
                { 0, "stringProperty", "Value" },
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
                { 0, "stringProperty", "Value" },
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
                { 0, "stringProperty", "Value" },
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
                { 0, "stringProperty", "Value" },
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

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, GetSchemaPropertyReturnsDefaultValue)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", "Value" },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    const auto* Value = Component->GetSchemaProperty(0);
    ASSERT_NE(Value, nullptr);

    EXPECT_EQ(*Value, "Value");
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, GetSchemaPropertyReturnsNullptrForUnknownKey)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", "Value" },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    const auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    EXPECT_EQ(Component->GetSchemaProperty(999), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, SetSchemaPropertyWithMatchingTypeSucceeds)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", "Value" },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    Component->SetSchemaProperty(0, "NewValue");

    const auto* Value = Component->GetSchemaProperty(0);
    ASSERT_NE(Value, nullptr);

    EXPECT_EQ(*Value, "NewValue");
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, SetSchemaPropertyWithMismatchedTypeLeavesValueUnchanged)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", "Value" },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    Component->SetSchemaProperty(0, int64_t { 42 });

    const auto* Value = Component->GetSchemaProperty(0);
    ASSERT_NE(Value, nullptr);

    EXPECT_EQ(*Value, "Value");
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, SetSchemaPropertyWithUnknownKeyHasNoEffect)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", "Value" },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    Component->SetSchemaProperty(999, "SomeValue");

    EXPECT_EQ(Component->GetSchemaProperty(999), nullptr);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, SetSchemaPropertyWhenParentIsLockedHasNoEffect)
{
    auto Fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                { 0, "stringProperty", "Value" },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    Entity->Lock();
    Component->SetSchemaProperty(0, "NewValue");

    const auto* Value = Component->GetSchemaProperty(0);
    ASSERT_NE(Value, nullptr);

    EXPECT_EQ(*Value, "Value");
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, SetSchemaPropertyReflectsInTypedGetter)
{
    auto Fixture = TestFixture({});

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(static_cast<uint64_t>(csp::multiplayer::ComponentType::Audio));
    ASSERT_NE(Component, nullptr);

    const auto* AudioComponent = dynamic_cast<const csp::multiplayer::AudioSpaceComponent*>(Component);
    ASSERT_NE(AudioComponent, nullptr);

    const auto NewPosition = csp::common::Vector3 { 1.0f, 2.0f, 3.0f };
    Component->SetSchemaProperty(static_cast<uint16_t>(csp::multiplayer::AudioPropertyKeys::Position), NewPosition);

    EXPECT_EQ(AudioComponent->GetPosition(), NewPosition);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, TypedSetterReflectsInGetSchemaProperty)
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

    const auto* SchemaValue = Component->GetSchemaProperty(static_cast<uint16_t>(csp::multiplayer::AudioPropertyKeys::Position));
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
            "value",
        };
        const auto B = Property {
            0,
            "name",
            "value",
        };
        EXPECT_EQ(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            "value",
        };
        const auto B = Property {
            1,
            "name",
            "value",
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            "value",
        };
        const auto B = Property {
            0,
            "other",
            "value",
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            "value",
        };
        const auto B = Property {
            0,
            "name",
            "other",
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
                    "value",
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
                    "value",
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
                    "value",
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
                    "value",
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
                    "value",
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
                    "value",
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
                    "value",
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
                    "value",
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
                    "other",
                },
            },
        };
        EXPECT_NE(A, B);
    }
}
