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
				if (VariantProductInformation[i]->GetImage()->HasMediaContentType())
				{
					ProductInfo.Variants[i].Media.MediaContentType = VariantProductInformation[i]->GetImage()->GetMediaContentType();
				}

				ProductInfo.Variants[i].Media.Alt	 = VariantProductInformation[i]->GetImage()->GetAlt();
				ProductInfo.Variants[i].Media.Url	 = VariantProductInformation[i]->GetImage()->GetUrl();
				ProductInfo.Variants[i].Media.Width	 = VariantProductInformation[i]->GetImage()->GetWidth();
				ProductInfo.Variants[i].Media.Height = VariantProductInformation[i]->GetImage()->GetHeight();
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
			ProductInfo.Media[i].Alt			  = MediaProductInformation[i]->GetAlt();
			ProductInfo.Media[i].Url			  = MediaProductInformation[i]->GetUrl();
			ProductInfo.Media[i].MediaContentType = MediaProductInformation[i]->GetMediaContentType();
			ProductInfo.Media[i].Width			  = MediaProductInformation[i]->GetWidth();
			ProductInfo.Media[i].Height			  = MediaProductInformation[i]->GetHeight();
		}
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

} // namespace csp::systems
