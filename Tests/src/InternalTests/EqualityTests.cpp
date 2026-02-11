/*
 * Copyright 2026 Magnopus LLC
 *
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

#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Multiplayer/PatchTypes.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/ECommerce/ECommerce.h"
#include "CSP/Systems/EventTicketing/EventTicketing.h"
#include "CSP/Systems/HotspotSequence/HotspotGroup.h"
#include "CSP/Systems/Multiplayer/Scope.h"
#include "CSP/Systems/Quota/Quota.h"
#include "CSP/Systems/Sequence/Sequence.h"
#include "CSP/Systems/ServiceStatus.h"
#include "CSP/Systems/Spaces/Site.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Spaces/UserRoles.h"
#include "CSP/Systems/Spatial/Anchor.h"
#include "CSP/Systems/Spatial/PointOfInterest.h"
#include "CSP/Systems/Spatial/SpatialDataTypes.h"
#include "CSP/Systems/Users/Profile.h"

#include "TestHelpers.h"

#include <gtest/gtest.h>

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, MessageInfoEqualityTest)
{
    using namespace csp::multiplayer;

    MessageInfo A;
    A.MessageId = "msg1";
    MessageInfo B = A;

    ASSERT_TRUE(A == B);
    B.MessageId = "msg2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, ComponentUpdateInfoEqualityTest)
{
    using namespace csp::multiplayer;

    ComponentUpdateInfo A;
    A.ComponentId = 1;
    A.UpdateType = ComponentUpdateType::Add;
    ComponentUpdateInfo B = A;

    ASSERT_TRUE(A == B);
    B.ComponentId = 2;
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, AssetEqualityTest)
{
    using namespace csp::systems;

    Asset A;
    A.Id = "asset1";
    Asset B = A;

    ASSERT_TRUE(A == B);
    B.Id = "asset2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, AssetCollectionEqualityTest)
{
    using namespace csp::systems;

    AssetCollection A;
    A.Id = "col1";
    AssetCollection B = A;

    ASSERT_TRUE(A == B);
    B.Id = "col2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, CurrencyInfoEqualityTest)
{
    using namespace csp::systems;

    CurrencyInfo A;
    A.CurrencyCode = "USD";
    CurrencyInfo B = A;

    ASSERT_TRUE(A == B);
    B.CurrencyCode = "EUR";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, ProductMediaInfoEqualityTest)
{
    using namespace csp::systems;

    ProductMediaInfo A;
    A.Url = "http://example.com";
    ProductMediaInfo B = A;

    ASSERT_TRUE(A == B);
    B.Url = "http://other.com";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, VariantOptionInfoEqualityTest)
{
    using namespace csp::systems;

    VariantOptionInfo A;
    A.Name = "Color";
    VariantOptionInfo B = A;

    ASSERT_TRUE(A == B);
    B.Name = "Size";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, ProductVariantInfoEqualityTest)
{
    using namespace csp::systems;

    ProductVariantInfo A;
    A.Id = "var1";
    ProductVariantInfo B = A;

    ASSERT_TRUE(A == B);
    B.Id = "var2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, ProductInfoEqualityTest)
{
    using namespace csp::systems;

    ProductInfo A;
    A.Id = "prod1";
    ProductInfo B = A;

    ASSERT_TRUE(A == B);
    B.Id = "prod2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, CartLineEqualityTest)
{
    using namespace csp::systems;

    CartLine A;
    A.CartLineId = "line1";
    CartLine B = A;

    ASSERT_TRUE(A == B);
    B.CartLineId = "line2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, ShopifyStoreInfoEqualityTest)
{
    using namespace csp::systems;

    ShopifyStoreInfo A;
    A.StoreId = "store1";
    ShopifyStoreInfo B = A;

    ASSERT_TRUE(A == B);
    B.StoreId = "store2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, TicketedEventEqualityTest)
{
    using namespace csp::systems;

    TicketedEvent A;
    A.Id = "event1";
    TicketedEvent B = A;

    ASSERT_TRUE(A == B);
    B.Id = "event2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, HotspotGroupEqualityTest)
{
    using namespace csp::systems;

    HotspotGroup A;
    A.Name = "group1";
    HotspotGroup B = A;

    ASSERT_TRUE(A == B);
    B.Name = "group2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, ScopeEqualityTest)
{
    using namespace csp::systems;

    Scope A;
    A.Id = "scope1";
    A.PubSubType = PubSubModelType::Global;
    Scope B = A;

    ASSERT_TRUE(A == B);
    B.Id = "scope2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, FeatureLimitInfoEqualityTest)
{
    using namespace csp::systems;

    FeatureLimitInfo A;
    A.FeatureName = TierFeatures::Agora;
    FeatureLimitInfo B = A;

    ASSERT_TRUE(A == B);
    B.FeatureName = TierFeatures::Shopify;
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, UserTierInfoEqualityTest)
{
    using namespace csp::systems;

    UserTierInfo A;
    A.AssignToId = "user1";
    A.TierName = TierNames::Basic;
    UserTierInfo B = A;

    ASSERT_TRUE(A == B);
    B.AssignToId = "user2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, FeatureQuotaInfoEqualityTest)
{
    using namespace csp::systems;

    FeatureQuotaInfo A;
    A.FeatureName = TierFeatures::Agora;
    A.TierName = TierNames::Basic;
    A.Period = PeriodEnum::Total;
    FeatureQuotaInfo B = A;

    ASSERT_TRUE(A == B);
    B.TierName = TierNames::Premium;
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, SequenceEqualityTest)
{
    using namespace csp::systems;

    Sequence A;
    A.Key = "key1";
    Sequence B = A;

    ASSERT_TRUE(A == B);
    B.Key = "key2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, VersionMetadataEqualityTest)
{
    using namespace csp::systems;

    VersionMetadata A;
    A.Version = "v1";
    VersionMetadata B = A;

    ASSERT_TRUE(A == B);
    B.Version = "v2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, ServiceStatusEqualityTest)
{
    using namespace csp::systems;

    ServiceStatus A;
    A.Name = "UserService";
    ServiceStatus B = A;

    ASSERT_TRUE(A == B);
    B.Name = "SpaceService";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, ServicesDeploymentStatusEqualityTest)
{
    using namespace csp::systems;

    ServicesDeploymentStatus A;
    A.Version = "1.0.0";
    ServicesDeploymentStatus B = A;

    ASSERT_TRUE(A == B);
    B.Version = "2.0.0";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, SiteEqualityTest)
{
    using namespace csp::systems;

    Site A;
    A.Id = "site1";
    A.Location.Latitude = 0.0;
    A.Location.Longitude = 0.0;
    A.Rotation.X = 0.0;
    A.Rotation.Y = 0.0;
    A.Rotation.Z = 0.0;
    A.Rotation.W = 1.0;
    Site B = A;

    ASSERT_TRUE(A == B);
    B.Id = "site2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, BasicSpaceEqualityTest)
{
    using namespace csp::systems;

    BasicSpace A;
    A.Id = "space1";
    A.Attributes = SpaceAttributes::None;
    BasicSpace B = A;

    ASSERT_TRUE(A == B);
    B.Id = "space2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, SpaceEqualityTest)
{
    using namespace csp::systems;

    Space A;
    A.Id = "space1";
    A.Attributes = SpaceAttributes::None;
    Space B = A;

    ASSERT_TRUE(A == B);
    B.OwnerId = "owner2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, UserRoleInfoEqualityTest)
{
    using namespace csp::systems;

    UserRoleInfo A;
    A.UserId = "user1";
    A.UserRole = SpaceUserRole::User;
    UserRoleInfo B = A;

    ASSERT_TRUE(A == B);
    B.UserId = "user2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, InviteUserRoleInfoEqualityTest)
{
    using namespace csp::systems;

    InviteUserRoleInfo A;
    A.UserEmail = "a@b.com";
    A.UserRole = SpaceUserRole::User;
    InviteUserRoleInfo B = A;

    ASSERT_TRUE(A == B);
    B.UserEmail = "c@d.com";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, OlyAnchorPositionEqualityTest)
{
    using namespace csp::systems;

    OlyAnchorPosition A(1.0, 2.0, 3.0);
    OlyAnchorPosition B = A;

    ASSERT_TRUE(A == B);
    B.X = 4.0;
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, OlyRotationEqualityTest)
{
    using namespace csp::systems;

    OlyRotation A;
    A.X = 0.0;
    A.Y = 0.0;
    A.Z = 0.0;
    A.W = 1.0;
    OlyRotation B = A;

    ASSERT_TRUE(A == B);
    B.W = 0.0;
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, AnchorEqualityTest)
{
    using namespace csp::systems;

    Anchor A;
    A.Id = "anchor1";
    A.ThirdPartyAnchorProvider = AnchorProvider::GoogleCloudAnchors;
    A.SpaceEntityId = 0;
    A.Location.Latitude = 0.0;
    A.Location.Longitude = 0.0;
    Anchor B = A;

    ASSERT_TRUE(A == B);
    B.Id = "anchor2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, AnchorResolutionEqualityTest)
{
    using namespace csp::systems;

    AnchorResolution A;
    A.Id = "res1";
    A.SuccessfullyResolved = true;
    A.ResolveAttempted = 1;
    A.ResolveTime = 0.5;
    AnchorResolution B = A;

    ASSERT_TRUE(A == B);
    B.Id = "res2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, PointOfInterestEqualityTest)
{
    using namespace csp::systems;

    PointOfInterest A;
    A.Id = "poi1";
    A.Type = EPointOfInterestType::DEFAULT;
    A.Location.Latitude = 0.0;
    A.Location.Longitude = 0.0;
    PointOfInterest B = A;

    ASSERT_TRUE(A == B);
    B.Id = "poi2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, BasicProfileEqualityTest)
{
    using namespace csp::systems;

    BasicProfile A;
    A.UserId = "user1";
    BasicProfile B = A;

    ASSERT_TRUE(A == B);
    B.UserId = "user2";
    ASSERT_FALSE(A == B);
}

CSP_INTERNAL_TEST(CSPEngine, EqualityTests, ProfileEqualityTest)
{
    using namespace csp::systems;

    Profile A;
    A.UserId = "user1";
    Profile B = A;

    ASSERT_TRUE(A == B);
    B.Email = "different@email.com";
    ASSERT_FALSE(A == B);
}
