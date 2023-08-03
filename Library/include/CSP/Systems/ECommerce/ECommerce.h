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
#include "CSP/Services/WebService.h"
#include "Common/DateTime.h"

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
	ProductMediaInfo(const csp::common::String& MediaContentTypeIn, const csp::common::String& AltIn, const csp::common::String& UrlIn);

	/// @brief Type of media content used.
	csp::common::String MediaContentType;
	/// @brief Alternative description of the media.
	csp::common::String Alt;
	/// @brief Url of the media.
	csp::common::String Url;
};

/// @ingroup ECommerce System
/// @brief Represents a single product and the information associated with it.
class CSP_API ProductInfo
{
public:
	ProductInfo() = default;
	ProductInfo(const csp::common::String& IdIn,
				const csp::common::String& TitleIn,
				const csp::common::DateTime CreatedAtIn,
				const csp::common::Array<common::String>& TagsIn,
				csp::common::Map<common::String, common::String> VariantsIn,
				const csp::common::Array<ProductMediaInfo>& MediaIn);

	/// @brief Id of the product.
	csp::common::String Id;
	/// @brief Title of the product.
	csp::common::String Title;
	/// @brief  Time the product was created.
	csp::common::DateTime CreatedAt;
	/// @brief Description of the product.
	csp::common::String Description;
	/// @brief map of product variants.
	csp::common::Map<common::String, common::String> Variants;
	/// @brief array of product tags.
	csp::common::Array<common::String> Tags;
	/// @brief This array holds object of additional media for the product
	csp::common::Array<ProductMediaInfo> Media;
};

/// @ingroup ECommerce System
/// @brief Data class used to contain information when attempting to get Product Info.
class CSP_API ProductInfoResult : public csp::services::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	CSP_END_IGNORE
	/** @endcond */

public:
	/// @brief Retrieves the Product Info being stored.
	/// @return ProductInfo : reference to the ProductInfo
	const ProductInfo& GetProductInfo() const;

	/// @brief Retrieves the Product Info being stored.
	/// @return ProductInfo : reference to the ProductInfo
	ProductInfo& GetProductInfo();

private:
	ProductInfoResult(void*) {};

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	ProductInfo ProductInformation;
};

typedef std::function<void(const ProductInfoResult& Result)> ProductInfoResultCallback;

} // namespace csp::systems
