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

ProductMediaInfo::ProductMediaInfo(const csp::common::String& MediaContentTypeIn, const csp::common::String& AltIn, const csp::common::String& UrlIn)
	: MediaContentType(MediaContentTypeIn), Alt(AltIn), Url(UrlIn)
{
}

ProductInfo::ProductInfo(const csp::common::String& IdIn,
						 const csp::common::String& TitleIn,
						 const csp::common::String& CreatedAtIn,
						 const csp::common::Array<common::String>& TagsIn,
						 const csp::common::Map<csp::common::String, csp::common::String>& VariantsIn,
						 const csp::common::Array<ProductMediaInfo>& MediaIn)
	: Id(IdIn), Title(TitleIn), CreatedAt(CreatedAtIn), Tags(TagsIn), Variants(VariantsIn), Media(MediaIn)
{
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

		ProductInformation.Id		   = ProductInformationResponse->GetId();
		ProductInformation.Title	   = ProductInformationResponse->GetTitle();
		ProductInformation.Description = ProductInformationResponse->GetDescription();
		ProductInformation.CreatedAt   = ProductInformationResponse->GetCreatedAt();

		auto VariantProductInformation = ProductInformationResponse->GetVariants();

		for (int i = 0; i < VariantProductInformation.size(); ++i)
		{
			ProductInformation.Variants[VariantProductInformation[i]->GetId()] = VariantProductInformation[i]->GetTitle();
		}

		auto TagsProductInformation = ProductInformationResponse->GetTags();

		ProductInformation.Tags = common::Array<common::String>(TagsProductInformation.size());

		for (int i = 0; i < TagsProductInformation.size(); ++i)
		{
			ProductInformation.Tags[i] = TagsProductInformation[i];
		}

		auto MediaProductInformation = ProductInformationResponse->GetMedia();

		ProductInformation.Media = common::Array<ProductMediaInfo>(MediaProductInformation.size());

		for (int i = 0; i < MediaProductInformation.size(); ++i)
		{
			ProductInformation.Media[i].Alt				 = MediaProductInformation[i]->GetAlt();
			ProductInformation.Media[i].Url				 = MediaProductInformation[i]->GetUrl();
			ProductInformation.Media[i].MediaContentType = MediaProductInformation[i]->GetMediaContentType();
		}
	}
}

} // namespace csp::systems
