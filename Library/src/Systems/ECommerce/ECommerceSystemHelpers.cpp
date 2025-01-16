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
#include "Systems/ECommerce/ECommerceSystemHelpers.h"

#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Debug/Logging.h"

namespace chs = csp::services::generated::aggregationservice;

namespace csp::systems
{

namespace ECommerceSystemHelpers
{
    std::shared_ptr<chs::ShopifyStorefrontDto> DefaultShopifyStorefrontInfo()
    {
        auto DefaultStoreInfo = std::make_shared<chs::ShopifyStorefrontDto>();

        DefaultStoreInfo->SetStoreName("ShopifyStore");
        DefaultStoreInfo->SetIsEcommerceActive(false);
        DefaultStoreInfo->SetPrivateAccessToken("");

        return DefaultStoreInfo;
    }

    std::shared_ptr<chs::ShopifyStorefrontValidationRequest> DefaultShopifyStorefrontValidationRequest()
    {
        auto DefaultShopifyStorefrontValidationInfo = std::make_shared<chs::ShopifyStorefrontValidationRequest>();

        DefaultShopifyStorefrontValidationInfo->SetStoreName("");
        DefaultShopifyStorefrontValidationInfo->SetPrivateAccessToken("");

        return DefaultShopifyStorefrontValidationInfo;
    }

} // namespace ECommerceSystemHelpers
} // namespace csp::systems
