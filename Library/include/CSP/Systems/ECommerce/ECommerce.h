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
#pragma once
#include "CSP/CSPCommon.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"

namespace csp::services
{

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services



namespace csp::systems
{

/// @ingroup ECommerce System
/// @brief Represents media information for a product such as additional images
class CSP_API ProductMediaInfo
{
public:
	ProductMediaInfo() = default;
	ProductMediaInfo(const csp::common::String& MediaContentTypeIn,
					 const csp::common::String& AltIn,
					 const csp::common::String& UrlIn,
					 const int64_t WidthIn,
					 const int64_t HeightIn);

	csp::common::String MediaContentType; // Type of media content used.
	csp::common::String Alt;			  // Alternative description of the media.
	csp::common::String Url;			  // Url of the media.
	int64_t Width;						  // Width fo the media.
	int64_t Height;						  // Height of the media.
};

/// @ingroup ECommerce System
/// @brief Represents image information for a product
class CSP_API ProductImageInfo
{
public:
	ProductImageInfo() = default;
	ProductImageInfo(const csp::common::String& AltIn, const csp::common::String& UrlIn, const int64_t WidthIn, const int64_t HeightIn);

	csp::common::String Alt; // Alternative description of the image.
	csp::common::String Url; // Url of the image.
	int64_t Width;			 // Width fo the image.
	int64_t Height;			 // Height of the Image.
};

/// @ingroup ECommerce System
/// @brief Represents a single product and the information associated with it.
class CSP_API ProductInfo
{
public:
	ProductInfo(const csp::common::String& IdIn,
				const csp::common::String& TitleIn,
				bool AvailableForSaleIn,
				const ProductImageInfo& ImageIn,
				const csp::common::Map<csp::common::String, csp::common::String>& SelectedOptionsIn,
				double UnitPriceIn,
				const csp::common::Array<ProductMediaInfo>& Media);


	csp::common::String Id;														// Id of the product.
	csp::common::String Title;													// Title of the product.
	bool AvailableForSale;														// Flag showing if the Product is available.
	ProductImageInfo Image;														// The object holding Information about the Product image.
	csp::common::Map<csp::common::String, csp::common::String> SelectedOptions; // map of product options available for selection.
	double UnitPrice;															// price of the product.
	csp::common::Array<ProductMediaInfo> Media;									// This array holds object of additional media for the product
};


} // namespace csp::systems
