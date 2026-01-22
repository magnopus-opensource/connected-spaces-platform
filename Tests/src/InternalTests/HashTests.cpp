/*
 * Copyright 2025 Magnopus LLC

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
    const Vector2 MyVector2 = { 0, 0 };
    const Vector3 MyVector3 = { 15.0f, 0, 999.99f };
    const Vector4 MyVector4 = { 15.0f, 5, 999.99f, 5 };

    const Vector2 MyVector2Alt = { 0, 0 };
    const Vector3 MyVector3Alt = { 15.0f, 0, 999.99f };
    const Vector4 MyVector4Alt = { 15.0f, 5, 999.99f, 5 };

    CompareHashEquality(MyVector2, MyVector2Alt);
    CompareHashEquality(MyVector3, MyVector3Alt);
    CompareHashEquality(MyVector4, MyVector4Alt);
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, StringHashEqualityTest)
{
    const String MyString = "Hello, World!";
    const String MyStringAlt = "Hello, World!";

    CompareHashEquality(MyString, MyStringAlt);
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, ArrayHashEqualityTest)
{
    Array<int> MyArray(3);
    MyArray[0] = 1;
    MyArray[1] = 2;
    MyArray[2] = 3;

    Array<int> MyArrayAlt(3);
    MyArrayAlt[0] = 1;
    MyArrayAlt[1] = 2;
    MyArrayAlt[2] = 3;

    // Arrays don't have equality, and adding it would introduce a requirement for contained types to have equality.
    // The best way to address this would be to add equality to all these types, but too big for me right this second.
    // Hence, don't use the utility, only test the hash.
    ASSERT_TRUE(std::hash<Array<int>>()(MyArray) == std::hash<Array<int>>()(MyArrayAlt));
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, ListHashEqualityTest)
{
    List<int> MyList;
    MyList.Append(1);
    MyList.Append(2);
    MyList.Append(3);

    List<int> MyListAlt;
    MyListAlt.Append(1);
    MyListAlt.Append(2);
    MyListAlt.Append(3);

    // Lists don't have equality, and adding it would introduce a requirement for contained types to have equality.
    // The best way to address this would be to add equality to all these types, but too big for me right this second.
    // Hence, don't use the utility, only test the hash.
    ASSERT_TRUE(std::hash<List<int>>()(MyList) == std::hash<List<int>>()(MyListAlt));
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, MapHashEqualityTest)
{
    Map<String, String> MyMap;
    MyMap["Key1"] = "Value1";
    MyMap["Key2"] = "Value2";

    Map<String, String> MyMapAlt;
    MyMapAlt["Key1"] = "Value1";
    MyMapAlt["Key2"] = "Value2";

    CompareHashEquality(MyMap, MyMapAlt);
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, ReplicatedValueHashEqualityTest)
{
    ReplicatedValue MyValueBool(true);
    ReplicatedValue MyValueBoolAlt(true);
    CompareHashEquality(MyValueBool, MyValueBoolAlt);

    ReplicatedValue MyValueInt(static_cast<int64_t>(42));
    ReplicatedValue MyValueIntAlt(static_cast<int64_t>(42));
    CompareHashEquality(MyValueInt, MyValueIntAlt);

    ReplicatedValue MyValueFloat(42.0f);
    ReplicatedValue MyValueFloatAlt(42.0f);
    CompareHashEquality(MyValueFloat, MyValueFloatAlt);

    ReplicatedValue MyValueString("FortyTwo");
    ReplicatedValue MyValueStringAlt("FortyTwo");
    CompareHashEquality(MyValueString, MyValueStringAlt);

    ReplicatedValue MyValueVector2(Vector2 { 4.0, 2.0 });
    ReplicatedValue MyValueVector2Alt(Vector2 { 4.0, 2.0 });
    CompareHashEquality(MyValueVector2, MyValueVector2Alt);

    Map<String, ReplicatedValue> MyMap;
    MyMap["Key1"] = ReplicatedValue { 42.0f };
    MyMap["Key2"] = ReplicatedValue { 42.0f };
    ReplicatedValue MyValue(MyMap);

    Map<String, ReplicatedValue> MyMapAlt;
    MyMapAlt["Key1"] = ReplicatedValue { 42.0f };
    MyMapAlt["Key2"] = ReplicatedValue { 42.0f };
    ReplicatedValue MyValueAlt(MyMapAlt);

    CompareHashEquality(MyMap, MyMapAlt);
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, ReplicatedValueTypeDifferenceHashTest)
{
    ReplicatedValue IntValue(static_cast<int64_t>(42));
    ReplicatedValue FloatValue(42.0f);

    EXPECT_FALSE(IntValue == FloatValue);
    EXPECT_FALSE(std::hash<ReplicatedValue>()(IntValue) == std::hash<ReplicatedValue>()(FloatValue));
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, ApplicationSettingsHashEqualityTest)
{
    ApplicationSettings MyApplicationSettings;
    MyApplicationSettings.ApplicationName = "TestApp";
    MyApplicationSettings.Context = "TestContext";
    MyApplicationSettings.AllowAnonymous = true;
    MyApplicationSettings.Settings["Setting1"] = "Value1";

    ApplicationSettings MyApplicationSettingsAlt;
    MyApplicationSettingsAlt.ApplicationName = "TestApp";
    MyApplicationSettingsAlt.Context = "TestContext";
    MyApplicationSettingsAlt.AllowAnonymous = true;
    MyApplicationSettingsAlt.Settings["Setting1"] = "Value1";

    CompareHashEquality(MyApplicationSettings, MyApplicationSettingsAlt);
}

CSP_INTERNAL_TEST(CSPEngine, HashTests, SettingsCollectionHashEqualityTest)
{
    SettingsCollection MySettingsCollection;
    MySettingsCollection.UserId = "User123";
    MySettingsCollection.Context = "TestContext";
    MySettingsCollection.Settings["Setting1"] = "Value1";

    SettingsCollection MySettingsCollectionAlt;
    MySettingsCollectionAlt.UserId = "User123";
    MySettingsCollectionAlt.Context = "TestContext";
    MySettingsCollectionAlt.Settings["Setting1"] = "Value1";

    CompareHashEquality(MySettingsCollection, MySettingsCollectionAlt);
}
