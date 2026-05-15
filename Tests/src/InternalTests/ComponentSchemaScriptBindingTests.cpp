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
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Script/ScriptSystem.h"

#include "quickjspp.hpp"

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

namespace
{

using Schema = csp::multiplayer::ComponentSchema;
using Property = csp::multiplayer::ComponentProperty;

class TestFixture final
{
public:
    TestFixture(const csp::common::Array<Schema>& additionalComponents)
        : m_scriptSystem(csp::systems::ScriptSystem::MakeInitialised())
        , m_engine(m_logSystem, *m_scriptSystem, additionalComponents)
    {
        m_logSystem.SetSystemLevel(csp::common::LogLevel::VeryVerbose);
        m_logSystem.SetLogCallback([](auto, const auto& message) { std::cerr << message.c_str() << std::endl; });
    }

    struct Entity final
    {
        struct Component : public csp::multiplayer::ComponentBase
        {
            using ComponentBase::ComponentBase;
            using ComponentBase::GetProperty;
            using ComponentBase::SetProperty;
        };

        struct ComponentCreationArgs final
        {
            using ComponentTypeId = Schema::TypeIdType;
            using InstanceCount = uint16_t;

            ComponentTypeId TypeId;
            InstanceCount Count;
        };

        static Entity Make(TestFixture& fixture, const csp::common::String& name, std::vector<ComponentCreationArgs> componentsToAdd)
        {
            // ownership: The engine is supposed to own these, but it seems OfflineRealtimeEngine is currently leaking them.
            auto* entity
                = std::get<0>(AWAIT(&fixture.m_engine, CreateEntity, name, csp::multiplayer::SpaceTransform {}, csp::common::Optional<uint64_t> {}));

            const auto findSchema = [&](auto typeId) -> const Schema*
            {
                const auto& registry = fixture.m_engine.GetComponentSchemaRegistry()->GetUnderlying();
                if (const auto it = registry.find(typeId); it != registry.end())
                {
                    return &it->second;
                }

                return nullptr;
            };

            auto schemaComponents = std::vector<Component*>();

            auto schemaComponentKey = uint16_t { 0 };
            for (const auto& [TypeId, InstanceCount] : componentsToAdd)
            {
                if (const auto* schema = findSchema(TypeId))
                {
                    for (uint16_t count = 0; count < InstanceCount; ++count)
                    {
                        // This (mis)uses what is available via the public API to add a component to an entity with a given schema
                        // as there isn't currently a way of doing this via the public API.
                        auto componentToAdd = std::make_unique<Component>(*schema, &fixture.m_logSystem, entity);

                        schemaComponents.push_back(componentToAdd.get());

                        // ownership: SpaceEntity is presumably supposed to take ownership, but it currently leaks all components
                        entity->AddComponentDirect(schemaComponentKey++, componentToAdd.release());
                    }
                }
            }

            // ownership: As above, SpaceEntity is presumably intended to own its components, but currently it leaks them.
            auto* scriptComponent
                = static_cast<csp::multiplayer::ScriptSpaceComponent*>(entity->AddComponent(csp::multiplayer::ComponentType::ScriptData));

            return {
                *entity,
                *scriptComponent,
                std::move(schemaComponents),
            };
        }

        csp::multiplayer::SpaceEntity& Ref;
        csp::multiplayer::ScriptSpaceComponent& ScriptComponent;
        std::vector<Component*> SchemaComponents;
    };

    Entity MakeEntity(const csp::common::String& name, std::vector<Entity::ComponentCreationArgs> componentsToAdd)
    {
        return Entity::Make(*this, name, std::move(componentsToAdd));
    }

    struct ScriptResult final
    {
        struct Assertion final
        {
            bool Passed;
            std::string Explanation;
        };

        bool Completed = false;
        std::vector<Assertion> Assertions;

        bool Passed() const
        {
            return Completed && std::all_of(Assertions.begin(), Assertions.end(), [](const auto& assertion) { return assertion.Passed; });
        }

        operator ::testing::AssertionResult() const
        {
            if (Passed())
            {
                return ::testing::AssertionSuccess();
            }

            return ::testing::AssertionFailure() << "Script ran without errors: " << Completed
                                                 << ", Assertion failures: " << ::testing::PrintToString(AssertionFailures());
        }

        std::vector<std::string> AssertionFailures() const
        {
            auto explanations = std::vector<std::string>();

            for (const auto& assertion : Assertions)
            {
                if (!assertion.Passed)
                {
                    explanations.push_back(assertion.Explanation);
                }
            }

            return explanations;
        }
    };

    ScriptResult InvokeScript(Entity& entity, const csp::common::String& sourceText)
    {
        entity.ScriptComponent.SetScriptSource(sourceText);

        auto assertions = std::vector<ScriptResult::Assertion>();

        auto& module = *static_cast<qjs::Context::Module*>(m_scriptSystem->GetModule(entity.Ref.GetId(), "CSPTest"));
        module.function("assert", [&](bool passed, std::string explanation) { assertions.push_back({ passed, std::move(explanation) }); });

        const auto succeeded = entity.Ref.GetScript().Invoke();

        return {
            succeeded,
            std::move(assertions),
        };
    }

private:
    csp::common::LogSystem m_logSystem;
    std::shared_ptr<csp::systems::ScriptSystem> m_scriptSystem;
    csp::multiplayer::OfflineRealtimeEngine m_engine;
};

} // namespace

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaScriptBindingTests, GetterAvailableOnThisEntityForComponentType)
{
    auto fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "stringProperty",
                    "Value",
                },
            },
        },
    });

    auto entity = fixture.MakeEntity("Test Entity",
        {
            TestFixture::Entity::ComponentCreationArgs {
                Schema::TypeIdType { 123 },
                TestFixture::Entity::ComponentCreationArgs::InstanceCount { 2 },
            },
        });

    constexpr auto scriptText = R"(
        import { assert } from "CSPTest";

        const examples = ThisEntity.getExampleComponents();
        assert(examples.length == 2, "Should have 2 components");
    )";

    EXPECT_TRUE(fixture.InvokeScript(entity, scriptText));
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaScriptBindingTests, SupportedTypesGetterSetter)
{
    auto fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "boolProperty",
                    true,
                },
                {
                    1,
                    "floatProperty",
                    0.25f,
                },
                {
                    2,
                    "intProperty",
                    int64_t { 123 },
                },
                {
                    3,
                    "stringProperty",
                    "Test!",
                },
                {
                    4,
                    "vec2Property",
                    csp::common::Vector2(1.0f, 2.0f),
                },
                {
                    5,
                    "vec3Property",
                    csp::common::Vector3(1.0f, 2.0f, 3.0f),
                },
                {
                    6,
                    "vec4Property",
                    csp::common::Vector4(1.0f, 2.0f, 3.0f, 4.0f),
                },
            },
        },
    });

    auto entity = fixture.MakeEntity("Test Entity",
        {
            TestFixture::Entity::ComponentCreationArgs {
                Schema::TypeIdType { 123 },
                TestFixture::Entity::ComponentCreationArgs::InstanceCount { 1 },
            },
        });

    constexpr auto scriptText = R"(
        import { assert } from "CSPTest";

        assert(ThisEntity.getExampleComponents().length === 1, `Should have 1 component, ${ThisEntity.getExampleComponents().length}`);

        const example = ThisEntity.getExampleComponents()[0];
        assert(example.boolProperty === true, "Initial value of boolProperty === true");
        assert(example.floatProperty === 0.25, "Initial value of floatProperty === 0.25");
        assert(example.intProperty === 123, "Initial value of intProperty === 123");
        assert(example.stringProperty === "Test!", `Initial value of stringProperty === "Test!"`);

        const areEqual = (lhs, rhs) => {
            return lhs.length === rhs.length
                && lhs.every((value, index) => value === rhs[index]);
        };
    
        assert(areEqual(example.vec2Property, [1.0, 2.0]), "Initial value of vec2Property === [1.0, 2.0]");
        assert(areEqual(example.vec3Property, [1.0, 2.0, 3.0]), "Initial value of vec3Property === [1.0, 2.0, 3.0]");
        assert(areEqual(example.vec4Property, [1.0, 2.0, 3.0, 4.0]), "Initial value of vec4Property === [1.0, 2.0, 3.0, 4.0]");

        example.boolProperty = false;
        example.floatProperty = 0.5;
        example.intProperty = 1234;
        example.stringProperty = "Test JS!";
        example.vec2Property = [2, 3];
        example.vec3Property = [2, 3, 4];
        example.vec4Property = [2, 3, 4, 5];
    )";

    EXPECT_TRUE(fixture.InvokeScript(entity, scriptText));

    ASSERT_EQ(entity.SchemaComponents.size(), 1);

    const auto& component = *entity.SchemaComponents.front();
    EXPECT_EQ(component.GetProperty(0), false);
    EXPECT_EQ(component.GetProperty(1), 0.5f);
    EXPECT_EQ(component.GetProperty(2), static_cast<int64_t>(1234));
    EXPECT_EQ(component.GetProperty(3), "Test JS!");
    EXPECT_EQ(component.GetProperty(4), csp::common::Vector2(2.f, 3.f));
    EXPECT_EQ(component.GetProperty(5), csp::common::Vector3(2.f, 3.f, 4.f));
    EXPECT_EQ(component.GetProperty(6), csp::common::Vector4(2.f, 3.f, 4.f, 5.f));
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaScriptBindingTests, SchemaWithBothScriptableAndNonScriptableProperties)
{
    auto fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Mixed",
            {
                {
                    0,
                    "stringProperty",
                    "Hello",
                },
                {
                    1,
                    "unsupportedTypeProperty",
                    csp::common::Map<csp::common::String, csp::common::ReplicatedValue>(),
                },
                {
                    2,
                    {}, // Note: No property name
                    csp::common::Map<csp::common::String, csp::common::ReplicatedValue>(),
                },
                {
                    3,
                    "boolProperty",
                    true,
                },
            },
        },
    });

    auto entity = fixture.MakeEntity("Test Entity",
        {
            TestFixture::Entity::ComponentCreationArgs {
                Schema::TypeIdType { 123 },
                TestFixture::Entity::ComponentCreationArgs::InstanceCount { 1 },
            },
        });

    constexpr auto scriptText = R"(
        import { assert } from "CSPTest";

        const mixed = ThisEntity.getMixedComponents()[0];
    
        assert("stringProperty" in mixed, "stringProperty exists");
        assert(!("unsupportedTypeProperty" in mixed), "unsupportedTypeProperty does not exist");
        assert("boolProperty" in mixed, "boolProperty exists");
    )";

    EXPECT_TRUE(fixture.InvokeScript(entity, scriptText));
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaScriptBindingTests, CoercesOrThrowsWhenAssigningDifferentType)
{
    auto fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "boolProperty",
                    true,
                },
                {
                    1,
                    "floatProperty",
                    0.25f,
                },
                {
                    2,
                    "intProperty",
                    int64_t { 123 },
                },
                {
                    3,
                    "stringProperty",
                    "Test!",
                },
                {
                    4,
                    "vec2Property",
                    csp::common::Vector2(1.0f, 2.0f),
                },
                {
                    5,
                    "vec3Property",
                    csp::common::Vector3(1.0f, 2.0f, 3.0f),
                },
                {
                    6,
                    "vec4Property",
                    csp::common::Vector4(1.0f, 2.0f, 3.0f, 4.0f),
                },
            },
        },
    });

    auto entity = fixture.MakeEntity("Test Entity",
        {
            TestFixture::Entity::ComponentCreationArgs {
                Schema::TypeIdType { 123 },
                TestFixture::Entity::ComponentCreationArgs::InstanceCount { 1 },
            },
        });

    constexpr auto scriptText = R"(
        import { assert } from "CSPTest";

        const example = ThisEntity.getExampleComponents()[0];

        const throws = (fn) => {
            try {
                fn();
            } catch (e) {
                return true;
            }

            return false;
        };

        assert(!throws(() => example.boolProperty = ""), "Setting boolProperty to different type applies JS type conversion rules");
        assert(!throws(() => example.floatProperty = ""), "Setting floatProperty to different type applies JS type conversion rules");
        assert(!throws(() => example.intProperty = ""), "Setting intProperty to applies JS type conversion rules");
        assert(!throws(() => example.stringProperty = false), "Setting stringProperty to applies JS type conversion rules");
        assert(throws(() => example.vec2Property = ""), "Setting vec2Property to different type should throw");
        assert(throws(() => example.vec3Property = ""), "Setting vec3Property to different type should throw");
        assert(throws(() => example.vec4Property = ""), "Setting vec4Property to different type should throw");
    )";

    EXPECT_TRUE(fixture.InvokeScript(entity, scriptText));

    ASSERT_EQ(entity.SchemaComponents.size(), 1);

    const auto& component = *entity.SchemaComponents.front();
    EXPECT_EQ(component.GetProperty(0).GetReplicatedValueType(), csp::common::ReplicatedValueType::Boolean);
    EXPECT_EQ(component.GetProperty(1).GetReplicatedValueType(), csp::common::ReplicatedValueType::Float);
    EXPECT_EQ(component.GetProperty(2).GetReplicatedValueType(), csp::common::ReplicatedValueType::Integer);
    EXPECT_EQ(component.GetProperty(3).GetReplicatedValueType(), csp::common::ReplicatedValueType::String);
    EXPECT_EQ(component.GetProperty(4).GetReplicatedValueType(), csp::common::ReplicatedValueType::Vector2);
    EXPECT_EQ(component.GetProperty(5).GetReplicatedValueType(), csp::common::ReplicatedValueType::Vector3);
    EXPECT_EQ(component.GetProperty(6).GetReplicatedValueType(), csp::common::ReplicatedValueType::Vector4);

    // Note: we might consider changing the setters to always prevent setting the value when the type
    // differs, but currently we get the default quickjspp behaviour, which is consistent with our
    // traditional manual bindings. Currently, the type is preserved (as tested by the expectations
    // above), but for primitive/built-in types we get JS type conversion, which can result in
    // things like getting a string with the value "false", or ending up with NaN etc.
    EXPECT_EQ(component.GetProperty(0), false);
    EXPECT_EQ(component.GetProperty(1), 0.f);
    EXPECT_EQ(component.GetProperty(2), static_cast<int64_t>(0));
    EXPECT_EQ(component.GetProperty(3), "false");
    EXPECT_EQ(component.GetProperty(4), csp::common::Vector2(1.f, 2.f));
    EXPECT_EQ(component.GetProperty(5), csp::common::Vector3(1.f, 2.f, 3.f));
    EXPECT_EQ(component.GetProperty(6), csp::common::Vector4(1.f, 2.f, 3.f, 4.f));
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaScriptBindingTests, ComponentsOfSameTypeHaveSamePrototype)
{
    // Note: There are a couple of reasons for testing this:
    // 1. The main reason is for consistency with the traditional manual bindings, in case any
    //    script depends on this (i.e. Hyrum's Law).
    // 2. As a consequence of the first point, we end up having to maintain a cache of prototypes,
    //    so this exercises that code

    auto fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "stringProperty",
                    "Value",
                },
            },
        },
    });

    auto entity = fixture.MakeEntity("Test Entity",
        {
            TestFixture::Entity::ComponentCreationArgs {
                Schema::TypeIdType { 123 },
                TestFixture::Entity::ComponentCreationArgs::InstanceCount { 2 },
            },
        });

    constexpr auto scriptText = R"(
        import { assert } from "CSPTest";

        const examples = ThisEntity.getExampleComponents();
        const one = examples[0];
        const two = examples[1];
        assert(Object.getPrototypeOf(one) === Object.getPrototypeOf(two), "Component instances have same prototype");
    )";

    EXPECT_TRUE(fixture.InvokeScript(entity, scriptText));
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaScriptBindingTests, PropertiesAreEnumerable)
{
    // Note: This is solely for consistency with the traditional manual bindings, in case any
    // script depends on this (i.e. Hyrum's Law).

    auto fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "stringProperty",
                    "Value",
                },
            },
        },
    });

    auto entity = fixture.MakeEntity("Test Entity",
        {
            TestFixture::Entity::ComponentCreationArgs {
                Schema::TypeIdType { 123 },
                TestFixture::Entity::ComponentCreationArgs::InstanceCount { 1 },
            },
        });

    constexpr auto scriptText = R"(
        import { assert } from "CSPTest";

        const example = ThisEntity.getExampleComponents()[0];

        const enumerable = [];
        for (const key in example) {
            enumerable.push(key);
        }
        assert(enumerable.includes("stringProperty"), "Exposed properties should be enumerable");
    )";

    EXPECT_TRUE(fixture.InvokeScript(entity, scriptText));
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaScriptBindingTests, BaseClassFunctionalityRemainsAccessible)
{
    auto fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    1,
                    "stringProperty",
                    "Value",
                },
            },
        },
    });

    auto entity = fixture.MakeEntity("Test Entity",
        {
            TestFixture::Entity::ComponentCreationArgs {
                Schema::TypeIdType { 123 },
                TestFixture::Entity::ComponentCreationArgs::InstanceCount { 1 },
            },
        });

    ASSERT_EQ(entity.SchemaComponents.size(), 1);

    auto& component = *entity.SchemaComponents.front();
    component.SetComponentName("Test Component");

    ASSERT_EQ(component.GetId(), static_cast<Property::KeyType>(0));
    ASSERT_EQ(static_cast<Schema::TypeIdType>(component.GetComponentType()), static_cast<Schema::TypeIdType>(123));

    constexpr auto scriptText = R"(
        import { assert } from "CSPTest";

        const example = ThisEntity.getExampleComponents()[0];

        globalThis.onStringPropertyChanged = () => {
            example.invokeAction("ScriptMessage", "Success");
        };
    
        ThisEntity.subscribeToMessage("onStringPropertyChanged", "onStringPropertyChanged");

        example.subscribeToPropertyChange(1, "onStringPropertyChanged");

        assert(example.id === 0, "example.id === 0");
        assert(example.type === 123, "example.type === 123");
        assert(example.name === "Test Component", "example.name === 'Test Component'");
    )";

    auto maybeScriptMessage = std::optional<std::string>();

    component.RegisterActionHandler("ScriptMessage", [&](auto*, auto&, const auto& message) { maybeScriptMessage = std::string(message.c_str()); });

    EXPECT_TRUE(fixture.InvokeScript(entity, scriptText));

    component.SetProperty(1, "Changed");

    EXPECT_EQ(maybeScriptMessage, "Success");
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaScriptBindingTests, MultipleComponentInstancesMaintainUniqueState)
{
    auto fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "stringProperty",
                    "",
                },
            },
        },
    });

    auto entity = fixture.MakeEntity("Test Entity",
        {
            TestFixture::Entity::ComponentCreationArgs {
                Schema::TypeIdType { 123 },
                TestFixture::Entity::ComponentCreationArgs::InstanceCount { 2 },
            },
        });

    ASSERT_EQ(entity.SchemaComponents.size(), 2);

    auto& one = *entity.SchemaComponents[0];
    auto& two = *entity.SchemaComponents[1];

    one.SetProperty(0, "One");
    two.SetProperty(0, "Two");

    constexpr auto scriptText = R"(
        import { assert } from "CSPTest";

        const examples = ThisEntity.getExampleComponents();

        const one = examples[0];
        const two = examples[1];

        assert(one.stringProperty === "One", "one.stringProperty === 'One'");
        assert(two.stringProperty === "Two", "one.stringProperty === 'Two'");

        one.stringProperty = "1";
        two.stringProperty = "2";
    )";

    EXPECT_TRUE(fixture.InvokeScript(entity, scriptText));
    EXPECT_EQ(one.GetProperty(0), "1");
    EXPECT_EQ(two.GetProperty(0), "2");
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaScriptBindingTests, MultipleEntityInstancesMaintainUniqueState)
{
    auto fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "Example",
            {
                {
                    0,
                    "stringProperty",
                    "",
                },
            },
        },
    });

    auto entityOne = fixture.MakeEntity("Test Entity",
        {
            TestFixture::Entity::ComponentCreationArgs {
                Schema::TypeIdType { 123 },
                TestFixture::Entity::ComponentCreationArgs::InstanceCount { 1 },
            },
        });

    EXPECT_TRUE(fixture.InvokeScript(entityOne, R"(
        import { assert } from "CSPTest";

        const example = ThisEntity.getExampleComponents()[0];

        example.stringProperty = "1";
    )"));

    const auto& entityOneComponent = *entityOne.SchemaComponents.front();
    EXPECT_EQ(entityOneComponent.GetProperty(0), "1");

    auto entityTwo = fixture.MakeEntity("Test Entity",
        {
            TestFixture::Entity::ComponentCreationArgs {
                Schema::TypeIdType { 123 },
                TestFixture::Entity::ComponentCreationArgs::InstanceCount { 1 },
            },
        });
    
    const auto& entityTwoComponent = *entityTwo.SchemaComponents.front();

    ASSERT_EQ(entityTwoComponent.GetProperty(0), "");

    EXPECT_TRUE(fixture.InvokeScript(entityTwo, R"(
        import { assert } from "CSPTest";

        const example = ThisEntity.getExampleComponents()[0];

        example.stringProperty = "2";
    )"));

    EXPECT_EQ(entityTwoComponent.GetProperty(0), "2");

    EXPECT_TRUE(fixture.InvokeScript(entityOne, R"(
        import { assert } from "CSPTest";

        const example = ThisEntity.getExampleComponents()[0];

        assert(example.stringProperty === "1", "first entity remains unchanged");
    )"));
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaScriptBindingTests, MultipleScriptableComponentTypesCanBeRegistered)
{
    auto fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "String",
            {
                {
                    0,
                    "value",
                    "Test",
                },
            },
        },
        Schema {
            Schema::TypeIdType { 321 },
            "Int",
            {
                {
                    0,
                    "value",
                    int64_t { 616 },
                },
            },
        },
    });

    auto entity = fixture.MakeEntity("Test Entity",
        {
            TestFixture::Entity::ComponentCreationArgs {
                Schema::TypeIdType { 123 },
                TestFixture::Entity::ComponentCreationArgs::InstanceCount { 1 },
            },
            TestFixture::Entity::ComponentCreationArgs {
                Schema::TypeIdType { 321 },
                TestFixture::Entity::ComponentCreationArgs::InstanceCount { 1 },
            },
        });

    ASSERT_EQ(entity.SchemaComponents.size(), 2);

    constexpr auto scriptText = R"(
        import { assert } from "CSPTest";

        const strings = ThisEntity.getStringComponents();
        const ints = ThisEntity.getIntComponents();

        assert(strings.length === 1, "strings.length === 1");
        assert(ints.length === 1, "ints.length === 1");

        assert(strings[0].value === "Test", "strings[0].value === 'Test'");
        assert(ints[0].value === 616, "ints[0].value === 616");
    )";

    EXPECT_TRUE(fixture.InvokeScript(entity, scriptText));
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaScriptBindingTests, MultipleSchemaComponentsOneNonScriptable)
{
    auto fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            {}, // Note: Non scriptable
            {
                {
                    0,
                    {}, // Note: Non scriptable
                    "Non Scriptable",
                },
            },
        },
        Schema {
            Schema::TypeIdType { 321 },
            "Scriptable",
            {
                {
                    0,
                    "value",
                    "Hello",
                },
            },
        },
    });

    auto entity = fixture.MakeEntity("Test Entity",
        {
            TestFixture::Entity::ComponentCreationArgs {
                Schema::TypeIdType { 123 },
                TestFixture::Entity::ComponentCreationArgs::InstanceCount { 1 },
            },
            TestFixture::Entity::ComponentCreationArgs {
                Schema::TypeIdType { 321 },
                TestFixture::Entity::ComponentCreationArgs::InstanceCount { 1 },
            },
        });

    // Test script ensures that getScriptableComponents still gets registered despite a
    // non-scriptable component preceeding it during engine registration
    constexpr auto scriptText = R"(
        import { assert } from "CSPTest";

        assert(typeof ThisEntity.getScriptableComponents === "function", "Scriptable component getter exists");
    )";

    EXPECT_TRUE(fixture.InvokeScript(entity, scriptText));
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaScriptBindingTests, BindingsAfterSourceChange)
{
    auto fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "String",
            {
                {
                    0,
                    "value",
                    "Value",
                },
            },
        },
    });

    auto entity = fixture.MakeEntity("Test Entity",
        {
            TestFixture::Entity::ComponentCreationArgs {
                Schema::TypeIdType { 123 },
                TestFixture::Entity::ComponentCreationArgs::InstanceCount { 1 },
            },
        });

    {
        constexpr auto scriptText = R"(
            import { assert } from "CSPTest";

            const string = ThisEntity.getStringComponents()[0];
            assert(string.value === "Value", "string.value === 'Value'");

            string.value = "JS";
        )";

        EXPECT_TRUE(fixture.InvokeScript(entity, scriptText));
    }

    {
        constexpr auto scriptText = R"(
            import { assert } from "CSPTest";

            const string = ThisEntity.getStringComponents()[0];
            assert(string.value === "JS", "string.value === 'JS'");
        )";

        EXPECT_TRUE(fixture.InvokeScript(entity, scriptText));
    }
}

CSP_INTERNAL_TEST(CSPEngine, ComponentSchemaScriptBindingTests, BindingsRegisteredRegardlessOfEntityState)
{
    auto fixture = TestFixture({
        Schema {
            Schema::TypeIdType { 123 },
            "String",
            {
                {
                    0,
                    "value",
                    "Value",
                },
            },
        },
    });

    auto entity = fixture.MakeEntity("Test Entity", {});

    constexpr auto scriptText = R"(
        import { assert } from "CSPTest";

        assert(ThisEntity.getStringComponents().length === 0, "getter should exist and return length 0");
    )";

    EXPECT_TRUE(fixture.InvokeScript(entity, scriptText));
}
