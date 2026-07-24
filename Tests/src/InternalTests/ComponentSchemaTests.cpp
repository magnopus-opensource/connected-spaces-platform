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

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, GetPropertyReturnsDefaultValue)
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
                { 0, "stringProperty", "Value" },
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
                { 0, "stringProperty", "Value" },
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
                { 0, "stringProperty", "Value" },
            },
        },
    });

    auto* Entity = Fixture.MakeEntity("Test Entity");
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(uint64_t { 123 });
    ASSERT_NE(Component, nullptr);

    Component->SetProperty(0, int64_t { 42 });

    const auto* Value = Component->GetProperty(0);
    ASSERT_NE(Value, nullptr);

    EXPECT_EQ(*Value, "Value");
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, SetPropertyWithUnknownKeyHasNoEffect)
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
                { 0, "stringProperty", "Value" },
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
    using Option = csp::multiplayer::SchemaOption;

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
    {
        const auto A = Property {
            0,
            "name",
            "value",
            /*.Description =*/"a description",
        };
        const auto B = Property {
            0,
            "name",
            "value",
            /*.Description =*/ {},
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            "value",
            /*.Description =*/ {},
            /*.IsScriptable =*/false,
        };
        const auto B = Property {
            0,
            "name",
            "value",
            /*.Description =*/ {},
            /*.IsScriptable =*/true,
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            "value",
            /*.Description =*/ {},
            /*.IsScriptable =*/true,
            /*.IsDeprecated =*/true,
        };
        const auto B = Property {
            0,
            "name",
            "value",
            /*.Description =*/ {},
            /*.IsScriptable =*/true,
            /*.IsDeprecated =*/false,
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            "value",
            /*.Description =*/ {},
            /*.IsScriptable =*/true,
            /*.IsDeprecated =*/false,
            /*.IsReserved =*/true,
        };
        const auto B = Property {
            0,
            "name",
            "value",
            /*.Description =*/ {},
            /*.IsScriptable =*/true,
            /*.IsDeprecated =*/false,
            /*.IsReserved =*/false,
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            1.0f,
            /*.Description =*/ {},
            /*.IsScriptable =*/true,
            /*.IsDeprecated =*/false,
            /*.IsReserved =*/false,
            /*.RangeMin =*/0.0f,
            /*.RangeMax =*/1.0f,
        };
        const auto B = Property {
            0,
            "name",
            1.0f,
            /*.Description =*/ {},
            /*.IsScriptable =*/true,
            /*.IsDeprecated =*/false,
            /*.IsReserved =*/false,
            /*.RangeMin =*/ {},
            /*.RangeMax =*/ {},
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Property {
            0,
            "name",
            int64_t { 0 },
            /*.Description =*/ {},
            /*.IsScriptable =*/true,
            /*.IsDeprecated =*/false,
            /*.IsReserved =*/false,
            /*.RangeMin =*/ {},
            /*.RangeMax =*/ {},
            /*.Options =*/
            csp::common::Array<Option> {
                { "Off", int64_t { 0 } },
                { "On", int64_t { 1 } },
            },
        };
        const auto B = Property {
            0,
            "name",
            int64_t { 0 },
            /*.Description =*/ {},
            /*.IsScriptable =*/true,
            /*.IsDeprecated =*/false,
            /*.IsReserved =*/false,
            /*.RangeMin =*/ {},
            /*.RangeMax =*/ {},
            /*.Options =*/ {},
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
    {
        const auto A = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {},
            /*.Description =*/"A description.",
        };
        const auto B = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {},
            /*.Description =*/ {},
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {},
            /*.Description =*/ {},
            /*.IsScriptable =*/false,
        };
        const auto B = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {},
            /*.Description =*/ {},
            /*.IsScriptable =*/true,
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {},
            /*.Description =*/ {},
            /*.IsScriptable =*/false,
            /*.IsReserved =*/true,
        };
        const auto B = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {},
            /*.Description =*/ {},
            /*.IsScriptable =*/false,
            /*.IsReserved =*/false,
        };
        EXPECT_NE(A, B);
    }
    {
        const auto A = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {},
            /*.Description =*/ {},
            /*.IsScriptable =*/true,
            /*.IsReserved =*/false,
            /*.IsDeprecated =*/true,
        };
        const auto B = Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {},
            /*.Description =*/ {},
            /*.IsScriptable =*/true,
            /*.IsReserved =*/false,
            /*.IsDeprecated =*/false,
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
                csp::common::String { "hello" },
            },
            {
                1,
                "floatProperty",
                1.5f,
            },
            {
                2,
                "intProperty",
                int64_t { 42 },
            },
            {
                3,
                "boolProperty",
                true,
            },
            {
                4,
                "vec2Property",
                csp::common::Vector2 { 1.0f, 2.0f },
            },
            {
                5,
                "vec3Property",
                csp::common::Vector3 { 1.0f, 2.0f, 3.0f },
            },
            {
                6,
                "vec4Property",
                csp::common::Vector4 { 1.0f, 2.0f, 3.0f, 4.0f },
            },
        },
    };

    const auto Result = Schema::FromJson(csp::common::String { RawJson });

    ASSERT_TRUE(Result.HasValue());
    EXPECT_EQ(*Result, Expected);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, JsonSerializationRoundTrip)
{
    using Option = csp::multiplayer::SchemaOption;

    const auto Original = Schema {
        Schema::TypeIdType { 123 },
        "Example",
        {
            {
                0,
                "stringProperty",
                "value",
            },
            {
                1,
                "intProp",
                int64_t { 0 },
                /*.Description =*/"A description.",
                /*.IsScriptable =*/true,
                /*.IsDeprecated =*/false,
                /*.IsReserved =*/false,
                /*.RangeMin =*/ {},
                /*.RangeMax =*/ {},
                /*.Options =*/
                csp::common::Array<Option> {
                    { "Off", int64_t { 0 } },
                    { "On", int64_t { 1 } },
                },
            },
            {
                2,
                "floatProp",
                1.0f,
                /*.Description =*/"Another description.",
                /*.IsScriptable =*/false,
                /*.IsDeprecated =*/false,
                /*.IsReserved =*/false,
                /*.RangeMin =*/0.0f,
                /*.RangeMax =*/1.0f,
            },
            {
                3,
                "stringProp",
                csp::common::String { "" },
                /*.Description =*/ {},
                /*.IsScriptable =*/true,
                /*.IsDeprecated =*/true,
            },
            {
                4,
                {},
                {},
                /*.Description =*/"Was prop, removed",
                /*.IsScriptable =*/false,
                /*.IsDeprecated =*/false,
                /*.IsReserved =*/true,
            },
        },
        /*.Description =*/"A component description.",
        /*.IsScriptable =*/false,
        /*.IsReserved =*/false,
        /*.IsDeprecated =*/true,
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

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonParsesPropertyMetadata)
{
    using Option = csp::multiplayer::SchemaOption;

    constexpr auto RawJson = R"({
        "typeId": 123,
        "name": "Example",
        "properties": [
            {
                "key": 0,
                "name": "intProp",
                "type": "int",
                "defaultValue": 0,
                "description": "A description.",
                "scripting": false,
                "options": [
                    { "name": "Off", "value": 0 },
                    { "name": "On", "value": 1 }
                ]
            },
            {
                "key": 1,
                "name": "floatProp",
                "type": "float",
                "defaultValue": 1.0,
                "scripting": false,
                "range": { "min": 0.0, "max": 1.0 }
            },
            {
                "key": 2,
                "name": "stringProp",
                "type": "string",
                "defaultValue": "",
                "deprecated": true
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
                int64_t { 0 },
                /*.Description =*/"A description.",
                /*.IsScriptable =*/false,
                /*.IsDeprecated =*/false,
                /*.IsReserved =*/false,
                /*.RangeMin =*/ {},
                /*.RangeMax =*/ {},
                /*.Options =*/
                csp::common::Array<Option> {
                    { "Off", int64_t { 0 } },
                    { "On", int64_t { 1 } },
                },
            },
            {
                1,
                "floatProp",
                1.0f,
                /*.Description =*/ {},
                /*.IsScriptable =*/false,
                /*.IsDeprecated =*/false,
                /*.IsReserved =*/false,
                /*.RangeMin =*/0.0f,
                /*.RangeMax =*/1.0f,
            },
            {
                2,
                "stringProp",
                csp::common::String { "" },
                /*.Description =*/ {},
                /*.IsScriptable =*/true,
                /*.IsDeprecated =*/true,
            },
        },
    };

    const auto Result = Schema::FromJson(csp::common::String { RawJson });

    ASSERT_TRUE(Result.HasValue());
    EXPECT_EQ(*Result, Expected);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonParsesReservedProperty)
{
    constexpr auto RawJson = R"({
        "typeId": 123,
        "name": "Example",
        "properties": [
            {
                "key": 0,
                "name": "prop",
                "type": "string",
                "defaultValue": ""
            },
            {
                "key": 1,
                "reserved": true,
                "description": "Was prop, removed"
            }
        ]
    })";

    const auto Expected = Schema {
        Schema::TypeIdType { 123 },
        "Example",
        {
            {
                0,
                "prop",
                csp::common::String { "" },
            },
            {
                1,
                {},
                {},
                /*.Description =*/"Was prop, removed",
                /*.IsScriptable =*/false,
                /*.IsDeprecated =*/false,
                /*.IsReserved =*/true,
            },
        },
    };

    const auto Result = Schema::FromJson(csp::common::String { RawJson });

    ASSERT_TRUE(Result.HasValue());
    EXPECT_EQ(*Result, Expected);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonParsesReservedSchema)
{
    constexpr auto RawJson = R"({
        "typeId": 5,
        "reserved": true,
        "description": "Was Example, removed"
    })";

    const auto Expected = Schema {
        Schema::TypeIdType { 5 },
        /*.Name =*/ {},
        /*.Properties =*/ {},
        /*.Description =*/"Was Example, removed",
        /*.IsScriptable =*/false,
        /*.IsReserved =*/true,
    };

    const auto Result = Schema::FromJson(csp::common::String { RawJson });

    ASSERT_TRUE(Result.HasValue());
    EXPECT_EQ(*Result, Expected);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, JsonSerializationRoundTripReservedSchema)
{
    const auto Original = Schema {
        Schema::TypeIdType { 5 },
        /*.Name =*/ {},
        /*.Properties =*/ {},
        /*.Description =*/"Was Example, removed",
        /*.IsScriptable =*/false,
        /*.IsReserved =*/true,
    };

    const auto Json = Schema::ToJson(Original);
    const auto Result = Schema::FromJson(Json);

    ASSERT_TRUE(Result.HasValue());
    EXPECT_EQ(*Result, Original);
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsPropertyWithMalformedRange)
{
    constexpr auto RawJson = R"({
        "typeId": 123,
        "name": "Example",
        "properties": [
            {
                "key": 0,
                "name": "floatProp",
                "type": "float",
                "defaultValue": 1.0,
                "range": { "min": "low", "max": "high" }
            }
        ]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, FromJsonRejectsPropertyWithMalformedOption)
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
                    { "name": "On" }
                ]
            }
        ]
    })";
    const auto Result = Schema::FromJson(csp::common::String { RawJson });
    EXPECT_FALSE(Result.HasValue());
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
                    "options": [{ "name": "Off", "value": false }, { "name": "On", "value": true }]
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
                    "options": [{ "name": "Zero", "value": [0.0, 0.0] }]
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
                    "options": [{ "name": "Zero", "value": [0.0, 0.0, 0.0] }]
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
                    "defaultValue": [0.0, 0.0, 0.0, 1.0],
                    "options": [{ "name": "Identity", "value": [0.0, 0.0, 0.0, 1.0] }]
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
                    "type": "bool",
                    "defaultValue": false,
                    "range": { "min": false, "max": true }
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
                    "defaultValue": [0.0, 0.0, 0.0, 1.0],
                    "range": { "min": 0.0, "max": 1.0 }
                }
            ]
        })";
        const auto Result = Schema::FromJson(csp::common::String { RawJson });
        EXPECT_FALSE(Result.HasValue());
    }
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaTests, IsCompatibleReturnsTrueForValidUpdate)
{
    const auto BuiltIn = Schema {
        Schema::TypeIdType { 808 },
        "DistortionAudioEffect",
        {
            { 0, "gain", 0.25f },
            { 1, "level", 0.5f },
        },
    };

    const auto Updated = Schema {
        Schema::TypeIdType { 808 },
        "DistortionAudioEffect",
        {
            { 0, "gain", 0.25f },
            { 1, "level", 0.5f },
            { 2, "tone", 0.5f },
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
            { 0, "gain", 0.25f },
            { 1, "level", 0.5f },
        },
    };

    const auto Updated = Schema {
        Schema::TypeIdType { 808 },
        "MegaDistortionAudioEffect",
        {
            { 0, "gain", 0.25f },
            { 1, "level", 0.5f },
            { 2, "tone", 0.5f },
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
            { 0, "gain", 0.25f },
            { 1, "level", 0.5f },
        },
    };

    const auto Updated = Schema {
        Schema::TypeIdType { 808 },
        "DistortionAudioEffect",
        {
            { 0, "gain", 0.25f },
            { 2, "tone", 0.5f },
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
            { 0, "gain", 0.25f },
            { 1, "level", 0.5f },
        },
    };

    const auto Updated = Schema {
        Schema::TypeIdType { 808 },
        "DistortionAudioEffect",
        {
            { 0, "gain", 0.5f },
            { 1, "level", 0.5f },
            { 2, "tone", 0.5f },
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
            { 0, "gain", 0.25f },
            { 1, "level", 0.5f },
        },
    };

    const auto Updated = Schema {
        Schema::TypeIdType { 808 },
        "DistortionAudioEffect",
        {
            { 0, "drive", 1.0f },
            { 1, "level", 0.5f },
            { 2, "tone", 0.5f },
        },
    };

    EXPECT_FALSE(csp::multiplayer::IsCompatible(BuiltIn, Updated));
}
