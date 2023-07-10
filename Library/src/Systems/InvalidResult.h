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


#include <type_traits>


namespace csp::services
{

class ResultBase;

}


namespace csp::systems
{

template <typename T> struct InvalidResult
{
	static_assert(std::is_base_of<csp::services::ResultBase, T>::value, "Template type must derive from `csp::services::ResultBase`");

	static T Get()
	{
		static T Result(csp::services::EResultCode::Failed, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseBadRequest));

		return Result;
	}
};

} // namespace csp::systems
