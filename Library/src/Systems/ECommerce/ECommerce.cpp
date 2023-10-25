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
	ProductInfo.Id		  = Dto.GetId();
	ProductInfo.Title	  = Dto.GetTitle();
	ProductInfo.CreatedAt = Dto.GetCreatedAt();

	if (Dto.HasDescription())
	{
		ProductInfo.Description = Dto.GetDescription();
	}

	if (Dto.HasVariants())
	{
		auto VariantProductInformation = Dto.GetVariants();
		ProductInfo.Variants		   = common::Array<ProductVariantInfo>(VariantProductInformation.size());

		for (int i = 0; i < VariantProductInformation.size(); ++i)
		{
			ProductInfo.Variants[i].Id				 = VariantProductInformation[i]->GetId();
			ProductInfo.Variants[i].Title			 = VariantProductInformation[i]->GetTitle();
			ProductInfo.Variants[i].AvailableForSale = VariantProductInformation[i]->GetAvailableForSale();
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

			if (Dto.GetVariants()[i]->HasSelectedOptions())
			{
				auto VariantOptionInformation = Dto.GetVariants()[i]->GetSelectedOptions();

				ProductInfo.Variants[i].Options = common::Array<VariantOptionInfo>(VariantOptionInformation.size());

				for (int n = 0; n < VariantOptionInformation.size(); ++n)
				{
					ProductInfo.Variants[i].Options[n].Name	 = VariantOptionInformation[n]->GetOptionName();
					ProductInfo.Variants[i].Options[n].Value = VariantOptionInformation[n]->GetOptionValue();
				}
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
		}
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
		auto DtoLines  = CartDto.GetLines();
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


const ProductInfo& ProductInfoResult::GetProductInfo() const
{
	return ProductInformation;
}

ProductInfo& ProductInfoResult::GetProductInfo()
{
	return ProductInformation;
}

void ProductInfoResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	chs_aggregation::ShopifyProductDto* ProductInformationResponse = static_cast<chs_aggregation::ShopifyProductDto*>(ApiResponse->GetDto());
	const csp::web::HttpResponse* Response						   = ApiResponse->GetResponse();

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		ProductInformationResponse->FromJson(Response->GetPayload().GetContent());
		ProductInfoDtoToProductInfo(*ProductInformationResponse, ProductInformation);
	}
}

const CheckoutInfo& CheckoutInfoResult::GetCheckoutInfo() const
{
	return CheckoutInformation;
}

CheckoutInfo& CheckoutInfoResult::GetCheckoutInfo()
{
	return CheckoutInformation;
}

void CheckoutInfoResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	chs_aggregation::ShopifyCheckoutDto* CheckoutInformationResponse = static_cast<chs_aggregation::ShopifyCheckoutDto*>(ApiResponse->GetDto());
	const csp::web::HttpResponse* Response							 = ApiResponse->GetResponse();

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		CheckoutInformationResponse->FromJson(Response->GetPayload().GetContent());
		CheckoutInformation.StoreUrl	= CheckoutInformationResponse->GetCheckoutUrl();
		CheckoutInformation.CheckoutUrl = CheckoutInformationResponse->GetCheckoutUrl();
	}
}

const CartInfo& CartInfoResult::GetCartInfo() const
{
	return Cart;
}

CartInfo& CartInfoResult::GetCartInfo()
{
	return Cart;
}

CartInfoResult CartInfoResult::Invalid()
{
	static CartInfoResult InvalidResult(csp::services::EResultCode::Failed, 0);

	return InvalidResult;
}

void CartInfoResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	auto* Dto							   = static_cast<chs_aggregation::ShopifyCartDto*>(ApiResponse->GetDto());
	const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		Dto->FromJson(Response->GetPayload().GetContent());

		CartDtoToCartInfo(*Dto, Cart);
	}
}

const ShopifyStoreInfo& AddShopifyStoreResult::GetShopifyStoreInfo() const
{
	return Store;
}

ShopifyStoreInfo& AddShopifyStoreResult::GetShopifyStoreInfo()
{
	return Store;
}

void AddShopifyStoreResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	auto* Dto							   = static_cast<chs_aggregation::ShopifyStorefrontDto*>(ApiResponse->GetDto());
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
		ValidateResult = (bool) Response->GetPayload().GetContent();
	}
}

} // namespace csp::systems
