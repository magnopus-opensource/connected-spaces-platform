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


namespace csp::systems
{

ProductImageInfo::ProductImageInfo(const csp::common::String& AltIn, const csp::common::String& UrlIn, const int64_t WidthIn, const int64_t HeightIn)
	: Alt(AltIn), Url(UrlIn), Width(WidthIn), Height(HeightIn)
{
}

ProductMediaInfo::ProductMediaInfo(const csp::common::String& MediaContentTypeIn,
								   const csp::common::String& AltIn,
								   const csp::common::String& UrlIn,
								   const int64_t WidthIn,
								   const int64_t HeightIn)
	: MediaContentType(MediaContentTypeIn), Alt(AltIn), Url(UrlIn), Width(WidthIn), Height(HeightIn)
{
}

ProductInfo::ProductInfo(const csp::common::String& IdIn,
						 const csp::common::String& TitleIn,
						 bool AvailableForSaleIn,
						 const ProductImageInfo& ImageIn,
						 const csp::common::Map<csp::common::String, csp::common::String>& SelectedOptionsIn,
						 double UnitPriceIn,
						 const ProductMediaInfo& MediaIn)
	: Id(IdIn)
	, Title(TitleIn)
	, AvailableForSale(AvailableForSaleIn)
	, Image(ImageIn)
	, SelectedOptions(SelectedOptionsIn)
	, UnitPrice(UnitPriceIn)
	, Media(MediaIn)
{
}

} // namespace csp::systems
