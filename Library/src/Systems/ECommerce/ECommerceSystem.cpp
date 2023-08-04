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
#include "CSP/Systems/ECommerce/ECommerceSystem.h"

#include "Services/aggregationservice/Api.h"

using namespace csp;

namespace chs = csp::services::generated::aggregationservice;


namespace csp::systems
{
ECommerceSystem::ECommerceSystem() : SystemBase(), ShopifyAPI(nullptr)
{
}

ECommerceSystem::ECommerceSystem(csp::web::WebClient* InWebClient) : SystemBase(InWebClient)
{
	ShopifyAPI = CSP_NEW chs::ShopifyApi(InWebClient);
}

ECommerceSystem::~ECommerceSystem()
{
	CSP_DELETE(ShopifyAPI);
}

void ECommerceSystem::GetProductInformation(const common::String& SpaceId, const common::String& ProductId, ProductInfoResultCallback Callback)
{

	csp::services::ResponseHandlerPtr ResponseHandler
		= ShopifyAPI->CreateHandler<ProductInfoResultCallback, ProductInfoResult, void, chs::ShopifyProductDto>(
			Callback,
			nullptr,
			csp::web::EResponseCodes::ResponseCreated);

	static_cast<chs::ShopifyApi*>(ShopifyAPI)->apiV1SpacesSpaceIdVendorsShopifyProductsProductIdGet(SpaceId, ProductId, ResponseHandler);
}
} // namespace csp::systems
