/*
 * Copyright 2026 Magnopus LLC

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

#include "CSP/Common/Hash.h"
#include "CSP/Common/Vector.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::common;

namespace
{
template <typename T> void CompareHashEquality(const T& l, const T& r)
{
    ASSERT_TRUE(l == r);
    ASSERT_TRUE(std::hash<T>()(l) == std::hash<T>()(r));
}
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, VectorHashEqualityTest)
{
    const Vector2 myVector2 = { 0, 0 };
    const Vector3 myVector3 = { 15.0f, 0, 999.99f };
    const Vector4 myVector4 = { 15.0f, 5, 999.99f, 5 };

    const Vector2 myVector2Alt = { 0, 0 };
    const Vector3 myVector3Alt = { 15.0f, 0, 999.99f };
    const Vector4 myVector4Alt = { 15.0f, 5, 999.99f, 5 };

    CompareHashEquality(myVector2, myVector2Alt);
    CompareHashEquality(myVector3, myVector3Alt);
    CompareHashEquality(myVector4, myVector4Alt);
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, StringHashEqualityTest)
{
    const String myString = "Hello, World!";
    const String myStringAlt = "Hello, World!";

    CompareHashEquality(myString, myStringAlt);
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, ArrayHashEqualityTest)
{
    Array<int> myArray(3);
    myArray[0] = 1;
    myArray[1] = 2;
    myArray[2] = 3;

    Array<int> myArrayAlt(3);
    myArrayAlt[0] = 1;
    myArrayAlt[1] = 2;
    myArrayAlt[2] = 3;

    // Arrays don't have equality, and adding it would introduce a requirement for contained types to have equality.
    // The best way to address this would be to add equality to all these types, but too big for me right this second.
    // Hence, don't use the utility, only test the hash.
    ASSERT_TRUE(std::hash<Array<int>>()(myArray) == std::hash<Array<int>>()(myArrayAlt));
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, ListHashEqualityTest)
{
    List<int> myList;
    myList.Append(1);
    myList.Append(2);
    myList.Append(3);

    List<int> myListAlt;
    myListAlt.Append(1);
    myListAlt.Append(2);
    myListAlt.Append(3);

    // Lists don't have equality, and adding it would introduce a requirement for contained types to have equality.
    // The best way to address this would be to add equality to all these types, but too big for me right this second.
    // Hence, don't use the utility, only test the hash.
    ASSERT_TRUE(std::hash<List<int>>()(myList) == std::hash<List<int>>()(myListAlt));
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, MapHashEqualityTest)
{
    Map<String, String> myMap;
    myMap["Key1"] = "Value1";
    myMap["Key2"] = "Value2";

    Map<String, String> myMapAlt;
    myMapAlt["Key1"] = "Value1";
    myMapAlt["Key2"] = "Value2";

    CompareHashEquality(myMap, myMapAlt);
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, ReplicatedValueHashEqualityTest)
{
    ReplicatedValue myValueBool(true);
    ReplicatedValue myValueBoolAlt(true);
    CompareHashEquality(myValueBool, myValueBoolAlt);

    ReplicatedValue myValueInt(static_cast<int64_t>(42));
    ReplicatedValue myValueIntAlt(static_cast<int64_t>(42));
    CompareHashEquality(myValueInt, myValueIntAlt);

    ReplicatedValue myValueFloat(42.0f);
    ReplicatedValue myValueFloatAlt(42.0f);
    CompareHashEquality(myValueFloat, myValueFloatAlt);

    ReplicatedValue myValueString("FortyTwo");
    ReplicatedValue myValueStringAlt("FortyTwo");
    CompareHashEquality(myValueString, myValueStringAlt);

    ReplicatedValue myValueVector2(Vector2 { 4.0, 2.0 });
    ReplicatedValue myValueVector2Alt(Vector2 { 4.0, 2.0 });
    CompareHashEquality(myValueVector2, myValueVector2Alt);

    Map<String, ReplicatedValue> myMap;
    myMap["Key1"] = ReplicatedValue { 42.0f };
    myMap["Key2"] = ReplicatedValue { 42.0f };
    ReplicatedValue myValue(myMap);

    Map<String, ReplicatedValue> myMapAlt;
    myMapAlt["Key1"] = ReplicatedValue { 42.0f };
    myMapAlt["Key2"] = ReplicatedValue { 42.0f };
    ReplicatedValue myValueAlt(myMapAlt);

    CompareHashEquality(myMap, myMapAlt);
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, ReplicatedValueTypeDifferenceHashTest)
{
    ReplicatedValue intValue(static_cast<int64_t>(42));
    ReplicatedValue floatValue(42.0f);

    EXPECT_FALSE(intValue == floatValue);
    EXPECT_FALSE(std::hash<ReplicatedValue>()(intValue) == std::hash<ReplicatedValue>()(floatValue));
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, ApplicationSettingsHashEqualityTest)
{
    ApplicationSettings myApplicationSettings;
    myApplicationSettings.ApplicationName = "TestApp";
    myApplicationSettings.Context = "TestContext";
    myApplicationSettings.AllowAnonymous = true;
    myApplicationSettings.Settings["Setting1"] = "Value1";

    ApplicationSettings myApplicationSettingsAlt;
    myApplicationSettingsAlt.ApplicationName = "TestApp";
    myApplicationSettingsAlt.Context = "TestContext";
    myApplicationSettingsAlt.AllowAnonymous = true;
    myApplicationSettingsAlt.Settings["Setting1"] = "Value1";

    CompareHashEquality(myApplicationSettings, myApplicationSettingsAlt);
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, SettingsCollectionHashEqualityTest)
{
    SettingsCollection mySettingsCollection;
    mySettingsCollection.UserId = "User123";
    mySettingsCollection.Context = "TestContext";
    mySettingsCollection.Settings["Setting1"] = "Value1";

    SettingsCollection mySettingsCollectionAlt;
    mySettingsCollectionAlt.UserId = "User123";
    mySettingsCollectionAlt.Context = "TestContext";
    mySettingsCollectionAlt.Settings["Setting1"] = "Value1";

    CompareHashEquality(mySettingsCollection, mySettingsCollectionAlt);
}
