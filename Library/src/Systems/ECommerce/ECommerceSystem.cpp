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

#include "CSP/Systems/ECommerce/ECommerceSystem.h"

#include "CSP/Systems/Users/UserSystem.h"
#include "CallHelpers.h"
#include "Common/Convert.h"
#include "ECommerceSystemHelpers.h"
#include "Services/AggregationService/Api.h"
#include "Systems/ResultHelpers.h"

#include <array>

using namespace csp;

namespace chs = csp::services::generated::aggregationservice;

namespace
{

void RemoveUrl(csp::common::String& url)
{
    if (std::string(url.c_str()).find("gid://shopify/Cart/") != std::string::npos)
    {
        url = url.Split('/')[url.Split('/').Size() - 1];
    }
    else if (std::string(url.c_str()).find("?cart=") != std::string::npos)
    {
        url = url.Split('/')[url.Split('/').Size() - 1].c_str();
    }
    else if (std::string(url.c_str()).find("gid://shopify/ProductVariant/") != std::string::npos)
    {
        url = url.Split('/')[url.Split('/').Size() - 1];
    }
}

} // namespace

namespace csp::systems
{

ECommerceSystem::ECommerceSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , m_shopifyApi(nullptr)
{
}

ECommerceSystem::ECommerceSystem(csp::web::WebClient* inWebClient, csp::common::LogSystem& logSystem)
    : SystemBase(inWebClient, nullptr, &logSystem)
{
    m_shopifyApi = new chs::ShopifyApi(inWebClient);
}

ECommerceSystem::~ECommerceSystem() { delete (m_shopifyApi); }

void ECommerceSystem::GetProductInformation(const common::String& spaceId, const common::String& productId, ProductInfoResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_shopifyApi->CreateHandler<ProductInfoResultCallback, ProductInfoResult, void, chs::ShopifyProductDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::ShopifyApi*>(m_shopifyApi)->spacesSpaceIdVendorsShopifyProductsProductIdGet({ spaceId, productId }, responseHandler);
}

void ECommerceSystem::GetProductInfoCollectionByVariantIds(
    const common::String& spaceId, const Array<common::String>& inVariantIds, ProductInfoCollectionResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler = m_shopifyApi->CreateHandler<ProductInfoCollectionResultCallback, ProductInfoCollectionResult,
        void, csp::services::DtoArray<chs::ShopifyProductDto>>(callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    const std::vector<common::String> variantIds = common::Convert(inVariantIds);

    static_cast<chs::ShopifyApi*>(m_shopifyApi)->spacesSpaceIdVendorsShopifyProductsVariantsGet({ spaceId, variantIds }, responseHandler);
}

void ECommerceSystem::GetCheckoutInformation(const common::String& spaceId, const common::String& cartId, CheckoutInfoResultCallback callback)
{
    std::string characterCheck = cartId.c_str();
    if (characterCheck.find("/") != std::string::npos || characterCheck.find(R"(\)") != std::string::npos)
    {
        CSP_LOG_ERROR_MSG("Call to GetCheckoutInformation failed. CartId must NOT include path characters forward or back slash.");

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<CheckoutInfoResult>());

        return;
    }

    csp::services::ResponseHandlerPtr responseHandler
        = m_shopifyApi->CreateHandler<CheckoutInfoResultCallback, CheckoutInfoResult, void, chs::ShopifyCheckoutDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::ShopifyApi*>(m_shopifyApi)->spacesSpaceIdVendorsShopifyCartsCartIdCheckout_infoGet({ spaceId, cartId }, responseHandler);
}

void ECommerceSystem::CreateCart(const common::String& spaceId, CartInfoResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler = m_shopifyApi->CreateHandler<CartInfoResultCallback, CartInfoResult, void, chs::ShopifyCartDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::ShopifyApi*>(m_shopifyApi)->spacesSpaceIdVendorsShopifyCartsPost({ spaceId }, responseHandler);
}

void ECommerceSystem::GetCart(const common::String& spaceId, const common::String& cartId, CartInfoResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler = m_shopifyApi->CreateHandler<CartInfoResultCallback, CartInfoResult, void, chs::ShopifyCartDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::ShopifyApi*>(m_shopifyApi)->spacesSpaceIdVendorsShopifyCartsCartIdGet({ spaceId, cartId }, responseHandler);
}

void ECommerceSystem::GetShopifyStores(const csp::common::Optional<bool>& isActive, GetShopifyStoresResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_shopifyApi->CreateHandler<GetShopifyStoresResultCallback, GetShopifyStoresResult, void, csp::services::DtoArray<chs::ShopifyStorefrontDto>>(
            callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    std::optional<bool> activeParam;

    if (isActive.HasValue())
    {
        activeParam = *isActive;
    }

    auto& systemsManager = SystemsManager::Get();
    const auto* userSystem = systemsManager.GetUserSystem();

    const auto& userId = userSystem->GetLoginState().UserId;

    static_cast<chs::ShopifyApi*>(m_shopifyApi)
        ->vendorsShopifyUsersUserIdStorefrontsGet({ userId, activeParam, std::nullopt, std::nullopt }, responseHandler);
}

void ECommerceSystem::AddShopifyStore(const common::String& storeName, const common::String& spaceId, const bool isEcommerceActive,
    const common::String& privateAccessToken, AddShopifyStoreResultCallback callback)
{
    auto shopifyStorefrontInfo = systems::ECommerceSystemHelpers::DefaultShopifyStorefrontInfo();
    shopifyStorefrontInfo->SetStoreName(storeName);
    shopifyStorefrontInfo->SetIsEcommerceActive(isEcommerceActive);
    shopifyStorefrontInfo->SetPrivateAccessToken(privateAccessToken);

    csp::services::ResponseHandlerPtr responseHandler
        = m_shopifyApi->CreateHandler<AddShopifyStoreResultCallback, AddShopifyStoreResult, void, chs::ShopifyStorefrontDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::ShopifyApi*>(m_shopifyApi)->spacesSpaceIdVendorsShopifyPut({ spaceId, shopifyStorefrontInfo }, responseHandler);
}

void ECommerceSystem::SetECommerceActiveInSpace(
    const common::String& storeName, const common::String& spaceId, const bool isEcommerceActive, SetECommerceActiveResultCallback callback)
{
    auto shopifyStorefrontInfo = systems::ECommerceSystemHelpers::DefaultShopifyStorefrontInfo();
    shopifyStorefrontInfo->SetStoreName(storeName);
    shopifyStorefrontInfo->SetIsEcommerceActive(isEcommerceActive);

    csp::services::ResponseHandlerPtr responseHandler
        = m_shopifyApi->CreateHandler<SetECommerceActiveResultCallback, AddShopifyStoreResult, void, chs::ShopifyStorefrontDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::ShopifyApi*>(m_shopifyApi)->spacesSpaceIdVendorsShopifyPut({ spaceId, shopifyStorefrontInfo }, responseHandler);
}

void ECommerceSystem::ValidateShopifyStore(
    const common::String& storeName, const common::String& privateAccessToken, ValidateShopifyStoreResultCallback callback)
{
    auto shopifyStorefrontValidationInfo = systems::ECommerceSystemHelpers::DefaultShopifyStorefrontValidationRequest();
    shopifyStorefrontValidationInfo->SetStoreName(storeName);
    shopifyStorefrontValidationInfo->SetPrivateAccessToken(privateAccessToken);

    csp::services::ResponseHandlerPtr responseHandler
        = m_shopifyApi->CreateHandler<ValidateShopifyStoreResultCallback, ValidateShopifyStoreResult, void, chs::ShopifyStorefrontValidationRequest>(
            callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs::ShopifyApi*>(m_shopifyApi)->vendorsShopifyValidatePut({ shopifyStorefrontValidationInfo }, responseHandler);
}

void ECommerceSystem::UpdateCartInformation(const CartInfo& cartInformation, CartInfoResultCallback callback)
{
    if (cartInformation.SpaceId.IsEmpty() || cartInformation.CartId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("SpaceId and CartId inside CartInformation must be populated.")

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<CartInfoResult>());

        return;
    }

    auto cartUpdateInfo = std::make_shared<chs::ShopifyCartUpdateDto>();

    if (!cartInformation.CartLines.IsEmpty())
    {
        auto cartLinesRemoval = std::vector<std::shared_ptr<chs::ShopifyCartLineDto>>();
        auto cartLinesUpdate = std::vector<std::shared_ptr<chs::ShopifyCartLineDto>>();
        auto cartLinesAdditions = std::vector<std::shared_ptr<chs::ShopifyCartLineDto>>();

        for (size_t i = 0; i < cartInformation.CartLines.Size(); ++i)
        {

            csp::common::String cartLineId = cartInformation.CartLines[i].CartLineId;
            csp::common::String productVariantId = cartInformation.CartLines[i].ProductVariantId;
            RemoveUrl(cartLineId);
            RemoveUrl(productVariantId);

            if (cartInformation.CartLines[i].Quantity == 0)
            {
                auto cartLineRemoval = std::make_shared<chs::ShopifyCartLineDto>();

                cartLineRemoval->SetShopifyCartLineId(cartLineId);
                cartLineRemoval->SetProductVariantId(productVariantId);

                cartLinesRemoval.push_back(cartLineRemoval);
            }
            else if (cartInformation.CartLines[i].CartLineId.IsEmpty() && cartInformation.CartLines[i].Quantity != 0)
            {
                auto cartLineAdditions = std::make_shared<chs::ShopifyCartLineDto>();

                cartLineAdditions->SetQuantity(cartInformation.CartLines[i].Quantity);
                cartLineAdditions->SetShopifyCartLineId(cartLineId);
                cartLineAdditions->SetProductVariantId(productVariantId);

                cartLinesAdditions.push_back(cartLineAdditions);
            }
            else
            {
                auto cartLineUpdate = std::make_shared<chs::ShopifyCartLineDto>();

                cartLineUpdate->SetQuantity(cartInformation.CartLines[i].Quantity);
                cartLineUpdate->SetShopifyCartLineId(cartLineId);
                cartLineUpdate->SetProductVariantId(productVariantId);

                cartLinesUpdate.push_back(cartLineUpdate);
            }
        }

        cartUpdateInfo->SetAddLineCartChanges(cartLinesAdditions);
        cartUpdateInfo->SetRemoveLineCartChanges(cartLinesRemoval);
        cartUpdateInfo->SetUpdateLineQtyCartChanges(cartLinesUpdate);
    }

    csp::services::ResponseHandlerPtr responseHandler = m_shopifyApi->CreateHandler<CartInfoResultCallback, CartInfoResult, void, chs::ShopifyCartDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseCreated);
    static_cast<chs::ShopifyApi*>(m_shopifyApi)
        ->spacesSpaceIdVendorsShopifyCartsCartIdPut({ cartInformation.SpaceId, cartInformation.CartId, cartUpdateInfo }, responseHandler);
}

} // namespace csp::systems
