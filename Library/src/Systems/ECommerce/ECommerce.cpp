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
#include "CSP/Systems/ECommerce/ECommerce.h"

#include "Services/ApiBase/ApiBase.h"
#include "Services/AggregationService/Dto.h"

#include <regex>

namespace chs_aggregation = csp::services::generated::aggregationservice;
namespace csp::systems
{

bool CurrencyInfo::operator==(const CurrencyInfo& other) const { return Amount == other.Amount && CurrencyCode == other.CurrencyCode; }

bool ProductMediaInfo::operator==(const ProductMediaInfo& other) const
{
    return MediaContentType == other.MediaContentType && Alt == other.Alt && Url == other.Url && Width == other.Width && Height == other.Height;
}
bool VariantOptionInfo::operator==(const VariantOptionInfo& other) const { return Name == other.Name && Value == other.Value; }

bool ProductVariantInfo::operator==(const ProductVariantInfo& other) const
{
    return Id == other.Id && Title == other.Title && Url == other.Url && AvailableForSale == other.AvailableForSale && Media == other.Media
        && Options == other.Options && UnitPrice == other.UnitPrice && AvailableStock == other.AvailableStock;
}

bool ProductInfo::operator==(const ProductInfo& other) const
{
    return Id == other.Id && Title == other.Title && CreatedAt == other.CreatedAt && Description == other.Description && Variants == other.Variants
        && Tags == other.Tags && Media == other.Media;
}

bool CheckoutInfo::operator==(const CheckoutInfo& other) const { return StoreUrl == other.StoreUrl && CheckoutUrl == other.CheckoutUrl; }

bool CartLine::operator==(const CartLine& other) const
{
    return CartLineId == other.CartLineId && ProductVariantId == other.ProductVariantId && Quantity == other.Quantity;
}

bool CartInfo::operator==(const CartInfo& other) const
{
    return SpaceId == other.SpaceId && CartId == other.CartId && CartLines == other.CartLines && TotalQuantity == other.TotalQuantity;
}

bool ShopifyStoreInfo::operator==(const ShopifyStoreInfo& other) const
{
    return StoreId == other.StoreId && StoreName == other.StoreName && SpaceOwnerId == other.SpaceOwnerId && SpaceId == other.SpaceId
        && IsEcommerceActive == other.IsEcommerceActive;
}

bool CurrencyInfo::operator!=(const CurrencyInfo& other) const { return !(*this == other); }
bool ProductMediaInfo::operator!=(const ProductMediaInfo& other) const { return !(*this == other); }
bool VariantOptionInfo::operator!=(const VariantOptionInfo& other) const { return !(*this == other); }
bool ProductVariantInfo::operator!=(const ProductVariantInfo& other) const { return !(*this == other); }
bool ProductInfo::operator!=(const ProductInfo& other) const { return !(*this == other); }
bool CheckoutInfo::operator!=(const CheckoutInfo& other) const { return !(*this == other); }
bool CartLine::operator!=(const CartLine& other) const { return !(*this == other); }
bool CartInfo::operator!=(const CartInfo& other) const { return !(*this == other); }
bool ShopifyStoreInfo::operator!=(const ShopifyStoreInfo& other) const { return !(*this == other); }

void ProductInfoDtoToProductInfo(const chs_aggregation::ShopifyProductDto& dto, csp::systems::ProductInfo& productInfo)
{
    productInfo.Id = dto.GetId();
    productInfo.Title = dto.GetTitle();
    if (dto.HasCreatedAt())
    {
        productInfo.CreatedAt = dto.GetCreatedAt();
    }

    if (dto.HasDescription())
    {
        productInfo.Description = dto.GetDescription();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyProductDto missing Description");
    }

    if (dto.HasVariants())
    {
        auto variantProductInformation = dto.GetVariants();
        productInfo.Variants = common::Array<ProductVariantInfo>(variantProductInformation.size());

        for (size_t i = 0; i < variantProductInformation.size(); ++i)
        {
            productInfo.Variants[i].Id = variantProductInformation[i]->GetId();
            productInfo.Variants[i].Title = variantProductInformation[i]->GetTitle();

            if (variantProductInformation[i]->HasAvailableForSale())
            {
                productInfo.Variants[i].AvailableForSale = variantProductInformation[i]->GetAvailableForSale();
            }
            else
            {
                CSP_LOG_ERROR_MSG("ShopifyProductDto missing HasAvailableForSale");
            }

            if (variantProductInformation[i]->HasImage())
            {
                auto variantProductImage = variantProductInformation[i]->GetImage();

                if (variantProductImage->HasMediaContentType())
                {
                    productInfo.Variants[i].Media.MediaContentType = variantProductImage->GetMediaContentType();
                }

                if (variantProductImage->HasAlt())
                {
                    productInfo.Variants[i].Media.Alt = variantProductImage->GetAlt();
                }

                if (variantProductImage->HasUrl())
                {
                    productInfo.Variants[i].Media.Url = variantProductImage->GetUrl();
                }

                if (variantProductImage->HasWidth())
                {
                    productInfo.Variants[i].Media.Width = variantProductImage->GetWidth();
                }

                if (variantProductImage->HasHeight())
                {
                    productInfo.Variants[i].Media.Height = variantProductImage->GetHeight();
                }
            }
            else
            {
                CSP_LOG_ERROR_MSG("ShopifyProductDto missing Image");
            }

            if (dto.GetVariants()[i]->HasSelectedOptions())
            {
                auto variantOptionInformation = dto.GetVariants()[i]->GetSelectedOptions();

                productInfo.Variants[i].Options = common::Array<VariantOptionInfo>(variantOptionInformation.size());

                for (size_t n = 0; n < variantOptionInformation.size(); ++n)
                {
                    productInfo.Variants[i].Options[n].Name = variantOptionInformation[n]->GetOptionName();
                    productInfo.Variants[i].Options[n].Value = variantOptionInformation[n]->GetOptionValue();
                }
            }
            else
            {
                CSP_LOG_ERROR_MSG("ShopifyProductDto missing SelectedOptions");
            }

            if (variantProductInformation[i]->HasUnitPrice())
            {
                if (variantProductInformation[i]->GetUnitPrice()->HasAmount())
                {
                    productInfo.Variants[i].UnitPrice.Amount = variantProductInformation[i]->GetUnitPrice()->GetAmount();
                }

                if (variantProductInformation[i]->GetUnitPrice()->HasCurrencyCode())
                {
                    productInfo.Variants[i].UnitPrice.CurrencyCode = variantProductInformation[i]->GetUnitPrice()->GetCurrencyCode();
                }
            }
            else
            {
                CSP_LOG_ERROR_MSG("ShopifyProductDto missing UnitPrice");
            }

            if (variantProductInformation[i]->HasQuantityAvailable())
            {
                productInfo.Variants[i].AvailableStock = variantProductInformation[i]->GetQuantityAvailable();
            }
            else
            {
                CSP_LOG_ERROR_MSG("ShopifyProductDto missing QuantityAvailable");
            }
        }
    }
    {
        CSP_LOG_ERROR_MSG("ShopifyProductDto missing Variants");
    }

    if (dto.HasTags())
    {
        auto tagsProductInformation = dto.GetTags();

        productInfo.Tags = common::Array<common::String>(tagsProductInformation.size());

        for (size_t i = 0; i < tagsProductInformation.size(); ++i)
        {
            productInfo.Tags[i] = tagsProductInformation[i];
        }
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyProductDto missing Tags");
    }

    if (dto.HasMedia())
    {
        auto mediaProductInformation = dto.GetMedia();

        productInfo.Media = common::Array<ProductMediaInfo>(mediaProductInformation.size());

        for (size_t i = 0; i < mediaProductInformation.size(); ++i)
        {
            if (mediaProductInformation[i]->HasAlt())
            {
                productInfo.Media[i].Alt = mediaProductInformation[i]->GetAlt();
            }

            if (mediaProductInformation[i]->HasUrl())
            {
                productInfo.Media[i].Url = mediaProductInformation[i]->GetUrl();
            }

            if (mediaProductInformation[i]->HasMediaContentType())
            {
                productInfo.Media[i].MediaContentType = mediaProductInformation[i]->GetMediaContentType();
            }

            if (mediaProductInformation[i]->HasWidth())
            {
                productInfo.Media[i].Width = mediaProductInformation[i]->GetWidth();
            }

            if (mediaProductInformation[i]->HasHeight())
            {
                productInfo.Media[i].Height = mediaProductInformation[i]->GetHeight();
            }
        }
    }
    {
        CSP_LOG_ERROR_MSG("ShopifyProductDto missing Media");
    }
}

void ProductInfoDtoToProductInfoVariantCollection(
    const std::vector<chs_aggregation::ShopifyProductDto>& dtoArray, csp::common::Array<ProductInfo>& productInfoCollection)
{
    productInfoCollection = common::Array<csp::systems::ProductInfo>(productInfoCollection.Size());
    int totalVariantIndex = 0; // Count the total number of variants included in the product Dto's

    for (size_t dtoCount = 0; dtoCount < dtoArray.size(); dtoCount++) // Loop all Dto's (products)
    {
        const chs_aggregation::ShopifyProductDto& dto = dtoArray[dtoCount];

        if (dto.HasVariants()) // if there are no variants, we don't process the dto (shouldn't happen)
        {
            for (size_t variantCount = 0; variantCount < dto.GetVariants().size();
                 variantCount++) // Loop each variant in the product dto and store the info into the indexed output array
            {
                auto variantProductInformation = dto.GetVariants()[variantCount];

                productInfoCollection[totalVariantIndex].Id = dto.GetId();
                productInfoCollection[totalVariantIndex].Title = dto.GetTitle();

                if (dto.HasCreatedAt())
                {
                    productInfoCollection[totalVariantIndex].CreatedAt = dto.GetCreatedAt();
                }

                if (dto.HasDescription())
                {
                    productInfoCollection[totalVariantIndex].Description = dto.GetDescription();
                }

                // The output should only contain productinfo with single variants in, so we hardcode the index and size of the array here
                productInfoCollection[totalVariantIndex].Variants = common::Array<ProductVariantInfo>(1);
                productInfoCollection[totalVariantIndex].Variants[0].Id = variantProductInformation->GetId();
                productInfoCollection[totalVariantIndex].Variants[0].Title = variantProductInformation->GetTitle();

                if (variantProductInformation->HasImage())
                {
                    auto variantProductImage = variantProductInformation->GetImage();

                    if (variantProductImage->HasMediaContentType())
                    {
                        productInfoCollection[totalVariantIndex].Variants[0].Media.MediaContentType = variantProductImage->GetMediaContentType();
                    }

                    if (variantProductImage->HasAlt())
                    {
                        productInfoCollection[totalVariantIndex].Variants[0].Media.Alt = variantProductImage->GetAlt();
                    }

                    if (variantProductImage->HasUrl())
                    {
                        productInfoCollection[totalVariantIndex].Variants[0].Media.Url = variantProductImage->GetUrl();
                    }

                    if (variantProductImage->HasWidth())
                    {
                        productInfoCollection[totalVariantIndex].Variants[0].Media.Width = variantProductImage->GetWidth();
                    }

                    if (variantProductImage->HasHeight())
                    {
                        productInfoCollection[totalVariantIndex].Variants[0].Media.Height = variantProductImage->GetHeight();
                    }
                }
                else
                {
                    CSP_LOG_MSG(csp::common::LogLevel::Log, "ShopifyProductDto missing Image");
                }

                if (variantProductInformation->HasSelectedOptions())
                {
                    auto variantOptionInformation = variantProductInformation->GetSelectedOptions();

                    productInfoCollection[totalVariantIndex].Variants[0].Options = common::Array<VariantOptionInfo>(variantOptionInformation.size());

                    for (size_t n = 0; n < variantOptionInformation.size(); ++n)
                    {
                        productInfoCollection[totalVariantIndex].Variants[0].Options[n].Name = variantOptionInformation[n]->GetOptionName();
                        productInfoCollection[totalVariantIndex].Variants[0].Options[n].Value = variantOptionInformation[n]->GetOptionValue();
                    }
                }
                else
                {
                    CSP_LOG_MSG(csp::common::LogLevel::Log, "ShopifyProductDto missing SelectedOptions");
                }

                if (variantProductInformation->HasUnitPrice())
                {
                    if (variantProductInformation->GetUnitPrice()->HasAmount())
                    {
                        productInfoCollection[totalVariantIndex].Variants[0].UnitPrice.Amount
                            = variantProductInformation->GetUnitPrice()->GetAmount();
                    }

                    if (variantProductInformation->GetUnitPrice()->HasCurrencyCode())
                    {
                        productInfoCollection[totalVariantIndex].Variants[0].UnitPrice.CurrencyCode
                            = variantProductInformation->GetUnitPrice()->GetCurrencyCode();
                    }
                }
                else
                {
                    CSP_LOG_MSG(csp::common::LogLevel::Log, "ShopifyProductDto missing UnitPrice");
                }

                if (variantProductInformation->HasQuantityAvailable())
                {
                    productInfoCollection[totalVariantIndex].Variants[0].AvailableStock = variantProductInformation->GetQuantityAvailable();
                }
                else
                {
                    CSP_LOG_MSG(csp::common::LogLevel::Log, "ShopifyProductDto missing QuantityAvailable");
                }

                if (dto.HasTags())
                {
                    auto tagsProductInformation = dto.GetTags();

                    productInfoCollection[totalVariantIndex].Tags = common::Array<common::String>(tagsProductInformation.size());

                    for (size_t j = 0; j < tagsProductInformation.size(); ++j)
                    {
                        productInfoCollection[totalVariantIndex].Tags[j] = tagsProductInformation[j];
                    }
                }
                else
                {
                    CSP_LOG_MSG(csp::common::LogLevel::Log, "ShopifyProductDto missing Tags");
                }

                if (dto.HasMedia())
                {
                    auto mediaProductInformation = dto.GetMedia();

                    productInfoCollection[totalVariantIndex].Media = common::Array<ProductMediaInfo>(mediaProductInformation.size());

                    for (size_t j = 0; j < mediaProductInformation.size(); ++j)
                    {
                        if (mediaProductInformation[j]->HasAlt())
                        {
                            productInfoCollection[totalVariantIndex].Media[j].Alt = mediaProductInformation[j]->GetAlt();
                        }

                        if (mediaProductInformation[j]->HasUrl())
                        {
                            productInfoCollection[totalVariantIndex].Media[j].Url = mediaProductInformation[j]->GetUrl();
                        }

                        if (mediaProductInformation[j]->HasMediaContentType())
                        {
                            productInfoCollection[totalVariantIndex].Media[j].MediaContentType = mediaProductInformation[j]->GetMediaContentType();
                        }

                        if (mediaProductInformation[j]->HasWidth())
                        {
                            productInfoCollection[totalVariantIndex].Media[j].Width = mediaProductInformation[j]->GetWidth();
                        }

                        if (mediaProductInformation[j]->HasHeight())
                        {
                            productInfoCollection[totalVariantIndex].Media[j].Height = mediaProductInformation[j]->GetHeight();
                        }
                    }
                }
                else
                {
                    CSP_LOG_MSG(csp::common::LogLevel::Log, "ShopifyProductDto missing Media");
                }

                totalVariantIndex++;
            }
        }
        else
        {
            CSP_LOG_MSG(csp::common::LogLevel::Log, "ShopifyProductDto missing Variants");
        }
    }
}

void CartDtoToCartInfo(const chs_aggregation::ShopifyCartDto& cartDto, csp::systems::CartInfo& cart)
{
    if (cartDto.HasSpaceId())
    {
        cart.SpaceId = cartDto.GetSpaceId();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyCartDto missing SpaceId");
    }

    if (cartDto.HasShopifyCartId())
    {
        // Magnopus Services adds a prefix to the Shopify cart ID. We strip that out here to ensure we are only
        // using the raw Shopify ID. Magnopus Services accepts the ID with or without the prefix so the latter
        // is chosen to be more generic if used with other cloud service providers.
        std::regex cartIdPrefixRegex("^gid:\\/\\/shopify\\/Cart\\/");
        cart.CartId = std::regex_replace(cartDto.GetShopifyCartId().c_str(), cartIdPrefixRegex, "").c_str();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyCartDto missing ShopifyCartId");
    }

    if (cartDto.HasLines())
    {
        auto dtoLines = cartDto.GetLines();
        cart.CartLines = csp::common::Array<csp::systems::CartLine>(dtoLines.size());

        for (size_t i = 0; i < dtoLines.size(); ++i)
        {
            auto cartLineDto = *dtoLines[i];

            cart.CartLines[i] = csp::systems::CartLine();

            if (cartLineDto.HasShopifyCartLineId())
            {
                cart.CartLines[i].CartLineId = cartLineDto.GetShopifyCartLineId();
            }
            else
            {
                CSP_LOG_ERROR_MSG("ShopifyCartLineDto missing ShopifyCartLineId");
            }

            if (cartLineDto.HasProductVariantId())
            {
                cart.CartLines[i].ProductVariantId = cartLineDto.GetProductVariantId();
            }
            else
            {
                CSP_LOG_ERROR_MSG("ShopifyCartLineDto missing ProductVariantId");
            }

            if (cartLineDto.HasQuantity())
            {
                cart.CartLines[i].Quantity = cartLineDto.GetQuantity();
            }
            else
            {
                CSP_LOG_ERROR_MSG("ShopifyCartLineDto missing Quantity");
            }
        }
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyCartDto missing Lines");
    }

    if (cartDto.HasTotalQuantity())
    {
        cart.TotalQuantity = cartDto.GetTotalQuantity();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyCartDto missing TotalQuantity");
    }
}

void ShopifyStoreDtoToShopifyStoreInfo(const chs_aggregation::ShopifyStorefrontDto& storeDto, csp::systems::ShopifyStoreInfo& store)
{
    if (storeDto.HasId())
    {
        store.StoreId = storeDto.GetId();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing StoreId");
    }

    if (storeDto.HasStoreName())
    {
        store.StoreName = storeDto.GetStoreName();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing StoreName");
    }

    if (storeDto.HasSpaceOwnerId())
    {
        store.SpaceOwnerId = storeDto.GetSpaceOwnerId();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing SpaceOwnerId");
    }

    if (storeDto.HasSpaceId())
    {
        store.SpaceId = storeDto.GetSpaceId();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing SpaceId");
    }

    if (storeDto.HasIsEcommerceActive())
    {
        store.IsEcommerceActive = storeDto.GetIsEcommerceActive();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing IsEcommerceActive");
    }
}

void ShopifyStoreDtoArrayToShopifyStoreInfoArray(
    const std::vector<chs_aggregation::ShopifyStorefrontDto>& storeDtos, csp::common::Array<csp::systems::ShopifyStoreInfo>& stores)
{
    for (size_t i = 0; i < storeDtos.size(); i++)
    {
        const chs_aggregation::ShopifyStorefrontDto& storeDto = storeDtos[i];
        csp::systems::ShopifyStoreInfo& store = stores[i];

        if (storeDto.HasId())
        {
            store.StoreId = storeDto.GetId();
        }
        else
        {
            CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing StoreId");
        }

        if (storeDto.HasStoreName())
        {
            store.StoreName = storeDto.GetStoreName();
        }
        else
        {
            CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing StoreName");
        }

        if (storeDto.HasSpaceOwnerId())
        {
            store.SpaceOwnerId = storeDto.GetSpaceOwnerId();
        }
        else
        {
            CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing SpaceOwnerId");
        }

        if (storeDto.HasSpaceId())
        {
            store.SpaceId = storeDto.GetSpaceId();
        }
        else
        {
            CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing SpaceId");
        }

        if (storeDto.HasIsEcommerceActive())
        {
            store.IsEcommerceActive = storeDto.GetIsEcommerceActive();
        }
        else
        {
            CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing IsEcommerceActive");
        }
    }
}

const ProductInfo& ProductInfoResult::GetProductInfo() const { return m_productInformation; }

ProductInfo& ProductInfoResult::GetProductInfo() { return m_productInformation; }

void ProductInfoResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    chs_aggregation::ShopifyProductDto* productInformationResponse = static_cast<chs_aggregation::ShopifyProductDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        productInformationResponse->FromJson(response->GetPayload().GetContent());
        ProductInfoDtoToProductInfo(*productInformationResponse, m_productInformation);
    }
}

const CheckoutInfo& CheckoutInfoResult::GetCheckoutInfo() const { return m_checkoutInformation; }

CheckoutInfo& CheckoutInfoResult::GetCheckoutInfo() { return m_checkoutInformation; }

void CheckoutInfoResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    chs_aggregation::ShopifyCheckoutDto* checkoutInformationResponse = static_cast<chs_aggregation::ShopifyCheckoutDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        checkoutInformationResponse->FromJson(response->GetPayload().GetContent());

        if (checkoutInformationResponse->HasStoreUrl())
        {
            m_checkoutInformation.StoreUrl = checkoutInformationResponse->GetStoreUrl();
        }

        if (checkoutInformationResponse->HasCheckoutUrl())
        {
            m_checkoutInformation.CheckoutUrl = checkoutInformationResponse->GetCheckoutUrl();
        }
    }
}

const CartInfo& CartInfoResult::GetCartInfo() const { return m_cart; }

CartInfo& CartInfoResult::GetCartInfo() { return m_cart; }

void CartInfoResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* dto = static_cast<chs_aggregation::ShopifyCartDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        dto->FromJson(response->GetPayload().GetContent());

        CartDtoToCartInfo(*dto, m_cart);
    }
}

const ShopifyStoreInfo& AddShopifyStoreResult::GetShopifyStoreInfo() const { return m_store; }

ShopifyStoreInfo& AddShopifyStoreResult::GetShopifyStoreInfo() { return m_store; }

void AddShopifyStoreResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* dto = static_cast<chs_aggregation::ShopifyStorefrontDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        dto->FromJson(response->GetPayload().GetContent());

        ShopifyStoreDtoToShopifyStoreInfo(*dto, m_store);
    }
}

void ValidateShopifyStoreResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        ValidateResult = (bool)response->GetPayload().GetContent();
    }
}

const csp::common::Array<ProductInfo>& ProductInfoCollectionResult::GetProducts() const { return m_products; }

csp::common::Array<ProductInfo>& ProductInfoCollectionResult::GetProducts() { return m_products; }

void ProductInfoCollectionResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* productCollectionResponse = static_cast<csp::services::DtoArray<chs_aggregation::ShopifyProductDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        productCollectionResponse->FromJson(response->GetPayload().GetContent());

        // Extract data from the response into our Products array
        std::vector<chs_aggregation::ShopifyProductDto>& productsArray = productCollectionResponse->GetArray();

        // Loop through products to count the variants, we want 1 output product per variant
        size_t variantCount = 0;
        for (size_t dtoCount = 0; dtoCount < productsArray.size(); dtoCount++)
        {
            variantCount += productsArray[dtoCount].GetVariants().size();
        }

        m_products = csp::common::Array<csp::systems::ProductInfo>(variantCount);

        ProductInfoDtoToProductInfoVariantCollection(productsArray, m_products);
    }
}

const csp::common::Array<ShopifyStoreInfo>& GetShopifyStoresResult::GetShopifyStores() const { return m_stores; }

csp::common::Array<ShopifyStoreInfo>& GetShopifyStoresResult::GetShopifyStores() { return m_stores; }

void GetShopifyStoresResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* shopifyStoresResponse = static_cast<csp::services::DtoArray<chs_aggregation::ShopifyStorefrontDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        shopifyStoresResponse->FromJson(response->GetPayload().GetContent());

        // Extract data from the response into our Stores array
        std::vector<chs_aggregation::ShopifyStorefrontDto>& storesArray = shopifyStoresResponse->GetArray();

        m_stores = csp::common::Array<csp::systems::ShopifyStoreInfo>(storesArray.size());

        ShopifyStoreDtoArrayToShopifyStoreInfoArray(storesArray, m_stores);
    }
}
} // namespace csp::systems
