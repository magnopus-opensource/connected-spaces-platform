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
#include "Services/aggregationservice/Dto.h"

#include <regex>

namespace chs_aggregation = csp::services::generated::aggregationservice;
namespace csp::systems
{

void ProductInfoDtoToProductInfo(const chs_aggregation::ShopifyProductDto& Dto, csp::systems::ProductInfo& ProductInfo)
{
    ProductInfo.Id = Dto.GetId();
    ProductInfo.Title = Dto.GetTitle();
    if (Dto.HasCreatedAt())
    {
        ProductInfo.CreatedAt = Dto.GetCreatedAt();
    }

    if (Dto.HasDescription())
    {
        ProductInfo.Description = Dto.GetDescription();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyProductDto missing Description");
    }

    if (Dto.HasVariants())
    {
        auto VariantProductInformation = Dto.GetVariants();
        ProductInfo.Variants = common::Array<ProductVariantInfo>(VariantProductInformation.size());

        for (int i = 0; i < VariantProductInformation.size(); ++i)
        {
            ProductInfo.Variants[i].Id = VariantProductInformation[i]->GetId();
            ProductInfo.Variants[i].Title = VariantProductInformation[i]->GetTitle();

            if (VariantProductInformation[i]->HasAvailableForSale())
            {
                ProductInfo.Variants[i].AvailableForSale = VariantProductInformation[i]->GetAvailableForSale();
            }
            else
            {
                CSP_LOG_ERROR_MSG("ShopifyProductDto missing HasAvailableForSale");
            }

            if (VariantProductInformation[i]->HasImage())
            {
                auto VariantProductImage = VariantProductInformation[i]->GetImage();

                if (VariantProductImage->HasMediaContentType())
                {
                    ProductInfo.Variants[i].Media.MediaContentType = VariantProductImage->GetMediaContentType();
                }

                if (VariantProductImage->HasAlt())
                {
                    ProductInfo.Variants[i].Media.Alt = VariantProductImage->GetAlt();
                }

                if (VariantProductImage->HasUrl())
                {
                    ProductInfo.Variants[i].Media.Url = VariantProductImage->GetUrl();
                }

                if (VariantProductImage->HasWidth())
                {
                    ProductInfo.Variants[i].Media.Width = VariantProductImage->GetWidth();
                }

                if (VariantProductImage->HasHeight())
                {
                    ProductInfo.Variants[i].Media.Height = VariantProductImage->GetHeight();
                }
            }
            else
            {
                CSP_LOG_ERROR_MSG("ShopifyProductDto missing Image");
            }

            if (Dto.GetVariants()[i]->HasSelectedOptions())
            {
                auto VariantOptionInformation = Dto.GetVariants()[i]->GetSelectedOptions();

                ProductInfo.Variants[i].Options = common::Array<VariantOptionInfo>(VariantOptionInformation.size());

                for (int n = 0; n < VariantOptionInformation.size(); ++n)
                {
                    ProductInfo.Variants[i].Options[n].Name = VariantOptionInformation[n]->GetOptionName();
                    ProductInfo.Variants[i].Options[n].Value = VariantOptionInformation[n]->GetOptionValue();
                }
            }
            else
            {
                CSP_LOG_ERROR_MSG("ShopifyProductDto missing SelectedOptions");
            }

            if (VariantProductInformation[i]->HasUnitPrice())
            {
                if (VariantProductInformation[i]->GetUnitPrice()->HasAmount())
                {
                    ProductInfo.Variants[i].UnitPrice.Amount = VariantProductInformation[i]->GetUnitPrice()->GetAmount();
                }

                if (VariantProductInformation[i]->GetUnitPrice()->HasCurrencyCode())
                {
                    ProductInfo.Variants[i].UnitPrice.CurrencyCode = VariantProductInformation[i]->GetUnitPrice()->GetCurrencyCode();
                }
            }
            else
            {
                CSP_LOG_ERROR_MSG("ShopifyProductDto missing UnitPrice");
            }

            if (VariantProductInformation[i]->HasQuantityAvailable())
            {
                ProductInfo.Variants[i].AvailableStock = VariantProductInformation[i]->GetQuantityAvailable();
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

    if (Dto.HasTags())
    {
        auto TagsProductInformation = Dto.GetTags();

        ProductInfo.Tags = common::Array<common::String>(TagsProductInformation.size());

        for (int i = 0; i < TagsProductInformation.size(); ++i)
        {
            ProductInfo.Tags[i] = TagsProductInformation[i];
        }
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyProductDto missing Tags");
    }

    if (Dto.HasMedia())
    {
        auto MediaProductInformation = Dto.GetMedia();

        ProductInfo.Media = common::Array<ProductMediaInfo>(MediaProductInformation.size());

        for (int i = 0; i < MediaProductInformation.size(); ++i)
        {
            if (MediaProductInformation[i]->HasAlt())
            {
                ProductInfo.Media[i].Alt = MediaProductInformation[i]->GetAlt();
            }

            if (MediaProductInformation[i]->HasUrl())
            {
                ProductInfo.Media[i].Url = MediaProductInformation[i]->GetUrl();
            }

            if (MediaProductInformation[i]->HasMediaContentType())
            {
                ProductInfo.Media[i].MediaContentType = MediaProductInformation[i]->GetMediaContentType();
            }

            if (MediaProductInformation[i]->HasWidth())
            {
                ProductInfo.Media[i].Width = MediaProductInformation[i]->GetWidth();
            }

            if (MediaProductInformation[i]->HasHeight())
            {
                ProductInfo.Media[i].Height = MediaProductInformation[i]->GetHeight();
            }
        }
    }
    {
        CSP_LOG_ERROR_MSG("ShopifyProductDto missing Media");
    }
}

void ProductInfoDtoToProductInfoVariantCollection(
    const std::vector<chs_aggregation::ShopifyProductDto>& DtoArray, csp::common::Array<ProductInfo>& ProductInfoCollection)
{
    ProductInfoCollection = common::Array<csp::systems::ProductInfo>(ProductInfoCollection.Size());
    int TotalVariantIndex = 0; // Count the total number of variants included in the product Dto's

    for (int DtoCount = 0; DtoCount < DtoArray.size(); DtoCount++) // Loop all Dto's (products)
    {
        const chs_aggregation::ShopifyProductDto& Dto = DtoArray[DtoCount];

        if (Dto.HasVariants()) // if there are no variants, we don't process the dto (shouldn't happen)
        {
            for (int VariantCount = 0; VariantCount < Dto.GetVariants().size();
                 VariantCount++) // Loop each variant in the product dto and store the info into the indexed output array
            {
                auto VariantProductInformation = Dto.GetVariants()[VariantCount];

                ProductInfoCollection[TotalVariantIndex].Id = Dto.GetId();
                ProductInfoCollection[TotalVariantIndex].Title = Dto.GetTitle();

                if (Dto.HasCreatedAt())
                {
                    ProductInfoCollection[TotalVariantIndex].CreatedAt = Dto.GetCreatedAt();
                }

                if (Dto.HasDescription())
                {
                    ProductInfoCollection[TotalVariantIndex].Description = Dto.GetDescription();
                }

                // The output should only contain productinfo with single variants in, so we hardcode the index and size of the array here
                ProductInfoCollection[TotalVariantIndex].Variants = common::Array<ProductVariantInfo>(1);
                ProductInfoCollection[TotalVariantIndex].Variants[0].Id = VariantProductInformation->GetId();
                ProductInfoCollection[TotalVariantIndex].Variants[0].Title = VariantProductInformation->GetTitle();

                if (VariantProductInformation->HasImage())
                {
                    auto VariantProductImage = VariantProductInformation->GetImage();

                    if (VariantProductImage->HasMediaContentType())
                    {
                        ProductInfoCollection[TotalVariantIndex].Variants[0].Media.MediaContentType = VariantProductImage->GetMediaContentType();
                    }

                    if (VariantProductImage->HasAlt())
                    {
                        ProductInfoCollection[TotalVariantIndex].Variants[0].Media.Alt = VariantProductImage->GetAlt();
                    }

                    if (VariantProductImage->HasUrl())
                    {
                        ProductInfoCollection[TotalVariantIndex].Variants[0].Media.Url = VariantProductImage->GetUrl();
                    }

                    if (VariantProductImage->HasWidth())
                    {
                        ProductInfoCollection[TotalVariantIndex].Variants[0].Media.Width = VariantProductImage->GetWidth();
                    }

                    if (VariantProductImage->HasHeight())
                    {
                        ProductInfoCollection[TotalVariantIndex].Variants[0].Media.Height = VariantProductImage->GetHeight();
                    }
                }
                else
                {
                    CSP_LOG_MSG(LogLevel::Log, "ShopifyProductDto missing Image");
                }

                if (VariantProductInformation->HasSelectedOptions())
                {
                    auto VariantOptionInformation = VariantProductInformation->GetSelectedOptions();

                    ProductInfoCollection[TotalVariantIndex].Variants[0].Options = common::Array<VariantOptionInfo>(VariantOptionInformation.size());

                    for (int n = 0; n < VariantOptionInformation.size(); ++n)
                    {
                        ProductInfoCollection[TotalVariantIndex].Variants[0].Options[n].Name = VariantOptionInformation[n]->GetOptionName();
                        ProductInfoCollection[TotalVariantIndex].Variants[0].Options[n].Value = VariantOptionInformation[n]->GetOptionValue();
                    }
                }
                else
                {
                    CSP_LOG_MSG(LogLevel::Log, "ShopifyProductDto missing SelectedOptions");
                }

                if (VariantProductInformation->HasUnitPrice())
                {
                    if (VariantProductInformation->GetUnitPrice()->HasAmount())
                    {
                        ProductInfoCollection[TotalVariantIndex].Variants[0].UnitPrice.Amount
                            = VariantProductInformation->GetUnitPrice()->GetAmount();
                    }

                    if (VariantProductInformation->GetUnitPrice()->HasCurrencyCode())
                    {
                        ProductInfoCollection[TotalVariantIndex].Variants[0].UnitPrice.CurrencyCode
                            = VariantProductInformation->GetUnitPrice()->GetCurrencyCode();
                    }
                }
                else
                {
                    CSP_LOG_MSG(LogLevel::Log, "ShopifyProductDto missing UnitPrice");
                }

                if (VariantProductInformation->HasQuantityAvailable())
                {
                    ProductInfoCollection[TotalVariantIndex].Variants[0].AvailableStock = VariantProductInformation->GetQuantityAvailable();
                }
                else
                {
                    CSP_LOG_MSG(LogLevel::Log, "ShopifyProductDto missing QuantityAvailable");
                }

                if (Dto.HasTags())
                {
                    auto TagsProductInformation = Dto.GetTags();

                    ProductInfoCollection[TotalVariantIndex].Tags = common::Array<common::String>(TagsProductInformation.size());

                    for (int j = 0; j < TagsProductInformation.size(); ++j)
                    {
                        ProductInfoCollection[TotalVariantIndex].Tags[j] = TagsProductInformation[j];
                    }
                }
                else
                {
                    CSP_LOG_MSG(LogLevel::Log, "ShopifyProductDto missing Tags");
                }

                if (Dto.HasMedia())
                {
                    auto MediaProductInformation = Dto.GetMedia();

                    ProductInfoCollection[TotalVariantIndex].Media = common::Array<ProductMediaInfo>(MediaProductInformation.size());

                    for (int j = 0; j < MediaProductInformation.size(); ++j)
                    {
                        if (MediaProductInformation[j]->HasAlt())
                        {
                            ProductInfoCollection[TotalVariantIndex].Media[j].Alt = MediaProductInformation[j]->GetAlt();
                        }

                        if (MediaProductInformation[j]->HasUrl())
                        {
                            ProductInfoCollection[TotalVariantIndex].Media[j].Url = MediaProductInformation[j]->GetUrl();
                        }

                        if (MediaProductInformation[j]->HasMediaContentType())
                        {
                            ProductInfoCollection[TotalVariantIndex].Media[j].MediaContentType = MediaProductInformation[j]->GetMediaContentType();
                        }

                        if (MediaProductInformation[j]->HasWidth())
                        {
                            ProductInfoCollection[TotalVariantIndex].Media[j].Width = MediaProductInformation[j]->GetWidth();
                        }

                        if (MediaProductInformation[j]->HasHeight())
                        {
                            ProductInfoCollection[TotalVariantIndex].Media[j].Height = MediaProductInformation[j]->GetHeight();
                        }
                    }
                }
                else
                {
                    CSP_LOG_MSG(LogLevel::Log, "ShopifyProductDto missing Media");
                }

                TotalVariantIndex++;
            }
        }
        else
        {
            CSP_LOG_MSG(LogLevel::Log, "ShopifyProductDto missing Variants");
        }
    }
}

void CartDtoToCartInfo(const chs_aggregation::ShopifyCartDto& CartDto, csp::systems::CartInfo& Cart)
{
    if (CartDto.HasSpaceId())
    {
        Cart.SpaceId = CartDto.GetSpaceId();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyCartDto missing SpaceId");
    }

    if (CartDto.HasShopifyCartId())
    {
        // Magnopus Services adds a prefix to the Shopify cart ID. We strip that out here to ensure we are only
        // using the raw Shopify ID. Magnopus Services accepts the ID with or without the prefix so the latter
        // is chosen to be more generic if used with other cloud service providers.
        std::regex CartIdPrefixRegex("^gid:\\/\\/shopify\\/Cart\\/");
        Cart.CartId = std::regex_replace(CartDto.GetShopifyCartId().c_str(), CartIdPrefixRegex, "").c_str();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyCartDto missing ShopifyCartId");
    }

    if (CartDto.HasLines())
    {
        auto DtoLines = CartDto.GetLines();
        Cart.CartLines = csp::common::Array<csp::systems::CartLine>(DtoLines.size());

        for (auto i = 0; i < DtoLines.size(); ++i)
        {
            auto CartLineDto = *DtoLines[i];

            Cart.CartLines[i] = csp::systems::CartLine();

            if (CartLineDto.HasShopifyCartLineId())
            {
                Cart.CartLines[i].CartLineId = CartLineDto.GetShopifyCartLineId();
            }
            else
            {
                CSP_LOG_ERROR_MSG("ShopifyCartLineDto missing ShopifyCartLineId");
            }

            if (CartLineDto.HasProductVariantId())
            {
                Cart.CartLines[i].ProductVariantId = CartLineDto.GetProductVariantId();
            }
            else
            {
                CSP_LOG_ERROR_MSG("ShopifyCartLineDto missing ProductVariantId");
            }

            if (CartLineDto.HasQuantity())
            {
                Cart.CartLines[i].Quantity = CartLineDto.GetQuantity();
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

    if (CartDto.HasTotalQuantity())
    {
        Cart.TotalQuantity = CartDto.GetTotalQuantity();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyCartDto missing TotalQuantity");
    }
}

void ShopifyStoreDtoToShopifyStoreInfo(const chs_aggregation::ShopifyStorefrontDto& StoreDto, csp::systems::ShopifyStoreInfo& Store)
{
    if (StoreDto.HasId())
    {
        Store.StoreId = StoreDto.GetId();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing StoreId");
    }

    if (StoreDto.HasStoreName())
    {
        Store.StoreName = StoreDto.GetStoreName();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing StoreName");
    }

    if (StoreDto.HasSpaceOwnerId())
    {
        Store.SpaceOwnerId = StoreDto.GetSpaceOwnerId();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing SpaceOwnerId");
    }

    if (StoreDto.HasSpaceId())
    {
        Store.SpaceId = StoreDto.GetSpaceId();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing SpaceId");
    }

    if (StoreDto.HasIsEcommerceActive())
    {
        Store.IsEcommerceActive = StoreDto.GetIsEcommerceActive();
    }
    else
    {
        CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing IsEcommerceActive");
    }
}

void ShopifyStoreDtoArrayToShopifyStoreInfoArray(
    const std::vector<chs_aggregation::ShopifyStorefrontDto>& StoreDtos, csp::common::Array<csp::systems::ShopifyStoreInfo>& Stores)
{
    for (int i = 0; i < StoreDtos.size(); i++)
    {
        const chs_aggregation::ShopifyStorefrontDto& StoreDto = StoreDtos[i];
        csp::systems::ShopifyStoreInfo& Store = Stores[i];

        if (StoreDto.HasId())
        {
            Store.StoreId = StoreDto.GetId();
        }
        else
        {
            CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing StoreId");
        }

        if (StoreDto.HasStoreName())
        {
            Store.StoreName = StoreDto.GetStoreName();
        }
        else
        {
            CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing StoreName");
        }

        if (StoreDto.HasSpaceOwnerId())
        {
            Store.SpaceOwnerId = StoreDto.GetSpaceOwnerId();
        }
        else
        {
            CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing SpaceOwnerId");
        }

        if (StoreDto.HasSpaceId())
        {
            Store.SpaceId = StoreDto.GetSpaceId();
        }
        else
        {
            CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing SpaceId");
        }

        if (StoreDto.HasIsEcommerceActive())
        {
            Store.IsEcommerceActive = StoreDto.GetIsEcommerceActive();
        }
        else
        {
            CSP_LOG_ERROR_MSG("ShopifyStorefrontDto missing IsEcommerceActive");
        }
    }
}

const ProductInfo& ProductInfoResult::GetProductInfo() const { return ProductInformation; }

ProductInfo& ProductInfoResult::GetProductInfo() { return ProductInformation; }

void ProductInfoResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    chs_aggregation::ShopifyProductDto* ProductInformationResponse = static_cast<chs_aggregation::ShopifyProductDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        ProductInformationResponse->FromJson(Response->GetPayload().GetContent());
        ProductInfoDtoToProductInfo(*ProductInformationResponse, ProductInformation);
    }
}

const CheckoutInfo& CheckoutInfoResult::GetCheckoutInfo() const { return CheckoutInformation; }

CheckoutInfo& CheckoutInfoResult::GetCheckoutInfo() { return CheckoutInformation; }

void CheckoutInfoResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    chs_aggregation::ShopifyCheckoutDto* CheckoutInformationResponse = static_cast<chs_aggregation::ShopifyCheckoutDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        CheckoutInformationResponse->FromJson(Response->GetPayload().GetContent());

        if (CheckoutInformationResponse->HasStoreUrl())
        {
            CheckoutInformation.StoreUrl = CheckoutInformationResponse->GetStoreUrl();
        }

        if (CheckoutInformationResponse->HasCheckoutUrl())
        {
            CheckoutInformation.CheckoutUrl = CheckoutInformationResponse->GetCheckoutUrl();
        }
    }
}

const CartInfo& CartInfoResult::GetCartInfo() const { return Cart; }

CartInfo& CartInfoResult::GetCartInfo() { return Cart; }

void CartInfoResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* Dto = static_cast<chs_aggregation::ShopifyCartDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        Dto->FromJson(Response->GetPayload().GetContent());

        CartDtoToCartInfo(*Dto, Cart);
    }
}

const ShopifyStoreInfo& AddShopifyStoreResult::GetShopifyStoreInfo() const { return Store; }

ShopifyStoreInfo& AddShopifyStoreResult::GetShopifyStoreInfo() { return Store; }

void AddShopifyStoreResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* Dto = static_cast<chs_aggregation::ShopifyStorefrontDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        Dto->FromJson(Response->GetPayload().GetContent());

        ShopifyStoreDtoToShopifyStoreInfo(*Dto, Store);
    }
}

void ValidateShopifyStoreResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        ValidateResult = (bool)Response->GetPayload().GetContent();
    }
}

const csp::common::Array<ProductInfo>& ProductInfoCollectionResult::GetProducts() const { return Products; }

csp::common::Array<ProductInfo>& ProductInfoCollectionResult::GetProducts() { return Products; }

void ProductInfoCollectionResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* ProductCollectionResponse = static_cast<csp::services::DtoArray<chs_aggregation::ShopifyProductDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        ProductCollectionResponse->FromJson(Response->GetPayload().GetContent());

        // Extract data from the response into our Products array
        std::vector<chs_aggregation::ShopifyProductDto>& ProductsArray = ProductCollectionResponse->GetArray();

        // Loop through products to count the variants, we want 1 output product per variant
        int VariantCount = 0;
        for (int DtoCount = 0; DtoCount < ProductsArray.size(); DtoCount++)
        {
            VariantCount += ProductsArray[DtoCount].GetVariants().size();
        }

        Products = csp::common::Array<csp::systems::ProductInfo>(VariantCount);

        ProductInfoDtoToProductInfoVariantCollection(ProductsArray, Products);
    }
}

const csp::common::Array<ShopifyStoreInfo>& GetShopifyStoresResult::GetShopifyStores() const { return Stores; }

csp::common::Array<ShopifyStoreInfo>& GetShopifyStoresResult::GetShopifyStores() { return Stores; }

void GetShopifyStoresResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* ShopifyStoresResponse = static_cast<csp::services::DtoArray<chs_aggregation::ShopifyStorefrontDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        ShopifyStoresResponse->FromJson(Response->GetPayload().GetContent());

        // Extract data from the response into our Stores array
        std::vector<chs_aggregation::ShopifyStorefrontDto>& StoresArray = ShopifyStoresResponse->GetArray();

        Stores = csp::common::Array<csp::systems::ShopifyStoreInfo>(StoresArray.size());

        ShopifyStoreDtoArrayToShopifyStoreInfoArray(StoresArray, Stores);
    }
}
} // namespace csp::systems
