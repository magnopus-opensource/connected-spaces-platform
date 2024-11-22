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

#include "CSP/Systems/Users/UserSystem.h"
#include "CallHelpers.h"
#include "Common/Convert.h"
#include "ECommerceSystemHelpers.h"
#include "Services/aggregationservice/Api.h"
#include "Systems/ResultHelpers.h"

#include <array>


using namespace csp;

namespace chs = csp::services::generated::aggregationservice;


namespace
{

void RemoveUrl(csp::common::String& Url)
{
	if (std::string(Url.c_str()).find("gid://shopify/Cart/") != std::string::npos)
	{
		Url = Url.Split('/')[Url.Split('/').Size() - 1];
	}
	else if (std::string(Url.c_str()).find("?cart=") != std::string::npos)
	{
		Url = Url.Split('/')[Url.Split('/').Size() - 1].c_str();
	}
	else if (std::string(Url.c_str()).find("gid://shopify/ProductVariant/") != std::string::npos)
	{
		Url = Url.Split('/')[Url.Split('/').Size() - 1];
	}
}

} // namespace


namespace csp::systems
{

ECommerceSystem::ECommerceSystem() : SystemBase(), ShopifyAPI(nullptr)
{
}

ECommerceSystem::ECommerceSystem(csp::web::WebClient* InWebClient) : SystemBase(InWebClient)
{
	ShopifyAPI = new chs::ShopifyApi(InWebClient);
}

ECommerceSystem::~ECommerceSystem()
{
	delete (ShopifyAPI);
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

void ECommerceSystem::GetProductInfoCollectionByVariantIds(const common::String& SpaceId,
														   const Array<common::String>& InVariantIds,
														   ProductInfoCollectionResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= ShopifyAPI->CreateHandler<ProductInfoCollectionResultCallback,
									ProductInfoCollectionResult,
									void,
									csp::services::DtoArray<chs::ShopifyProductDto>>(Callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

	const std::vector<common::String> VariantIds = common::Convert(InVariantIds);

	static_cast<chs::ShopifyApi*>(ShopifyAPI)->apiV1SpacesSpaceIdVendorsShopifyProductsVariantsGet(SpaceId, VariantIds, ResponseHandler);
}

void ECommerceSystem::GetCheckoutInformation(const common::String& SpaceId, const common::String& CartId, CheckoutInfoResultCallback Callback)
{
	std::string CharacterCheck = CartId.c_str();
	if (CharacterCheck.find("/") != std::string::npos || CharacterCheck.find(R"(\)") != std::string::npos)
	{
		CSP_LOG_ERROR_MSG("Call to GetCheckoutInformation failed. CartId must NOT include path characters forward or back slash.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<CheckoutInfoResult>());

		return;
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= ShopifyAPI->CreateHandler<CheckoutInfoResultCallback, CheckoutInfoResult, void, chs::ShopifyCheckoutDto>(
			Callback,
			nullptr,
			csp::web::EResponseCodes::ResponseCreated);

	static_cast<chs::ShopifyApi*>(ShopifyAPI)->apiV1SpacesSpaceIdVendorsShopifyCartsCartIdCheckoutInfoGet(SpaceId, CartId, ResponseHandler);
}

void ECommerceSystem::CreateCart(const common::String& SpaceId, CartInfoResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= ShopifyAPI->CreateHandler<CartInfoResultCallback, CartInfoResult, void, chs::ShopifyCartDto>(Callback,
																									   nullptr,
																									   csp::web::EResponseCodes::ResponseCreated);

	static_cast<chs::ShopifyApi*>(ShopifyAPI)->apiV1SpacesSpaceIdVendorsShopifyCartsPost(SpaceId, ResponseHandler);
}

void ECommerceSystem::GetCart(const common::String& SpaceId, const common::String& CartId, CartInfoResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= ShopifyAPI->CreateHandler<CartInfoResultCallback, CartInfoResult, void, chs::ShopifyCartDto>(Callback,
																									   nullptr,
																									   csp::web::EResponseCodes::ResponseCreated);

	static_cast<chs::ShopifyApi*>(ShopifyAPI)->apiV1SpacesSpaceIdVendorsShopifyCartsCartIdGet(SpaceId, CartId, ResponseHandler);
}

void ECommerceSystem::GetShopifyStores(const csp::common::Optional<bool>& IsActive, GetShopifyStoresResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= ShopifyAPI->CreateHandler<GetShopifyStoresResultCallback, GetShopifyStoresResult, void, csp::services::DtoArray<chs::ShopifyStorefrontDto>>(
			Callback,
			nullptr,
			csp::web::EResponseCodes::ResponseCreated);

	std::optional<bool> ActiveParam;

	if (IsActive.HasValue())
	{
		ActiveParam = *IsActive;
	}

	auto& SystemsManager   = SystemsManager::Get();
	const auto* UserSystem = SystemsManager.GetUserSystem();

	const auto& UserId = UserSystem->GetLoginState().UserId;

	static_cast<chs::ShopifyApi*>(ShopifyAPI)
		->apiV1VendorsShopifyUsersUserIdStorefrontsGet(UserId, ActiveParam, std::nullopt, std::nullopt, ResponseHandler);
}

void ECommerceSystem::AddShopifyStore(const common::String& StoreName,
									  const common::String& SpaceId,
									  const bool IsEcommerceActive,
									  const common::String& PrivateAccessToken,
									  AddShopifyStoreResultCallback Callback)
{
	auto ShopifyStorefrontInfo = systems::ECommerceSystemHelpers::DefaultShopifyStorefrontInfo();
	ShopifyStorefrontInfo->SetStoreName(StoreName);
	ShopifyStorefrontInfo->SetIsEcommerceActive(IsEcommerceActive);
	ShopifyStorefrontInfo->SetPrivateAccessToken(PrivateAccessToken);

	csp::services::ResponseHandlerPtr ResponseHandler
		= ShopifyAPI->CreateHandler<AddShopifyStoreResultCallback, AddShopifyStoreResult, void, chs::ShopifyStorefrontDto>(
			Callback,
			nullptr,
			csp::web::EResponseCodes::ResponseCreated);

	static_cast<chs::ShopifyApi*>(ShopifyAPI)->apiV1SpacesSpaceIdVendorsShopifyPut(SpaceId, ShopifyStorefrontInfo, ResponseHandler);
}

void ECommerceSystem::SetECommerceActiveInSpace(const common::String& StoreName,
												const common::String& SpaceId,
												const bool IsEcommerceActive,
												SetECommerceActiveResultCallback Callback)
{
	auto ShopifyStorefrontInfo = systems::ECommerceSystemHelpers::DefaultShopifyStorefrontInfo();
	ShopifyStorefrontInfo->SetStoreName(StoreName);
	ShopifyStorefrontInfo->SetIsEcommerceActive(IsEcommerceActive);

	csp::services::ResponseHandlerPtr ResponseHandler
		= ShopifyAPI->CreateHandler<SetECommerceActiveResultCallback, AddShopifyStoreResult, void, chs::ShopifyStorefrontDto>(
			Callback,
			nullptr,
			csp::web::EResponseCodes::ResponseCreated);

	static_cast<chs::ShopifyApi*>(ShopifyAPI)->apiV1SpacesSpaceIdVendorsShopifyPut(SpaceId, ShopifyStorefrontInfo, ResponseHandler);
}

void ECommerceSystem::ValidateShopifyStore(const common::String& StoreName,
										   const common::String& PrivateAccessToken,
										   ValidateShopifyStoreResultCallback Callback)
{
	auto ShopifyStorefrontValidationInfo = systems::ECommerceSystemHelpers::DefaultShopifyStorefrontValidationRequest();
	ShopifyStorefrontValidationInfo->SetStoreName(StoreName);
	ShopifyStorefrontValidationInfo->SetPrivateAccessToken(PrivateAccessToken);

	csp::services::ResponseHandlerPtr ResponseHandler
		= ShopifyAPI->CreateHandler<ValidateShopifyStoreResultCallback, ValidateShopifyStoreResult, void, chs::ShopifyStorefrontValidationRequest>(
			Callback,
			nullptr,
			csp::web::EResponseCodes::ResponseCreated);

	static_cast<chs::ShopifyApi*>(ShopifyAPI)->apiV1VendorsShopifyValidatePut(ShopifyStorefrontValidationInfo, ResponseHandler);
}

void ECommerceSystem::UpdateCartInformation(const CartInfo& CartInformation, CartInfoResultCallback Callback)
{
	if (CartInformation.SpaceId.IsEmpty() || CartInformation.CartId.IsEmpty())
	{
		CSP_LOG_ERROR_MSG("SpaceId and CartId inside CartInformation must be populated.")

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<CartInfoResult>());

		return;
	}

	auto CartUpdateInfo = std::make_shared<chs::ShopifyCartUpdateDto>();

	if (!CartInformation.CartLines.IsEmpty())
	{
		auto CartLinesRemoval	= std::vector<std::shared_ptr<chs::ShopifyCartLineDto>>();
		auto CartLinesUpdate	= std::vector<std::shared_ptr<chs::ShopifyCartLineDto>>();
		auto CartLinesAdditions = std::vector<std::shared_ptr<chs::ShopifyCartLineDto>>();

		for (int i = 0; i < CartInformation.CartLines.Size(); ++i)
		{

			csp::common::String CartLineId		 = CartInformation.CartLines[i].CartLineId;
			csp::common::String ProductVariantId = CartInformation.CartLines[i].ProductVariantId;
			RemoveUrl(CartLineId);
			RemoveUrl(ProductVariantId);

			if (CartInformation.CartLines[i].Quantity == 0)
			{
				auto CartLineRemoval = std::make_shared<chs::ShopifyCartLineDto>();

				CartLineRemoval->SetShopifyCartLineId(CartLineId);
				CartLineRemoval->SetProductVariantId(ProductVariantId);

				CartLinesRemoval.push_back(CartLineRemoval);
			}
			else if (CartInformation.CartLines[i].CartLineId.IsEmpty() && CartInformation.CartLines[i].Quantity != 0)
			{
				auto CartLineAdditions = std::make_shared<chs::ShopifyCartLineDto>();

				CartLineAdditions->SetQuantity(CartInformation.CartLines[i].Quantity);
				CartLineAdditions->SetShopifyCartLineId(CartLineId);
				CartLineAdditions->SetProductVariantId(ProductVariantId);

				CartLinesAdditions.push_back(CartLineAdditions);
			}
			else
			{
				auto CartLineUpdate = std::make_shared<chs::ShopifyCartLineDto>();

				CartLineUpdate->SetQuantity(CartInformation.CartLines[i].Quantity);
				CartLineUpdate->SetShopifyCartLineId(CartLineId);
				CartLineUpdate->SetProductVariantId(ProductVariantId);

				CartLinesUpdate.push_back(CartLineUpdate);
			}
		}

		CartUpdateInfo->SetAddLineCartChanges(CartLinesAdditions);
		CartUpdateInfo->SetRemoveLineCartChanges(CartLinesRemoval);
		CartUpdateInfo->SetUpdateLineQtyCartChanges(CartLinesUpdate);
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= ShopifyAPI->CreateHandler<CartInfoResultCallback, CartInfoResult, void, chs::ShopifyCartDto>(Callback,
																									   nullptr,
																									   csp::web::EResponseCodes::ResponseCreated);
	static_cast<chs::ShopifyApi*>(ShopifyAPI)
		->apiV1SpacesSpaceIdVendorsShopifyCartsCartIdPut(CartInformation.SpaceId, CartInformation.CartId, CartUpdateInfo, ResponseHandler);
}

} // namespace csp::systems
