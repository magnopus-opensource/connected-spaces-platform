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

#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Systems/ECommerce/ECommerceSystem.h"
#include "CSP/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>

using namespace csp::systems;

// These tests currently require manual steps and will be reviewed as part of OF-1535.

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

csp::common::Map<csp::common::String, csp::common::String> GetShopifyDetails()
{
    if (!std::filesystem::exists("ShopifyCreds.txt"))
    {
        LogFatal("ShopifyCreds.txt not found! This file must exist and must contain a minimum of the following information:\nSpaceId "
                 "<SpaceId>\nProductId <ProductId> but may also need \nProductId <ProductId>\nCartId <CartId>\n");
    }

    csp::common::Map<csp::common::String, csp::common::String> OutMap;

    std::ifstream CredsFile;
    CredsFile.open("ShopifyCreds.txt");
    std::string Key, Value;
    while (CredsFile >> Key >> Value)
    {
        OutMap[Key.c_str()] = Value.c_str();
    }

    return OutMap;
}

/*These test are currently internal tests because they utilise that is currently only available
through internal CSP infrastructure.*/

#if RUN_ALL_UNIT_TESTS || RUN_ECOMMERCE_TESTS || RUN_ECOMMERCE_GET_PRODUCT_INFORMATION_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, ECommerceSystemTests, GetProductInformationTest)
{
    /*Steps needed to be performed before running this test are:

    1. Create a space (Add to Shopify Creds)
    2. Connected your shopify.dev account to your space using the "Private Access Token" and store name
            Endpoint : /api/v1/spaces/{spaceId}/vendors/shopify
            {
                    "storeName": "string",
                    "isEcommerceActive": true,
                    "privateAccessToken": "string"
            }
    3. Check Shopify has synced with your namespace
            Endpoint: /api/v1/vendors/shopify/validate
            {
                    "storeName": "string",
                    "privateAccessToken": "string"
            }
    4. Either use the default "Gift Card" product or update these test variables with a new product. (Add product Id to Shopify Creds)
    Now you can use this test!*/
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ECommerceSystem = SystemsManager.GetECommerceSystem();

    // This is an example from Shopify dev quickstart "Gift Card"
    const csp::common::String ProductId = "gid://shopify/Product/8660541047057";
    const csp::common::String ProductTitle = "Gift Card";
    const csp::common::String ProductDescription = "This is a gift card for the store";
    const csp::common::String ImageMediaContentType = "IMAGE";
    const csp::common::String ImageAlt = "Gift card that shows text: Generated data gift card";
    const csp::common::String ImageUrl = "https://cdn.shopify.com/s/files/1/0803/6070/2225/products/gift_card.png?v=1691076851";
    const int32_t ImageWidth = 2881;
    const int32_t ImageHeight = 2881;
    const int32_t VariantSize = 4;
    const int32_t MediaSize = 1;
    const int32_t OptionsSize = 1;
    const csp::common::String OptionsName = "Denominations";
    csp::common::Array<csp::common::String> VariantTitleAndOptionValue = { "$10", "$25", "$50", "$100" };
    csp::common::Array<csp::common::String> VariantIds = { "gid://shopify/ProductVariant/46314311516433",
        "gid://shopify/ProductVariant/46314311647505", "gid://shopify/ProductVariant/46314311745809", "gid://shopify/ProductVariant/46314311844113" };
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);
    auto Details = GetShopifyDetails();

    auto [Result] = AWAIT_PRE(ECommerceSystem, GetProductInformation, RequestPredicate, Details["SpaceId"], Details["ProductId"]);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result.GetProductInfo().Id, ProductId);
    EXPECT_EQ(Result.GetProductInfo().Title, ProductTitle);
    EXPECT_EQ(Result.GetProductInfo().Description, ProductDescription);
    EXPECT_EQ(Result.GetProductInfo().Tags.Size(), 0);

    EXPECT_EQ(Result.GetProductInfo().Media.Size(), MediaSize);

    for (int i = 0; i < Result.GetProductInfo().Media.Size(); ++i)
    {
        EXPECT_EQ(Result.GetProductInfo().Media[i].MediaContentType, ImageMediaContentType);
        EXPECT_EQ(Result.GetProductInfo().Media[i].Url, ImageUrl);
        EXPECT_EQ(Result.GetProductInfo().Media[i].Alt, ImageAlt);
        EXPECT_EQ(Result.GetProductInfo().Media[i].Width, ImageWidth);
        EXPECT_EQ(Result.GetProductInfo().Media[i].Height, ImageHeight);
    }
    EXPECT_EQ(Result.GetProductInfo().Variants.Size(), VariantSize);

    for (int i = 0; i < Result.GetProductInfo().Variants.Size(); ++i)
    {
        EXPECT_EQ(Result.GetProductInfo().Variants[i].Id, VariantIds[i]);
        EXPECT_EQ(Result.GetProductInfo().Variants[i].Title, VariantTitleAndOptionValue[i]);
        EXPECT_EQ(Result.GetProductInfo().Variants[i].AvailableForSale, true);
        EXPECT_EQ(Result.GetProductInfo().Variants[i].Media.MediaContentType, "");
        EXPECT_EQ(Result.GetProductInfo().Variants[i].Media.Alt, ImageAlt);
        EXPECT_EQ(Result.GetProductInfo().Variants[i].Media.Url, ImageUrl);
        EXPECT_EQ(Result.GetProductInfo().Variants[i].Media.Width, ImageWidth);
        EXPECT_EQ(Result.GetProductInfo().Variants[i].Media.Height, ImageHeight);

        EXPECT_EQ(Result.GetProductInfo().Variants[i].Options.Size(), OptionsSize);

        for (int n = 0; n < Result.GetProductInfo().Variants[i].Options.Size(); ++n)
        {
            EXPECT_EQ(Result.GetProductInfo().Variants[i].Options[n].Name, OptionsName);
            EXPECT_EQ(Result.GetProductInfo().Variants[i].Options[n].Value, VariantTitleAndOptionValue[i]);
        }

        EXPECT_EQ(Result.GetProductInfo().Variants[i].UnitPrice.Amount, 0);
        EXPECT_EQ(Result.GetProductInfo().Variants[i].UnitPrice.CurrencyCode, "");
    }

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ECOMMERCE_TESTS || RUN_ECOMMERCE_GET_PRODUCT_INFORMATION_BY_VARIANT_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, ECommerceSystemTests, GetProductInformationByVariantTest)
{
    /*Steps needed to be performed before running this test are:

    1. Create a space (Add to Shopify Creds)
    this kind of test.
    2. Connected your shopify.dev account to your space using the "Private Access Token" and store name
            Endpoint : /api/v1/spaces/{spaceId}/vendors/shopify
            {
                    "storeName": "string",
                    "isEcommerceActive": true,
                    "privateAccessToken": "string"
            }
    3. Check Shopify has synced with your namespace
            Endpoint: /api/v1/vendors/shopify/validate
            {
                    "storeName": "string",
                    "privateAccessToken": "string"
            }
    4. Either use the default "Gift Card" product or update these test variables with a new product. (Add variant Id to Shopify Creds)
    Now you can use this test!*/
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ECommerceSystem = SystemsManager.GetECommerceSystem();

    // This is an example from Shopify dev quickstart "Gift Card"
    const csp::common::String ProductId = "gid://shopify/Product/8566195847465";
    const csp::common::String ProductTitle = "Gift Card";
    const csp::common::String ProductDescription = "This is a gift card for the store";
    const csp::common::String ImageMediaContentType = "IMAGE";
    const csp::common::String ImageAlt = "Gift card that shows text: Generated data gift card";
    const csp::common::String ImageUrl = "https://cdn.shopify.com/s/files/1/0813/7238/1481/products/gift_card.png?v=1692877145";
    const int32_t ImageWidth = 2881;
    const int32_t ImageHeight = 2881;
    const int32_t VariantSize = 1;
    const int32_t MediaSize = 1;
    const int32_t OptionsSize = 0;
    const csp::common::String OptionsName = "Denominations";
    csp::common::Array<csp::common::String> VariantTitleAndOptionValue = { "$10", "$25", "$50", "$100" };
    csp::common::Array<csp::common::String> VariantIds = { "gid://shopify/ProductVariant/46375586136361",
        "gid://shopify/ProductVariant/46375586234665", "gid://shopify/ProductVariant/46375586398505", "gid://shopify/ProductVariant/46375586496809" };
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);
    auto Details = GetShopifyDetails();

    auto [Result] = AWAIT_PRE(ECommerceSystem, GetProductInfoCollectionByVariantIds, RequestPredicate, Details["SpaceId"], { Details["VariantId"] });
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_GT(Result.GetProducts().Size(), 0);
    EXPECT_EQ(Result.GetProducts()[0].Id, ProductId);
    EXPECT_EQ(Result.GetProducts()[0].Title, ProductTitle);
    EXPECT_EQ(Result.GetProducts()[0].Description, ProductDescription);
    EXPECT_EQ(Result.GetProducts()[0].Tags.Size(), 0);

    EXPECT_EQ(Result.GetProducts()[0].Media.Size(), MediaSize);

    for (int i = 0; i < Result.GetProducts()[0].Media.Size(); ++i)
    {
        EXPECT_EQ(Result.GetProducts()[0].Media[i].MediaContentType, ImageMediaContentType);
        EXPECT_EQ(Result.GetProducts()[0].Media[i].Url, ImageUrl);
        EXPECT_EQ(Result.GetProducts()[0].Media[i].Alt, ImageAlt);
        EXPECT_EQ(Result.GetProducts()[0].Media[i].Width, ImageWidth);
        EXPECT_EQ(Result.GetProducts()[0].Media[i].Height, ImageHeight);
    }
    EXPECT_EQ(Result.GetProducts()[0].Variants.Size(), VariantSize);

    for (int i = 0; i < Result.GetProducts()[0].Variants.Size(); ++i)
    {
        auto& Variant = Result.GetProducts()[0].Variants[i];
        EXPECT_EQ(Variant.Id, VariantIds[i]);
        EXPECT_EQ(Variant.Title, VariantTitleAndOptionValue[i]);
        EXPECT_EQ(Variant.AvailableForSale, false);
        EXPECT_EQ(Variant.Media.MediaContentType, "");
        EXPECT_EQ(Variant.Media.Alt, ImageAlt);
        EXPECT_EQ(Variant.Media.Url, ImageUrl);
        EXPECT_EQ(Variant.Media.Width, ImageWidth);
        EXPECT_EQ(Variant.Media.Height, ImageHeight);

        EXPECT_EQ(Variant.AvailableStock, 0);

        EXPECT_EQ(Variant.Options.Size(), OptionsSize);

        for (int n = 0; n < Variant.Options.Size(); ++n)
        {
            EXPECT_EQ(Variant.Options[n].Name, OptionsName);
            EXPECT_EQ(Variant.Options[n].Value, VariantTitleAndOptionValue[i]);
        }
    }

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ECOMMERCE_TESTS || RUN_ECOMMERCE_GET_CHECKOUT_INFORMATION_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, ECommerceSystemTests, GetCheckoutInformationTest)
{
    SetRandSeed();
    /*Steps needed to be performed before running this test are:

    1. Create a space (Add to Shopify Creds)
    2. Connected your shopify.dev account to your space using the "Private Access Token" and store name
            Endpoint : /api/v1/spaces/{spaceId}/vendors/shopify
            {
                    "storeName": "string",
                    "isEcommerceActive": true,
                    "privateAccessToken": "string"
            }
    3. Check Shopify has synced with your namespace
            Endpoint: /api/v1/vendors/shopify/validate
            {
                    "storeName": "string",
                    "privateAccessToken": "string"
            }
    4. Create a CartId (Add to Shopify Creds)
            Endpoint: /api/v1/spaces/{spaceId}/vendors/shopify/carts
    Now you can use this test!*/

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ECommerceSystem = SystemsManager.GetECommerceSystem();

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);
    auto Details = GetShopifyDetails();
    const csp::common::String FalseSpaceId = "abcdefghijk1234567891011";
    const csp::common::String FalseCartId = "B1-1234567891011121314151617e8e21er";

    // The additional info such as "CartId" inside of this test need to be added to the ShopifyCreds.txt file on a new line as: <Key> <Value>
    auto [Result] = AWAIT_PRE(ECommerceSystem, GetCheckoutInformation, RequestPredicate, Details["SpaceId"], Details["CartId"]);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_TRUE(std::string(Result.GetCheckoutInfo().StoreUrl.c_str()).find(Details["StoreName"]));

    EXPECT_TRUE(std::string(Result.GetCheckoutInfo().CheckoutUrl.c_str()).find(Details["StoreName"]));

    EXPECT_TRUE(std::string(Result.GetCheckoutInfo().CheckoutUrl.c_str()).find(Details["CartId"]));

    // False Ids
    auto [FalseResult] = AWAIT_PRE(ECommerceSystem, GetCheckoutInformation, RequestPredicate, FalseSpaceId, FalseCartId);
    EXPECT_EQ(FalseResult.GetResultCode(), csp::systems::EResultCode::Failed);

    // False SpaceId
    auto [FalseSpaceResult] = AWAIT_PRE(ECommerceSystem, GetCheckoutInformation, RequestPredicate, FalseSpaceId, Details["CartId"]);
    EXPECT_EQ(FalseSpaceResult.GetResultCode(), csp::systems::EResultCode::Failed);

    // False CartId
    auto [FalseCartResult] = AWAIT_PRE(ECommerceSystem, GetCheckoutInformation, RequestPredicate, Details["SpaceId"], FalseCartId);
    EXPECT_EQ(FalseCartResult.GetResultCode(), csp::systems::EResultCode::Failed);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ECOMMERCE_TESTS || RUN_ECOMMERCE_CREATEANDGETCART_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, ECommerceSystemTests, CreateAndGetCartTest)
{
    SetRandSeed();
    /*Steps needed to be performed before running this test are:
    *
            1. Create a space (Add to Shopify Creds)
            2. Connect your shopify.dev account to your space using the "Private Access Token" and store name
                    Endpoint : /api/v1/spaces/{spaceId}/vendors/shopify
                    {
                            "storeName": "string",
                            "isEcommerceActive": true,
                            "privateAccessToken": "string"
                    }
            3. Check Shopify has synced with your namespace
                    Endpoint: /api/v1/vendors/shopify/validate
                    {
                            "storeName": "string",
                            "privateAccessToken": "string"
                    }
            Now you can use this test!*/

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ECommerceSystem = SystemsManager.GetECommerceSystem();

    auto Details = GetShopifyDetails();
    auto SpaceId = Details["SpaceId"];

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto [CreateCartResult] = AWAIT_PRE(ECommerceSystem, CreateCart, RequestPredicate, SpaceId);

    EXPECT_EQ(CreateCartResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto CreatedCart = CreateCartResult.GetCartInfo();

    EXPECT_EQ(CreatedCart.SpaceId, SpaceId);
    EXPECT_NE(CreatedCart.CartId, "");
    EXPECT_EQ(CreatedCart.CartLines.Size(), 0);
    EXPECT_EQ(CreatedCart.TotalQuantity, 0);

    auto [GetCartResult] = AWAIT_PRE(ECommerceSystem, GetCart, RequestPredicate, SpaceId, CreatedCart.CartId);

    EXPECT_EQ(GetCartResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto Cart = CreateCartResult.GetCartInfo();

    EXPECT_EQ(Cart.SpaceId, SpaceId);
    EXPECT_EQ(Cart.CartId, CreatedCart.CartId);
    EXPECT_EQ(Cart.CartLines.Size(), 0);
    EXPECT_EQ(Cart.TotalQuantity, 0);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ECOMMERCE_TESTS || RUN_ECOMMERCE_CREATECART_BADINPUTS_TEST
CSP_PUBLIC_TEST(CSPEngine, ECommerceSystemTests, CreateCartBadInputTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ECommerceSystem = SystemsManager.GetECommerceSystem();

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Not a valid space ID
    csp::common::String SpaceId = "12a345bc6789d012efa3b45c";

    auto [CreateCartResult] = AWAIT_PRE(ECommerceSystem, CreateCart, RequestPredicate, SpaceId);

    EXPECT_EQ(CreateCartResult.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(CreateCartResult.GetHttpResultCode(), 404);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ECOMMERCE_TESTS || RUN_ECOMMERCE_GETCART_BADINPUTS_TEST
CSP_PUBLIC_TEST(CSPEngine, ECommerceSystemTests, GetCartBadInputTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ECommerceSystem = SystemsManager.GetECommerceSystem();

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Not a valid space ID
    csp::common::String SpaceId = "12a345bc6789d012efa3b45c";

    auto [GetCartResult] = AWAIT_PRE(ECommerceSystem, GetCart, RequestPredicate, SpaceId, "NotAValidCartId");

    EXPECT_EQ(GetCartResult.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(GetCartResult.GetHttpResultCode(), 404);

    LogOut(UserSystem);
}

#endif

#if RUN_ALL_UNIT_TESTS || RUN_ECOMMERCE_TESTS || RUN_ECOMMERCE_ADDCARTLINES_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, ECommerceSystemTests, AddCartLinesTest)
{
    SetRandSeed();
    /*Steps needed to be performed before running this test are:
    *
            1. Create a space (Add to Shopify Creds)
            2. Connect your shopify.dev account to your space using the "Private Access Token" and store name
                    Endpoint : /api/v1/spaces/{spaceId}/vendors/shopify
                    {
                            "storeName": "string",
                            "isEcommerceActive": true,
                            "privateAccessToken": "string"
                    }
            3. Check Shopify has synced with your namespace
                    Endpoint: /api/v1/vendors/shopify/validate
                    {
                            "storeName": "string",
                            "privateAccessToken": "string"
                    }
            Now you can use this test!*/

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ECommerceSystem = SystemsManager.GetECommerceSystem();

    auto Details = GetShopifyDetails();
    auto SpaceId = Details["SpaceId"];
    const csp::common::String ProductId = "gid://shopify/Product/8660541047057";
    csp::common::Array<csp::common::String> VariantIds = { "gid://shopify/ProductVariant/46314311516433",
        "gid://shopify/ProductVariant/46314311647505", "gid://shopify/ProductVariant/46314311745809", "gid://shopify/ProductVariant/46314311844113" };

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create Cart
    auto [CreateCartResult] = AWAIT_PRE(ECommerceSystem, CreateCart, RequestPredicate, SpaceId);

    EXPECT_EQ(CreateCartResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto CreatedCart = CreateCartResult.GetCartInfo();

    EXPECT_EQ(CreatedCart.SpaceId, SpaceId);
    EXPECT_NE(CreatedCart.CartId, "");
    EXPECT_EQ(CreatedCart.CartLines.Size(), 0);
    EXPECT_EQ(CreatedCart.TotalQuantity, 0);

    auto CartLines = csp::common::Array<csp::systems::CartLine>(VariantIds.Size());

    // Add local cart lines
    for (int i = 0; i < VariantIds.Size(); ++i)
    {
        auto CartLine = csp::systems::CartLine();
        CartLine.Quantity = 1;
        CartLine.ProductVariantId = VariantIds[i];

        CartLines[i] = CartLine;
    }

    CreatedCart.CartLines = CartLines;

    CreatedCart.TotalQuantity = 4;

    EXPECT_EQ(CreatedCart.SpaceId, SpaceId);
    EXPECT_NE(CreatedCart.CartId, "");
    EXPECT_EQ(CreatedCart.CartLines.Size(), 4);
    EXPECT_EQ(CreatedCart.TotalQuantity, 4);

    // Add Cart Lines
    auto [AddCartLinesResult] = AWAIT_PRE(ECommerceSystem, UpdateCartInformation, RequestPredicate, CreatedCart);

    EXPECT_EQ(AddCartLinesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto AddCartLinesCart = AddCartLinesResult.GetCartInfo();

    EXPECT_EQ(AddCartLinesCart.SpaceId, SpaceId);
    EXPECT_EQ(AddCartLinesCart.CartId, CreatedCart.CartId);
    EXPECT_EQ(AddCartLinesCart.CartLines.Size(), 4);
    EXPECT_EQ(AddCartLinesCart.TotalQuantity, 4);

    for (int i = 0; i < CartLines.Size(); ++i)
    {
        EXPECT_EQ(AddCartLinesCart.CartLines[i].ProductVariantId, CartLines[i].ProductVariantId);
        EXPECT_NE(AddCartLinesCart.CartLines[i].CartLineId, "");
        EXPECT_EQ(AddCartLinesCart.CartLines[i].Quantity, 1);
    };
}

#endif
#if RUN_ALL_UNIT_TESTS || RUN_ECOMMERCE_TESTS || RUN_ECOMMERCE_UPDATECARTLINES_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, ECommerceSystemTests, UpdateCartLinesTest)
{
    SetRandSeed();
    /*Steps needed to be performed before running this test are:
    *
            1. Create a space (Add to Shopify Creds)
            2. Connect your shopify.dev account to your space using the "Private Access Token" and store name
                    Endpoint : /api/v1/spaces/{spaceId}/vendors/shopify
                    {
                            "storeName": "string",
                            "isEcommerceActive": true,
                            "privateAccessToken": "string"
                    }
            3. Check Shopify has synced with your namespace
                    Endpoint: /api/v1/vendors/shopify/validate
                    {
                            "storeName": "string",
                            "privateAccessToken": "string"
                    }
            Now you can use this test!*/

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ECommerceSystem = SystemsManager.GetECommerceSystem();

    auto Details = GetShopifyDetails();
    auto SpaceId = Details["SpaceId"];
    const csp::common::String ProductId = "gid://shopify/Product/8660541047057";
    const csp::common::String VariantId = "gid://shopify/ProductVariant/46314311516433";

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create Cart
    auto [CreateCartResult] = AWAIT_PRE(ECommerceSystem, CreateCart, RequestPredicate, SpaceId);

    EXPECT_EQ(CreateCartResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto CreatedCart = CreateCartResult.GetCartInfo();

    EXPECT_EQ(CreatedCart.SpaceId, SpaceId);
    EXPECT_NE(CreatedCart.CartId, "");
    EXPECT_EQ(CreatedCart.CartLines.Size(), 0);
    EXPECT_EQ(CreatedCart.TotalQuantity, 0);

    auto CartLines = csp::common::Array<csp::systems::CartLine>(1);

    // Add local cart lines
    auto CartLine = csp::systems::CartLine();
    CartLine.Quantity = 1;
    CartLine.ProductVariantId = VariantId;

    CartLines[0] = CartLine;

    CreatedCart.CartLines = CartLines;

    CreatedCart.TotalQuantity = 1;

    EXPECT_EQ(CreatedCart.SpaceId, SpaceId);
    EXPECT_NE(CreatedCart.CartId, "");
    EXPECT_EQ(CreatedCart.CartLines.Size(), 1);
    EXPECT_EQ(CreatedCart.TotalQuantity, 1);

    // Add Cart Lines
    auto [AddCartLinesResult] = AWAIT_PRE(ECommerceSystem, UpdateCartInformation, RequestPredicate, CreatedCart);

    EXPECT_EQ(AddCartLinesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto AddCartLinesCart = AddCartLinesResult.GetCartInfo();

    EXPECT_EQ(AddCartLinesCart.SpaceId, SpaceId);
    EXPECT_EQ(AddCartLinesCart.CartId, CreatedCart.CartId);
    EXPECT_EQ(AddCartLinesCart.CartLines.Size(), 1);
    EXPECT_EQ(AddCartLinesCart.TotalQuantity, 1);

    for (int i = 0; i < CartLines.Size(); ++i)
    {
        EXPECT_EQ(AddCartLinesCart.CartLines[i].ProductVariantId, CartLines[i].ProductVariantId);
        EXPECT_NE(AddCartLinesCart.CartLines[i].CartLineId, "");
        EXPECT_EQ(AddCartLinesCart.CartLines[i].Quantity, 1);
    };

    // update cart lines adding 1 extra quantity
    CartLine = csp::systems::CartLine();
    CartLine.CartLineId = AddCartLinesCart.CartLines[0].CartLineId;
    CartLine.Quantity = 2;
    CartLine.ProductVariantId = VariantId;

    CartLines[0] = CartLine;

    CreatedCart.CartLines = CartLines;

    CreatedCart.TotalQuantity = 2;

    EXPECT_EQ(CreatedCart.SpaceId, SpaceId);
    EXPECT_NE(CreatedCart.CartId, "");
    EXPECT_EQ(CreatedCart.CartLines.Size(), 1);
    EXPECT_EQ(CreatedCart.TotalQuantity, 2);

    // Add Cart Lines
    auto [UpdateCartLinesResult] = AWAIT_PRE(ECommerceSystem, UpdateCartInformation, RequestPredicate, CreatedCart);

    EXPECT_EQ(UpdateCartLinesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto UpdateCartLinesCart = UpdateCartLinesResult.GetCartInfo();

    EXPECT_EQ(UpdateCartLinesCart.SpaceId, SpaceId);
    EXPECT_EQ(UpdateCartLinesCart.CartId, CreatedCart.CartId);
    EXPECT_EQ(UpdateCartLinesCart.CartLines.Size(), 1);
    EXPECT_EQ(UpdateCartLinesCart.TotalQuantity, 2);

    for (int i = 0; i < CartLines.Size(); ++i)
    {
        EXPECT_EQ(UpdateCartLinesCart.CartLines[i].ProductVariantId, CartLines[i].ProductVariantId);
        EXPECT_NE(UpdateCartLinesCart.CartLines[i].CartLineId, "");
        EXPECT_EQ(UpdateCartLinesCart.CartLines[i].Quantity, 2);
    };
}

#endif

#if RUN_ALL_UNIT_TESTS || RUN_ECOMMERCE_TESTS || RUN_ECOMMERCE_DELETECARTLINES_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, ECommerceSystemTests, DeleteCartLinesTest)
{
    SetRandSeed();
    /*Steps needed to be performed before running this test are:
    *
            1. Create a space (Add to Shopify Creds)
            2. Connect your shopify.dev account to your space using the "Private Access Token" and store name
                    Endpoint : /api/v1/spaces/{spaceId}/vendors/shopify
                    {
                            "storeName": "string",
                            "isEcommerceActive": true,
                            "privateAccessToken": "string"
                    }
            3. Check Shopify has synced with your namespace
                    Endpoint: /api/v1/vendors/shopify/validate
                    {
                            "storeName": "string",
                            "privateAccessToken": "string"
                    }
            Now you can use this test!*/

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ECommerceSystem = SystemsManager.GetECommerceSystem();

    auto Details = GetShopifyDetails();
    auto SpaceId = Details["SpaceId"];
    const csp::common::String ProductId = "gid://shopify/Product/8660541047057";
    const csp::common::String VariantIds = "gid://shopify/ProductVariant/46314311516433";

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create Cart
    auto [CreateCartResult] = AWAIT_PRE(ECommerceSystem, CreateCart, RequestPredicate, SpaceId);

    EXPECT_EQ(CreateCartResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto CreatedCart = CreateCartResult.GetCartInfo();

    EXPECT_EQ(CreatedCart.SpaceId, SpaceId);
    EXPECT_NE(CreatedCart.CartId, "");
    EXPECT_EQ(CreatedCart.CartLines.Size(), 0);
    EXPECT_EQ(CreatedCart.TotalQuantity, 0);

    auto CartLines = csp::common::Array<csp::systems::CartLine>(1);

    // Add local cart lines
    auto CartLine = csp::systems::CartLine();
    CartLine.Quantity = 1;
    CartLine.ProductVariantId = VariantIds;

    CartLines[0] = CartLine;

    CreatedCart.CartLines = CartLines;

    CreatedCart.TotalQuantity = 1;

    EXPECT_EQ(CreatedCart.SpaceId, SpaceId);
    EXPECT_NE(CreatedCart.CartId, "");
    EXPECT_EQ(CreatedCart.CartLines.Size(), 1);
    EXPECT_EQ(CreatedCart.TotalQuantity, 1);

    // Add Cart Lines
    auto [AddCartLinesResult] = AWAIT_PRE(ECommerceSystem, UpdateCartInformation, RequestPredicate, CreatedCart);

    EXPECT_EQ(AddCartLinesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto AddCartLinesCart = AddCartLinesResult.GetCartInfo();

    EXPECT_EQ(AddCartLinesCart.SpaceId, SpaceId);
    EXPECT_EQ(AddCartLinesCart.CartId, CreatedCart.CartId);
    EXPECT_EQ(AddCartLinesCart.CartLines.Size(), 1);
    EXPECT_EQ(AddCartLinesCart.TotalQuantity, 1);

    for (int i = 0; i < CartLines.Size(); ++i)
    {
        EXPECT_EQ(AddCartLinesCart.CartLines[i].ProductVariantId, CartLines[i].ProductVariantId);
        EXPECT_NE(AddCartLinesCart.CartLines[i].CartLineId, CartLines[i].CartLineId);
        EXPECT_EQ(AddCartLinesCart.CartLines[i].Quantity, 1);
    };

    // Add update cart lines quantity to 0
    CartLine.Quantity = 0;
    CartLine.ProductVariantId = VariantIds;
    CartLine.CartLineId = AddCartLinesCart.CartLines[0].CartLineId;

    CartLines[0] = CartLine;

    CreatedCart.CartLines = CartLines;

    CreatedCart.TotalQuantity = 1;

    EXPECT_EQ(CreatedCart.SpaceId, SpaceId);
    EXPECT_NE(CreatedCart.CartId, "");
    EXPECT_EQ(CreatedCart.CartLines.Size(), 1);
    EXPECT_EQ(CreatedCart.TotalQuantity, 1);

    // Delete Cart Lines
    auto [DeleteCartLinesResult] = AWAIT_PRE(ECommerceSystem, UpdateCartInformation, RequestPredicate, CreatedCart);

    EXPECT_EQ(DeleteCartLinesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto DeleteCartLinesCart = DeleteCartLinesResult.GetCartInfo();

    EXPECT_EQ(DeleteCartLinesCart.SpaceId, SpaceId);
    EXPECT_EQ(DeleteCartLinesCart.CartId, CreatedCart.CartId);
    EXPECT_EQ(DeleteCartLinesCart.CartLines.Size(), 0);
    EXPECT_EQ(DeleteCartLinesCart.TotalQuantity, 0);
}

#endif

#if RUN_ALL_UNIT_TESTS || RUN_ECOMMERCE_TESTS || RUN_ECOMMERCE_ADDSHOPIFYSTORE_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, ECommerceSystemTests, AddShopifyStoreTest)
{
    SetRandSeed();
    /*Steps needed to be performed before running this test are:
    *
            1. Create a space (Add to Shopify Creds)
            2. Create a Shopify Store on the Shopify site (Ensure it has at least 1 product)
            3. Connect the Shopify Store to the Space you created
            4. Add `SpaceId YourSpaceId`, `StoreName MyStoreName` and `PrivateAccessToken MyPrivateAccessToken` to the ShopifyCreds.txt
            Now you can use this test!*/

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ECommerceSystem = SystemsManager.GetECommerceSystem();

    auto Details = GetShopifyDetails();
    auto SpaceId = Details["SpaceId"];
    auto StoreName = Details["StoreName"];
    auto PrivateAccessToken = Details["PrivateAccessToken"];

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto [ValidateShopifyStoreResult] = AWAIT_PRE(ECommerceSystem, ValidateShopifyStore, RequestPredicate, StoreName, PrivateAccessToken);

    EXPECT_EQ(ValidateShopifyStoreResult.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_TRUE(ValidateShopifyStoreResult.ValidateResult);

    auto [AddShopifyStoreResult] = AWAIT_PRE(ECommerceSystem, AddShopifyStore, RequestPredicate, StoreName, SpaceId, false, PrivateAccessToken);

    EXPECT_EQ(AddShopifyStoreResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto ShopifyStore = AddShopifyStoreResult.GetShopifyStoreInfo();

    EXPECT_EQ(ShopifyStore.SpaceId, SpaceId);
    EXPECT_EQ(ShopifyStore.IsEcommerceActive, false);
    EXPECT_NE(ShopifyStore.StoreId, "");
    EXPECT_EQ(ShopifyStore.StoreName, StoreName);

    { // Enable Ecommerce
        auto [EnableStoreResult] = AWAIT_PRE(ECommerceSystem, SetECommerceActiveInSpace, RequestPredicate, StoreName, SpaceId, true);

        EXPECT_EQ(EnableStoreResult.GetResultCode(), csp::systems::EResultCode::Success);

        auto EnableStore = EnableStoreResult.GetShopifyStoreInfo();

        EXPECT_EQ(EnableStore.IsEcommerceActive, true);
    }

    { // Disable Ecommerce
        auto [DisableStoreResult] = AWAIT_PRE(ECommerceSystem, SetECommerceActiveInSpace, RequestPredicate, StoreName, SpaceId, false);

        EXPECT_EQ(DisableStoreResult.GetResultCode(), csp::systems::EResultCode::Success);

        auto DisableStore = DisableStoreResult.GetShopifyStoreInfo();

        EXPECT_EQ(DisableStore.IsEcommerceActive, false);
    }

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ECOMMERCE_TESTS || RUN_ECOMMERCE_GETSHOPIFYSTORES_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, ECommerceSystemTests, GetShopifyStoresTest)
{
    SetRandSeed();
    /*Steps needed to be performed before running this test are:
    *
            1. Create a space (Add to Shopify Creds)
            2. Create a Shopify Store on the Shopify site (Ensure it has at least 1 product)
            3. Connect the Shopify Store to the Space you created
            3. Add `SpaceId YourSpaceId`, `StoreName MyStoreName` and `PrivateAccessToken MyPrivateAccessToken` to the ShopifyCreds.txt
            Now you can use this test!*/

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ECommerceSystem = SystemsManager.GetECommerceSystem();

    auto Details = GetShopifyDetails();
    auto SpaceId = Details["SpaceId"];
    auto StoreName = Details["StoreName"];
    auto PrivateAccessToken = Details["PrivateAccessToken"];

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto [ValidateShopifyStoreResult] = AWAIT_PRE(ECommerceSystem, ValidateShopifyStore, RequestPredicate, StoreName, PrivateAccessToken);

    EXPECT_EQ(ValidateShopifyStoreResult.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_TRUE(ValidateShopifyStoreResult.ValidateResult);

    auto [AddShopifyStoreResult] = AWAIT_PRE(ECommerceSystem, AddShopifyStore, RequestPredicate, StoreName, SpaceId, false, PrivateAccessToken);

    EXPECT_EQ(AddShopifyStoreResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto ShopifyStore = AddShopifyStoreResult.GetShopifyStoreInfo();

    EXPECT_EQ(ShopifyStore.SpaceId, SpaceId);
    EXPECT_EQ(ShopifyStore.IsEcommerceActive, false);
    EXPECT_NE(ShopifyStore.StoreId, "");
    EXPECT_EQ(ShopifyStore.StoreName, StoreName);

    auto [GetShopifyStoresResult] = AWAIT_PRE(ECommerceSystem, GetShopifyStores, RequestPredicate, nullptr);

    EXPECT_EQ(GetShopifyStoresResult.GetShopifyStores()[0].StoreId, ShopifyStore.StoreId);
    EXPECT_EQ(GetShopifyStoresResult.GetShopifyStores()[0].SpaceId, ShopifyStore.SpaceId);
    EXPECT_EQ(GetShopifyStoresResult.GetShopifyStores()[0].SpaceOwnerId, ShopifyStore.SpaceOwnerId);
    EXPECT_EQ(GetShopifyStoresResult.GetShopifyStores()[0].StoreName, ShopifyStore.StoreName);

    LogOut(UserSystem);
}
#endif