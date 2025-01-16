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

#include "Services/ApiBase/ApiBase.h"
#include "Services/DtoBase/DtoBase.h"

namespace csp::services
{

class AssetFileDto : public csp::services::DtoBase
{
public:
    AssetFileDto();
    virtual ~AssetFileDto();

    void FromJson(const csp::common::String& Json) override;
};

} // namespace csp::services
