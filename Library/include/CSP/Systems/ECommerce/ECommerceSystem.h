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
/// Offers methods for utilising shopify through CSP
class CSP_API CSP_NO_DISPOSE ECommerceSystem : public SystemBase
{
	/** @cond DO_NOT_DOCUMENT */
	friend class SystemsManager;
	/** @endcond */
public:
	ECommerceSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
	CSP_NO_EXPORT ECommerceSystem(csp::web::WebClient* InWebClient);
	~ECommerceSystem();

private:
	csp::services::ApiBase* ShopifyAPI;
};
} // namespace csp::systems
