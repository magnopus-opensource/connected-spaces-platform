#if MULTIPLAYER
#include "CSP/Multiplayer/Components/ExternalLinkSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

using namespace oly_multiplayer;

#if RUN_ALL_UNIT_TESTS || RUN_REPLICATEDVALUE_TESTS
CSP_PUBLIC_TEST(OlympusEngine, SpaceComponentTests, ExternalLinkComponentProperties)
{
    std::shared_ptr<SpaceEntity> TestSpaceEntity(new SpaceEntity());
    std::shared_ptr<ExternalLinkSpaceComponent> TestExternalLinkComponent(new ExternalLinkSpaceComponent(TestSpaceEntity.get()));

    EXPECT_NE(TestExternalLinkComponent, nullptr);
    EXPECT_EQ(TestExternalLinkComponent->GetType(), ComponentType::ExternalLink);

    // Name setter / getter validiation
    {
        const std::string ExpectedValue("Name");
        TestExternalLinkComponent->SetName(ExpectedValue.c_str());
        const std::string Result(TestExternalLinkComponent->GetName());
        EXPECT_EQ(ExpectedValue, Result);
    }

    // DisplayText setter / getter validation
    {
        const std::string ExpectedValue("DisplayText");
        TestExternalLinkComponent->SetDisplayText(ExpectedValue.c_str());
        const std::string Result(TestExternalLinkComponent->GetDisplayText());
        EXPECT_EQ(ExpectedValue, Result);
    }

    // LinkURL setter / getter validation
    {
        const std::string ExpectedValue("URL");
        TestExternalLinkComponent->SetLinkUrl(ExpectedValue.c_str());
        const std::string Result(TestExternalLinkComponent->GetLinkUrl());
        EXPECT_EQ(ExpectedValue, Result);
    }

    // Position setter / getter validation
    {
        const oly_common::Vector3 ExpectedValue(1.0f, 2.0f, 3.0f);
        TestExternalLinkComponent->SetPosition(ExpectedValue);
        const oly_common::Vector3 Result(TestExternalLinkComponent->GetPosition());
        EXPECT_EQ(ExpectedValue, Result);
    }

    // Rotation setter / getter validation
    {
        const oly_common::Vector4 ExpectedValue(1.0f, 2.0f, 3.0f, 4.0f);
        TestExternalLinkComponent->SetRotation(ExpectedValue);
        const oly_common::Vector4 Result(TestExternalLinkComponent->GetRotation());
        EXPECT_EQ(ExpectedValue, Result);
    }

    // Scale setter / getter validation
    {
        const oly_common::Vector3 ExpectedValue(1.0f, 2.0f, 3.0f);
        TestExternalLinkComponent->SetScale(ExpectedValue);
        const oly_common::Vector3 Result(TestExternalLinkComponent->GetScale());
        EXPECT_EQ(ExpectedValue, Result);
    }
}

#endif // RUN_ALL_UNIT_TESTS || RUN_REPLICATEDVALUE_TESTS

#endif // MULTIPLAYER