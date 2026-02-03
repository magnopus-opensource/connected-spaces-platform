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

#include "CSP/Systems/Assets/AssetCollection.h"
#include "signalrclient/signalr_value.h"

#include "Services/ApiBase/ApiBase.h"
#include "Systems/Spaces/SpaceSystemHelpers.h"
#include "TestHelpers.h"

using namespace csp::services;

CSP_INTERNAL_TEST(CSPEngine, UniqueStringTests, GetUniqueStringTest)
{

    const int TestLength = 10000;
    std::string UniqueHexStrings[TestLength];

    //static thread_local bool once = []
    //{
    //    std::random_device rd;
    //    auto s = rd();
    //    std::cout << "seed=" << s << " entropy=" << rd.entropy() << " thread=" << std::this_thread::get_id() << "\n";
    //    return true;
    //}();

    for (int i = 0; i < TestLength; i++)
    {
        std::string HexValue = GetUniqueString();

        auto Found = std::find(std::begin(UniqueHexStrings), std::end(UniqueHexStrings), HexValue);

        if (Found != std::end(UniqueHexStrings))
        {
            std::cout << "Value: " << *Found << std::endl;
            EXPECT_TRUE(false);
        }
        else
        {
            EXPECT_TRUE(true);
        }

        UniqueHexStrings[i] = HexValue;
    }
}
