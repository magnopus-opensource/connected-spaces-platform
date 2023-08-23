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

namespace csp::services
{

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services



namespace csp::systems
{
/// @ingroup ECommerce System
/// @brief Represents currency information for a product
class CSP_API CurrencyInfo
{
public:
	CurrencyInfo() = default;

	/// @brief Currency Amount.
	double Amount;
	/// @brief Currency type
	csp::common::String CurrencyCode;
};

/// @ingroup ECommerce System
/// @brief Represents media information for a product
class CSP_API ProductMediaInfo
{
public:
	ProductMediaInfo() = default;

	/// @brief Type of media content used.
	csp::common::String MediaContentType;
	/// @brief Alternative description of the media.
	csp::common::String Alt;
	/// @brief Url of the media.
	csp::common::String Url;
	/// @brief Width of the media.
	int32_t Width;
	/// @brief Height of the media.
	int32_t Height;
};

/// @ingroup ECommerce System
/// @brief Represents option for a variant
class CSP_API VariantOptionInfo
{
public:
	VariantOptionInfo() = default;

	/// @brief Id of the variant option.
	csp::common::String Name;
	/// @brief Value of variant option.
	csp::common::String Value;
};

/// @ingroup ECommerce System
/// @brief Represents variant information for a product
class CSP_API ProductVariantInfo
{
public:
	ProductVariantInfo() = default;
	/// @brief Id of the variant.
	csp::common::String Id;
	/// @brief Title of the variant.
	csp::common::String Title;
	/// @brief Url of variant.
	csp::common::String Url;
	/// @brief Is variant available for sale.
	bool AvailableForSale;
	/// @brief Media for a variant
	ProductMediaInfo Media;
	/// @brief Additional options for variant
	csp::common::Array<VariantOptionInfo> Options;
	/// @brief Unit price for the variant
	CurrencyInfo UnitPrice;
};


/// @ingroup ECommerce System
/// @brief Represents a single product and the information associated with it.
class CSP_API ProductInfo
{
public:
	ProductInfo() = default;

	/// @brief Id of the product.
	csp::common::String Id;
	/// @brief Title of the product.
	csp::common::String Title;
	/// @brief Time the product was created.
	csp::common::String CreatedAt;
	/// @brief Description of the product.
	csp::common::String Description;
	/// @brief Array of product variants.
	csp::common::Array<ProductVariantInfo> Variants;
	/// @brief Array of product tags.
	csp::common::Array<csp::common::String> Tags;
	/// @brief This array holds media for the product
	csp::common::Array<ProductMediaInfo> Media;
};

/// @ingroup ECommerce System
/// @brief Represents Checkout information for a product
class CSP_API CheckoutInfo
{
public:
	CheckoutInfo() = default;
	/// @brief Url of the Store.
	csp::common::String StoreUrl;
	/// @brief Url of Checkout.
	csp::common::String CheckoutUrl;
};

/// @ingroup ECommerce System
/// @brief Represents a line in a cart.
class CSP_API CartLine
{
public:
	CartLine() : Quantity(0) {};

	/// @brief ID of the line in the cart.
	csp::common::String CartLineId;

	/// @brief ID of the variant of the product.
	csp::common::String ProductVariantId;

	/// @brief Quantity of the product in the cart.
	int Quantity;
};

/// @ingroup ECommerce System
/// @brief Represents a cart.
class CSP_API CartInfo
{
public:
	CartInfo() : TotalQuantity(0) {};

	/// @brief Space that the cart is associated with.
	csp::common::String SpaceId;

	/// @brief ID of the cart.
	csp::common::String CartId;

	/// @brief An array of the lines in the cart.
	csp::common::Array<CartLine> CartLines;

	/// @brief Total quantity of all lines in the cart.
	int TotalQuantity;
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

/// @ingroup ECommerce System
/// @brief Data class used to contain information when attempting to get Checkout Info.
class CSP_API CheckoutInfoResult : public csp::services::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	CSP_END_IGNORE
	/** @endcond */

public:
	/// @brief Retrieves the Checkout Info being stored.
	/// @return ProductInfo : reference to the CheckoutInfo
	const CheckoutInfo& GetCheckoutInfo() const;

	/// @brief Retrieves the Checkout Info being stored.
	/// @return ProductInfo : reference to the CheckoutInfo
	CheckoutInfo& GetCheckoutInfo();

private:
	CheckoutInfoResult(void*) {};

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	CheckoutInfo CheckoutInformation;
};


/// @ingroup ECommerce System
/// @brief Data class used to contain information when attempting to get a Cart.
class CSP_API CartInfoResult : public csp::services::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	friend class ECommerceSystem;

	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	CSP_END_IGNORE
	/** @endcond */

public:
	/// @brief Retrieves the Cart Info being stored.
	/// @return ProductInfo : reference to the CartInfo
	const CartInfo& GetCartInfo() const;

	/// @brief Retrieves the Cart Info being stored.
	/// @return ProductInfo : reference to the CartInfo
	CartInfo& GetCartInfo();

	CSP_NO_EXPORT static CartInfoResult Invalid();

private:
	CartInfoResult(void*) {};
	CartInfoResult(csp::services::EResultCode ResCode, uint16_t HttpResCode) : csp::services::ResultBase(ResCode, HttpResCode) {};

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	CartInfo Cart;
};

typedef std::function<void(const ProductInfoResult& Result)> ProductInfoResultCallback;

typedef std::function<void(const CheckoutInfoResult& Result)> CheckoutInfoResultCallback;

typedef std::function<void(const CartInfoResult& Result)> CartInfoResultCallback;

} // namespace csp::systems
