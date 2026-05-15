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
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Systems/ECommerce/ECommerceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>

using namespace csp::systems;

// These tests currently require manual steps and will be reviewed as part of OF-1535.

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

csp::common::Map<csp::common::String, csp::common::String> GetShopifyDetails()
{
    if (!std::filesystem::exists("ShopifyCreds.txt"))
    {
        LogFatal("ShopifyCreds.txt not found! This file must exist and must contain a minimum of the following information:\nSpaceId "
                 "<SpaceId>\nProductId <ProductId> but may also need \nProductId <ProductId>\nCartId <CartId>\n");
    }

    csp::common::Map<csp::common::String, csp::common::String> outMap;

    std::ifstream credsFile;
    credsFile.open("ShopifyCreds.txt");
    std::string key, value;
    while (credsFile >> key >> value)
    {
        outMap[key.c_str()] = value.c_str();
    }

    return outMap;
}

/*These test are currently internal tests because they utilise that is currently only available
through internal CSP infrastructure.*/

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

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* eCommerceSystem = systemsManager.GetECommerceSystem();

    // This is an example from Shopify dev quickstart "Gift Card"
    const csp::common::String productId = "gid://shopify/Product/8660541047057";
    const csp::common::String productTitle = "Gift Card";
    const csp::common::String productDescription = "This is a gift card for the store";
    const csp::common::String imageMediaContentType = "IMAGE";
    const csp::common::String imageAlt = "Gift card that shows text: Generated data gift card";
    const csp::common::String imageUrl = "https://cdn.shopify.com/s/files/1/0803/6070/2225/products/gift_card.png?v=1691076851";
    const int32_t imageWidth = 2881;
    const int32_t imageHeight = 2881;
    const int32_t variantSize = 4;
    const int32_t mediaSize = 1;
    const int32_t optionsSize = 1;
    const csp::common::String optionsName = "Denominations";
    csp::common::Array<csp::common::String> variantTitleAndOptionValue = { "$10", "$25", "$50", "$100" };
    csp::common::Array<csp::common::String> variantIds = { "gid://shopify/ProductVariant/46314311516433",
        "gid://shopify/ProductVariant/46314311647505", "gid://shopify/ProductVariant/46314311745809", "gid://shopify/ProductVariant/46314311844113" };
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);
    auto details = GetShopifyDetails();

    auto [Result] = AWAIT_PRE(eCommerceSystem, GetProductInformation, RequestPredicate, details["SpaceId"], details["ProductId"]);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result.GetProductInfo().Id, productId);
    EXPECT_EQ(Result.GetProductInfo().Title, productTitle);
    EXPECT_EQ(Result.GetProductInfo().Description, productDescription);
    EXPECT_EQ(Result.GetProductInfo().Tags.Size(), 0);

    EXPECT_EQ(Result.GetProductInfo().Media.Size(), mediaSize);

    for (size_t i = 0; i < Result.GetProductInfo().Media.Size(); ++i)
    {
        EXPECT_EQ(Result.GetProductInfo().Media[i].MediaContentType, imageMediaContentType);
        EXPECT_EQ(Result.GetProductInfo().Media[i].Url, imageUrl);
        EXPECT_EQ(Result.GetProductInfo().Media[i].Alt, imageAlt);
        EXPECT_EQ(Result.GetProductInfo().Media[i].Width, imageWidth);
        EXPECT_EQ(Result.GetProductInfo().Media[i].Height, imageHeight);
    }
    EXPECT_EQ(Result.GetProductInfo().Variants.Size(), variantSize);

    for (size_t i = 0; i < Result.GetProductInfo().Variants.Size(); ++i)
    {
        EXPECT_EQ(Result.GetProductInfo().Variants[i].Id, variantIds[i]);
        EXPECT_EQ(Result.GetProductInfo().Variants[i].Title, variantTitleAndOptionValue[i]);
        EXPECT_EQ(Result.GetProductInfo().Variants[i].AvailableForSale, true);
        EXPECT_EQ(Result.GetProductInfo().Variants[i].Media.MediaContentType, "");
        EXPECT_EQ(Result.GetProductInfo().Variants[i].Media.Alt, imageAlt);
        EXPECT_EQ(Result.GetProductInfo().Variants[i].Media.Url, imageUrl);
        EXPECT_EQ(Result.GetProductInfo().Variants[i].Media.Width, imageWidth);
        EXPECT_EQ(Result.GetProductInfo().Variants[i].Media.Height, imageHeight);

        EXPECT_EQ(Result.GetProductInfo().Variants[i].Options.Size(), optionsSize);

        for (size_t n = 0; n < Result.GetProductInfo().Variants[i].Options.Size(); ++n)
        {
            EXPECT_EQ(Result.GetProductInfo().Variants[i].Options[n].Name, optionsName);
            EXPECT_EQ(Result.GetProductInfo().Variants[i].Options[n].Value, variantTitleAndOptionValue[i]);
        }

        EXPECT_EQ(Result.GetProductInfo().Variants[i].UnitPrice.Amount, 0);
        EXPECT_EQ(Result.GetProductInfo().Variants[i].UnitPrice.CurrencyCode, "");
    }

    LogOut(userSystem);
}

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

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* eCommerceSystem = systemsManager.GetECommerceSystem();

    // This is an example from Shopify dev quickstart "Gift Card"
    const csp::common::String productId = "gid://shopify/Product/8566195847465";
    const csp::common::String productTitle = "Gift Card";
    const csp::common::String productDescription = "This is a gift card for the store";
    const csp::common::String imageMediaContentType = "IMAGE";
    const csp::common::String imageAlt = "Gift card that shows text: Generated data gift card";
    const csp::common::String imageUrl = "https://cdn.shopify.com/s/files/1/0813/7238/1481/products/gift_card.png?v=1692877145";
    const int32_t imageWidth = 2881;
    const int32_t imageHeight = 2881;
    const int32_t variantSize = 1;
    const int32_t mediaSize = 1;
    const int32_t optionsSize = 0;
    const csp::common::String optionsName = "Denominations";
    csp::common::Array<csp::common::String> variantTitleAndOptionValue = { "$10", "$25", "$50", "$100" };
    csp::common::Array<csp::common::String> variantIds = { "gid://shopify/ProductVariant/46375586136361",
        "gid://shopify/ProductVariant/46375586234665", "gid://shopify/ProductVariant/46375586398505", "gid://shopify/ProductVariant/46375586496809" };
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);
    auto details = GetShopifyDetails();

    auto [Result] = AWAIT_PRE(eCommerceSystem, GetProductInfoCollectionByVariantIds, RequestPredicate, details["SpaceId"], { details["VariantId"] });
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_GT(Result.GetProducts().Size(), 0);
    EXPECT_EQ(Result.GetProducts()[0].Id, productId);
    EXPECT_EQ(Result.GetProducts()[0].Title, productTitle);
    EXPECT_EQ(Result.GetProducts()[0].Description, productDescription);
    EXPECT_EQ(Result.GetProducts()[0].Tags.Size(), 0);

    EXPECT_EQ(Result.GetProducts()[0].Media.Size(), mediaSize);

    for (size_t i = 0; i < Result.GetProducts()[0].Media.Size(); ++i)
    {
        EXPECT_EQ(Result.GetProducts()[0].Media[i].MediaContentType, imageMediaContentType);
        EXPECT_EQ(Result.GetProducts()[0].Media[i].Url, imageUrl);
        EXPECT_EQ(Result.GetProducts()[0].Media[i].Alt, imageAlt);
        EXPECT_EQ(Result.GetProducts()[0].Media[i].Width, imageWidth);
        EXPECT_EQ(Result.GetProducts()[0].Media[i].Height, imageHeight);
    }
    EXPECT_EQ(Result.GetProducts()[0].Variants.Size(), variantSize);

    for (size_t i = 0; i < Result.GetProducts()[0].Variants.Size(); ++i)
    {
        auto& variant = Result.GetProducts()[0].Variants[i];
        EXPECT_EQ(variant.Id, variantIds[i]);
        EXPECT_EQ(variant.Title, variantTitleAndOptionValue[i]);
        EXPECT_EQ(variant.AvailableForSale, false);
        EXPECT_EQ(variant.Media.MediaContentType, "");
        EXPECT_EQ(variant.Media.Alt, imageAlt);
        EXPECT_EQ(variant.Media.Url, imageUrl);
        EXPECT_EQ(variant.Media.Width, imageWidth);
        EXPECT_EQ(variant.Media.Height, imageHeight);

        EXPECT_EQ(variant.AvailableStock, 0);

        EXPECT_EQ(variant.Options.Size(), optionsSize);

        for (size_t n = 0; n < variant.Options.Size(); ++n)
        {
            EXPECT_EQ(variant.Options[n].Name, optionsName);
            EXPECT_EQ(variant.Options[n].Value, variantTitleAndOptionValue[i]);
        }
    }

    LogOut(userSystem);
}

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

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* eCommerceSystem = systemsManager.GetECommerceSystem();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);
    auto details = GetShopifyDetails();
    const csp::common::String falseSpaceId = "abcdefghijk1234567891011";
    const csp::common::String falseCartId = "B1-1234567891011121314151617e8e21er";

    // The additional info such as "CartId" inside of this test need to be added to the ShopifyCreds.txt file on a new line as: <Key> <Value>
    auto [Result] = AWAIT_PRE(eCommerceSystem, GetCheckoutInformation, RequestPredicate, details["SpaceId"], details["CartId"]);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_TRUE(std::string(Result.GetCheckoutInfo().StoreUrl.c_str()).find(details["StoreName"]));

    EXPECT_TRUE(std::string(Result.GetCheckoutInfo().CheckoutUrl.c_str()).find(details["StoreName"]));

    EXPECT_TRUE(std::string(Result.GetCheckoutInfo().CheckoutUrl.c_str()).find(details["CartId"]));

    // False Ids
    auto [FalseResult] = AWAIT_PRE(eCommerceSystem, GetCheckoutInformation, RequestPredicate, falseSpaceId, falseCartId);
    EXPECT_EQ(FalseResult.GetResultCode(), csp::systems::EResultCode::Failed);

    // False SpaceId
    auto [FalseSpaceResult] = AWAIT_PRE(eCommerceSystem, GetCheckoutInformation, RequestPredicate, falseSpaceId, details["CartId"]);
    EXPECT_EQ(FalseSpaceResult.GetResultCode(), csp::systems::EResultCode::Failed);

    // False CartId
    auto [FalseCartResult] = AWAIT_PRE(eCommerceSystem, GetCheckoutInformation, RequestPredicate, details["SpaceId"], falseCartId);
    EXPECT_EQ(FalseCartResult.GetResultCode(), csp::systems::EResultCode::Failed);

    LogOut(userSystem);
}

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

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* eCommerceSystem = systemsManager.GetECommerceSystem();

    auto details = GetShopifyDetails();
    auto spaceId = details["SpaceId"];

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto [CreateCartResult] = AWAIT_PRE(eCommerceSystem, CreateCart, RequestPredicate, spaceId);

    EXPECT_EQ(CreateCartResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto createdCart = CreateCartResult.GetCartInfo();

    EXPECT_EQ(createdCart.SpaceId, spaceId);
    EXPECT_NE(createdCart.CartId, "");
    EXPECT_EQ(createdCart.CartLines.Size(), 0);
    EXPECT_EQ(createdCart.TotalQuantity, 0);

    auto [GetCartResult] = AWAIT_PRE(eCommerceSystem, GetCart, RequestPredicate, spaceId, createdCart.CartId);

    EXPECT_EQ(GetCartResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto cart = CreateCartResult.GetCartInfo();

    EXPECT_EQ(cart.SpaceId, spaceId);
    EXPECT_EQ(cart.CartId, createdCart.CartId);
    EXPECT_EQ(cart.CartLines.Size(), 0);
    EXPECT_EQ(cart.TotalQuantity, 0);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ECommerceSystemTests, CreateCartBadInputTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* eCommerceSystem = systemsManager.GetECommerceSystem();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Not a valid space ID
    csp::common::String spaceId = "12a345bc6789d012efa3b45c";

    auto [CreateCartResult] = AWAIT_PRE(eCommerceSystem, CreateCart, RequestPredicate, spaceId);

    EXPECT_EQ(CreateCartResult.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(CreateCartResult.GetHttpResultCode(), 404);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ECommerceSystemTests, GetCartBadInputTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* eCommerceSystem = systemsManager.GetECommerceSystem();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Not a valid space ID
    csp::common::String spaceId = "12a345bc6789d012efa3b45c";

    auto [GetCartResult] = AWAIT_PRE(eCommerceSystem, GetCart, RequestPredicate, spaceId, "NotAValidCartId");

    EXPECT_EQ(GetCartResult.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(GetCartResult.GetHttpResultCode(), 404);

    LogOut(userSystem);
}

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

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* eCommerceSystem = systemsManager.GetECommerceSystem();

    auto details = GetShopifyDetails();
    auto spaceId = details["SpaceId"];
    const csp::common::String productId = "gid://shopify/Product/8660541047057";
    csp::common::Array<csp::common::String> variantIds = { "gid://shopify/ProductVariant/46314311516433",
        "gid://shopify/ProductVariant/46314311647505", "gid://shopify/ProductVariant/46314311745809", "gid://shopify/ProductVariant/46314311844113" };

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create Cart
    auto [CreateCartResult] = AWAIT_PRE(eCommerceSystem, CreateCart, RequestPredicate, spaceId);

    EXPECT_EQ(CreateCartResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto createdCart = CreateCartResult.GetCartInfo();

    EXPECT_EQ(createdCart.SpaceId, spaceId);
    EXPECT_NE(createdCart.CartId, "");
    EXPECT_EQ(createdCart.CartLines.Size(), 0);
    EXPECT_EQ(createdCart.TotalQuantity, 0);

    auto cartLines = csp::common::Array<csp::systems::CartLine>(variantIds.Size());

    // Add local cart lines
    for (size_t i = 0; i < variantIds.Size(); ++i)
    {
        auto cartLine = csp::systems::CartLine();
        cartLine.Quantity = 1;
        cartLine.ProductVariantId = variantIds[i];

        cartLines[i] = cartLine;
    }

    createdCart.CartLines = cartLines;

    createdCart.TotalQuantity = 4;

    EXPECT_EQ(createdCart.SpaceId, spaceId);
    EXPECT_NE(createdCart.CartId, "");
    EXPECT_EQ(createdCart.CartLines.Size(), 4);
    EXPECT_EQ(createdCart.TotalQuantity, 4);

    // Add Cart Lines
    auto [AddCartLinesResult] = AWAIT_PRE(eCommerceSystem, UpdateCartInformation, RequestPredicate, createdCart);

    EXPECT_EQ(AddCartLinesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto addCartLinesCart = AddCartLinesResult.GetCartInfo();

    EXPECT_EQ(addCartLinesCart.SpaceId, spaceId);
    EXPECT_EQ(addCartLinesCart.CartId, createdCart.CartId);
    EXPECT_EQ(addCartLinesCart.CartLines.Size(), 4);
    EXPECT_EQ(addCartLinesCart.TotalQuantity, 4);

    for (size_t i = 0; i < cartLines.Size(); ++i)
    {
        EXPECT_EQ(addCartLinesCart.CartLines[i].ProductVariantId, cartLines[i].ProductVariantId);
        EXPECT_NE(addCartLinesCart.CartLines[i].CartLineId, "");
        EXPECT_EQ(addCartLinesCart.CartLines[i].Quantity, 1);
    };
}

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

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* eCommerceSystem = systemsManager.GetECommerceSystem();

    auto details = GetShopifyDetails();
    auto spaceId = details["SpaceId"];
    const csp::common::String productId = "gid://shopify/Product/8660541047057";
    const csp::common::String variantId = "gid://shopify/ProductVariant/46314311516433";

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create Cart
    auto [CreateCartResult] = AWAIT_PRE(eCommerceSystem, CreateCart, RequestPredicate, spaceId);

    EXPECT_EQ(CreateCartResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto createdCart = CreateCartResult.GetCartInfo();

    EXPECT_EQ(createdCart.SpaceId, spaceId);
    EXPECT_NE(createdCart.CartId, "");
    EXPECT_EQ(createdCart.CartLines.Size(), 0);
    EXPECT_EQ(createdCart.TotalQuantity, 0);

    auto cartLines = csp::common::Array<csp::systems::CartLine>(1);

    // Add local cart lines
    auto cartLine = csp::systems::CartLine();
    cartLine.Quantity = 1;
    cartLine.ProductVariantId = variantId;

    cartLines[0] = cartLine;

    createdCart.CartLines = cartLines;

    createdCart.TotalQuantity = 1;

    EXPECT_EQ(createdCart.SpaceId, spaceId);
    EXPECT_NE(createdCart.CartId, "");
    EXPECT_EQ(createdCart.CartLines.Size(), 1);
    EXPECT_EQ(createdCart.TotalQuantity, 1);

    // Add Cart Lines
    auto [AddCartLinesResult] = AWAIT_PRE(eCommerceSystem, UpdateCartInformation, RequestPredicate, createdCart);

    EXPECT_EQ(AddCartLinesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto addCartLinesCart = AddCartLinesResult.GetCartInfo();

    EXPECT_EQ(addCartLinesCart.SpaceId, spaceId);
    EXPECT_EQ(addCartLinesCart.CartId, createdCart.CartId);
    EXPECT_EQ(addCartLinesCart.CartLines.Size(), 1);
    EXPECT_EQ(addCartLinesCart.TotalQuantity, 1);

    for (size_t i = 0; i < cartLines.Size(); ++i)
    {
        EXPECT_EQ(addCartLinesCart.CartLines[i].ProductVariantId, cartLines[i].ProductVariantId);
        EXPECT_NE(addCartLinesCart.CartLines[i].CartLineId, "");
        EXPECT_EQ(addCartLinesCart.CartLines[i].Quantity, 1);
    };

    // update cart lines adding 1 extra quantity
    cartLine = csp::systems::CartLine();
    cartLine.CartLineId = addCartLinesCart.CartLines[0].CartLineId;
    cartLine.Quantity = 2;
    cartLine.ProductVariantId = variantId;

    cartLines[0] = cartLine;

    createdCart.CartLines = cartLines;

    createdCart.TotalQuantity = 2;

    EXPECT_EQ(createdCart.SpaceId, spaceId);
    EXPECT_NE(createdCart.CartId, "");
    EXPECT_EQ(createdCart.CartLines.Size(), 1);
    EXPECT_EQ(createdCart.TotalQuantity, 2);

    // Add Cart Lines
    auto [UpdateCartLinesResult] = AWAIT_PRE(eCommerceSystem, UpdateCartInformation, RequestPredicate, createdCart);

    EXPECT_EQ(UpdateCartLinesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto updateCartLinesCart = UpdateCartLinesResult.GetCartInfo();

    EXPECT_EQ(updateCartLinesCart.SpaceId, spaceId);
    EXPECT_EQ(updateCartLinesCart.CartId, createdCart.CartId);
    EXPECT_EQ(updateCartLinesCart.CartLines.Size(), 1);
    EXPECT_EQ(updateCartLinesCart.TotalQuantity, 2);

    for (size_t i = 0; i < cartLines.Size(); ++i)
    {
        EXPECT_EQ(updateCartLinesCart.CartLines[i].ProductVariantId, cartLines[i].ProductVariantId);
        EXPECT_NE(updateCartLinesCart.CartLines[i].CartLineId, "");
        EXPECT_EQ(updateCartLinesCart.CartLines[i].Quantity, 2);
    };
}

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

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* eCommerceSystem = systemsManager.GetECommerceSystem();

    auto details = GetShopifyDetails();
    auto spaceId = details["SpaceId"];
    const csp::common::String productId = "gid://shopify/Product/8660541047057";
    const csp::common::String variantIds = "gid://shopify/ProductVariant/46314311516433";

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create Cart
    auto [CreateCartResult] = AWAIT_PRE(eCommerceSystem, CreateCart, RequestPredicate, spaceId);

    EXPECT_EQ(CreateCartResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto createdCart = CreateCartResult.GetCartInfo();

    EXPECT_EQ(createdCart.SpaceId, spaceId);
    EXPECT_NE(createdCart.CartId, "");
    EXPECT_EQ(createdCart.CartLines.Size(), 0);
    EXPECT_EQ(createdCart.TotalQuantity, 0);

    auto cartLines = csp::common::Array<csp::systems::CartLine>(1);

    // Add local cart lines
    auto cartLine = csp::systems::CartLine();
    cartLine.Quantity = 1;
    cartLine.ProductVariantId = variantIds;

    cartLines[0] = cartLine;

    createdCart.CartLines = cartLines;

    createdCart.TotalQuantity = 1;

    EXPECT_EQ(createdCart.SpaceId, spaceId);
    EXPECT_NE(createdCart.CartId, "");
    EXPECT_EQ(createdCart.CartLines.Size(), 1);
    EXPECT_EQ(createdCart.TotalQuantity, 1);

    // Add Cart Lines
    auto [AddCartLinesResult] = AWAIT_PRE(eCommerceSystem, UpdateCartInformation, RequestPredicate, createdCart);

    EXPECT_EQ(AddCartLinesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto addCartLinesCart = AddCartLinesResult.GetCartInfo();

    EXPECT_EQ(addCartLinesCart.SpaceId, spaceId);
    EXPECT_EQ(addCartLinesCart.CartId, createdCart.CartId);
    EXPECT_EQ(addCartLinesCart.CartLines.Size(), 1);
    EXPECT_EQ(addCartLinesCart.TotalQuantity, 1);

    for (size_t i = 0; i < cartLines.Size(); ++i)
    {
        EXPECT_EQ(addCartLinesCart.CartLines[i].ProductVariantId, cartLines[i].ProductVariantId);
        EXPECT_NE(addCartLinesCart.CartLines[i].CartLineId, cartLines[i].CartLineId);
        EXPECT_EQ(addCartLinesCart.CartLines[i].Quantity, 1);
    };

    // Add update cart lines quantity to 0
    cartLine.Quantity = 0;
    cartLine.ProductVariantId = variantIds;
    cartLine.CartLineId = addCartLinesCart.CartLines[0].CartLineId;

    cartLines[0] = cartLine;

    createdCart.CartLines = cartLines;

    createdCart.TotalQuantity = 1;

    EXPECT_EQ(createdCart.SpaceId, spaceId);
    EXPECT_NE(createdCart.CartId, "");
    EXPECT_EQ(createdCart.CartLines.Size(), 1);
    EXPECT_EQ(createdCart.TotalQuantity, 1);

    // Delete Cart Lines
    auto [DeleteCartLinesResult] = AWAIT_PRE(eCommerceSystem, UpdateCartInformation, RequestPredicate, createdCart);

    EXPECT_EQ(DeleteCartLinesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto deleteCartLinesCart = DeleteCartLinesResult.GetCartInfo();

    EXPECT_EQ(deleteCartLinesCart.SpaceId, spaceId);
    EXPECT_EQ(deleteCartLinesCart.CartId, createdCart.CartId);
    EXPECT_EQ(deleteCartLinesCart.CartLines.Size(), 0);
    EXPECT_EQ(deleteCartLinesCart.TotalQuantity, 0);
}

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

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* eCommerceSystem = systemsManager.GetECommerceSystem();

    auto details = GetShopifyDetails();
    auto spaceId = details["SpaceId"];
    auto storeName = details["StoreName"];
    auto privateAccessToken = details["PrivateAccessToken"];

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto [ValidateShopifyStoreResult] = AWAIT_PRE(eCommerceSystem, ValidateShopifyStore, RequestPredicate, storeName, privateAccessToken);

    EXPECT_EQ(ValidateShopifyStoreResult.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_TRUE(ValidateShopifyStoreResult.ValidateResult);

    auto [AddShopifyStoreResult] = AWAIT_PRE(eCommerceSystem, AddShopifyStore, RequestPredicate, storeName, spaceId, false, privateAccessToken);

    EXPECT_EQ(AddShopifyStoreResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto shopifyStore = AddShopifyStoreResult.GetShopifyStoreInfo();

    EXPECT_EQ(shopifyStore.SpaceId, spaceId);
    EXPECT_EQ(shopifyStore.IsEcommerceActive, false);
    EXPECT_NE(shopifyStore.StoreId, "");
    EXPECT_EQ(shopifyStore.StoreName, storeName);

    { // Enable Ecommerce
        auto [EnableStoreResult] = AWAIT_PRE(eCommerceSystem, SetECommerceActiveInSpace, RequestPredicate, storeName, spaceId, true);

        EXPECT_EQ(EnableStoreResult.GetResultCode(), csp::systems::EResultCode::Success);

        auto enableStore = EnableStoreResult.GetShopifyStoreInfo();

        EXPECT_EQ(enableStore.IsEcommerceActive, true);
    }

    { // Disable Ecommerce
        auto [DisableStoreResult] = AWAIT_PRE(eCommerceSystem, SetECommerceActiveInSpace, RequestPredicate, storeName, spaceId, false);

        EXPECT_EQ(DisableStoreResult.GetResultCode(), csp::systems::EResultCode::Success);

        auto disableStore = DisableStoreResult.GetShopifyStoreInfo();

        EXPECT_EQ(disableStore.IsEcommerceActive, false);
    }

    LogOut(userSystem);
}

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

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* eCommerceSystem = systemsManager.GetECommerceSystem();

    auto details = GetShopifyDetails();
    auto spaceId = details["SpaceId"];
    auto storeName = details["StoreName"];
    auto privateAccessToken = details["PrivateAccessToken"];

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto [ValidateShopifyStoreResult] = AWAIT_PRE(eCommerceSystem, ValidateShopifyStore, RequestPredicate, storeName, privateAccessToken);

    EXPECT_EQ(ValidateShopifyStoreResult.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_TRUE(ValidateShopifyStoreResult.ValidateResult);

    auto [AddShopifyStoreResult] = AWAIT_PRE(eCommerceSystem, AddShopifyStore, RequestPredicate, storeName, spaceId, false, privateAccessToken);

    EXPECT_EQ(AddShopifyStoreResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto shopifyStore = AddShopifyStoreResult.GetShopifyStoreInfo();

    EXPECT_EQ(shopifyStore.SpaceId, spaceId);
    EXPECT_EQ(shopifyStore.IsEcommerceActive, false);
    EXPECT_NE(shopifyStore.StoreId, "");
    EXPECT_EQ(shopifyStore.StoreName, storeName);

    auto [GetShopifyStoresResult] = AWAIT_PRE(eCommerceSystem, GetShopifyStores, RequestPredicate, nullptr);

    EXPECT_EQ(GetShopifyStoresResult.GetShopifyStores()[0].StoreId, shopifyStore.StoreId);
    EXPECT_EQ(GetShopifyStoresResult.GetShopifyStores()[0].SpaceId, shopifyStore.SpaceId);
    EXPECT_EQ(GetShopifyStoresResult.GetShopifyStores()[0].SpaceOwnerId, shopifyStore.SpaceOwnerId);
    EXPECT_EQ(GetShopifyStoresResult.GetShopifyStores()[0].StoreName, shopifyStore.StoreName);

    LogOut(userSystem);
}