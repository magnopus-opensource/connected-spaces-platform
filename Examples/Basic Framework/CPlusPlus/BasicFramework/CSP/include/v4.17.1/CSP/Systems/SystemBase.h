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

namespace csp::web
{

class WebClient;

}

namespace csp::systems
{
/// @brief Base class for all Connected Spaces Platform Systems, which enforces passing of a WebClient instance in the constructor of each System.
class CSP_API CSP_NO_DISPOSE SystemBase
{
protected:
    SystemBase()
        : WebClient(nullptr) {};
    CSP_NO_EXPORT SystemBase(csp::web::WebClient* InWebClient)
        : WebClient(InWebClient) {};

    csp::web::WebClient* WebClient;
};

} // namespace csp::systems
