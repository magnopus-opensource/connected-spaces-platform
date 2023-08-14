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
#include "CSP/Systems/ECommerce/ECommerce.h"
#include "CSP/Systems/SystemBase.h"

namespace csp::services
{

class ApiBase;

}


namespace csp::web
{

class WebClient;

}


namespace csp::systems
{
/// @ingroup ECommerce System
/// @brief Public facing system that allows interfacing with CSP's concept of a ECommerce platform.
/// Offers methods for utilising Ecommerce through CSP
class CSP_API CSP_NO_DISPOSE ECommerceSystem : public SystemBase
{
	/** @cond DO_NOT_DOCUMENT */
	friend class SystemsManager;
	/** @endcond */
public:
	ECommerceSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
	CSP_NO_EXPORT ECommerceSystem(csp::web::WebClient* InWebClient);
	~ECommerceSystem();

	/// @brief Get product information from a shopify store within a space
	/// @param SpaceId csp::common::String : space id of product
	/// @param ProductId csp::common::String : Product id of product
	/// @param Callback NullResultCallback : callback when asynchronous task finishes

	CSP_ASYNC_RESULT void GetProductInformation(const common::String& SpaceId, const common::String& ProductId, ProductInfoResultCallback Callback);

	/// @brief Get checkout information from a shopify store within a space
	/// @param SpaceId csp::common::String : space id of the cart
	/// @param CartId csp::common::String : id of Cart being checked out
	/// @param Callback NullResultCallback : callback when asynchronous task finishes

	CSP_ASYNC_RESULT void GetCheckoutInformation(const common::String& SpaceId, const common::String& CartId, CheckoutInfoResultCallback Callback);

	/// @brief Creates a cart for the current user in the given space.
	/// @param SpaceId csp::common::String : ID of the space to create the cart for.
	/// @param Callback CartInfoResultCallback : Callback when asynchronous task finishes
	CSP_ASYNC_RESULT void CreateCart(const common::String& SpaceId, CartInfoResultCallback Callback);

	/// @brief Creates a cart for the current user in the given space.
	/// @param SpaceId csp::common::String : ID of the space to create the cart for.
	/// @param CartId csp::common::String : ID of the cart.
	/// @param Callback CartInfoResultCallback : Callback when asynchronous task finishes
	CSP_ASYNC_RESULT void GetCart(const common::String& SpaceId, const common::String& CartId, CartInfoResultCallback Callback);

private:
	csp::services::ApiBase* ShopifyAPI;
};
} // namespace csp::systems
