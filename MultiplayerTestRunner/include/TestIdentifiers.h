/*
 * Copyright 2024 Magnopus LLC

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
#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace
{

/*
 * Construct a new string from the input that is lower cased (via std::tolower)
 */
std::string ToLowerCaseString(const std::string& input)
{
    std::string output = input;
    std::transform(input.cbegin(), input.cend(), output.begin(), [](const unsigned char c) { return std::tolower(c); });
    return output;
}

} // namespace

namespace MultiplayerTestRunner::TestIdentifiers
{
/*
 * The test runner works by passing a specific test identifier in an a command line arg.
 * These things have to be strings, so this file serves to encode which tests are available,
 * and to give a nice way to get at the correct strings.
 * Absolutey drives me mad that there's no good compile time solution for these conversion implementations.
 */

/*
 * The identifier of the test to launch. Each of these should map to one test. See `main::RunTest`
 * To pass an identifier to the CLI, you need to pass the exact string representation defined in `TestIdentifierStringMap`.
 */
enum class TestIdentifier
{
    CREATE_AVATAR //"CreateAvatar"
};

inline const std::unordered_map<TestIdentifier, std::string> TestIdentifierStringMap { { TestIdentifier::CREATE_AVATAR, "CreateAvatar" } };

/*
 * Use `TestIdentifierStringMap` to convert a string to a test identifier, if valid.
 */
inline std::string TestIdentifierToString(TestIdentifier identifier)
{
    if (TestIdentifierStringMap.count(identifier))
    {
        return TestIdentifierStringMap.at(identifier);
    }
    throw std::invalid_argument("Invalid TestIdentifier value in TestIdentifierToString: " + std::to_string(static_cast<int>(identifier)));
}

/*
 * Use `TestIdentifierStringMap` to convert a string to a test identifier, if valid.
 */
inline TestIdentifier StringToTestIdentifier(std::string identifier)
{
    // Find the key based on the value, case insensitive. There's probably a better data structure for this but performance don't matter here.
    auto it = std::find_if(TestIdentifierStringMap.begin(), TestIdentifierStringMap.end(),
        [&identifier](const auto& pair) { return ToLowerCaseString(pair.second) == ToLowerCaseString(identifier); });

    if (it != TestIdentifierStringMap.end())
    {
        return it->first;
    }

    /*
     * Design question here, don't want to throw an internal exception because these
     * are public functions, but also, the String->Enum path probably isn't needed externally.
     * There's an argument that this implementation should be moved so an internal exception can be thrown.
     * However, it'd odd if it's not together with the Enum->String variant, makes maintainance harder.
     */
    throw std::invalid_argument("String `" + identifier + "` does not match any TestIdentifier");
}
} // namespace MultiplayerTestRunner::TestIdentifiers