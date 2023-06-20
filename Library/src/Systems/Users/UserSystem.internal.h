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


#include "CSP/Systems/Users/UserSystem.h"


namespace csp::web
{

class WebClient;

}


namespace csp::systems_internal
{

class UserSystem : public csp::systems::UserSystem
{
public:
	inline UserSystem(csp::web::WebClient* InWebClient) : csp::systems::UserSystem(InWebClient)
	{
	}

	inline void RefreshAuthenticationSession(const csp::common::String& UserId,
											 const csp::common::String& RefreshToken,
											 const csp::common::String& DeviceId,
											 const csp::systems::LoginStateResultCallback& Callback)
	{
		csp::systems::UserSystem::RefreshAuthenticationSession(UserId, RefreshToken, DeviceId, Callback);
	}

	inline void NotifyRefreshTokenHasChanged()
	{
		csp::systems::UserSystem::NotifyRefreshTokenHasChanged();
	}
};

} // namespace csp::systems_internal
