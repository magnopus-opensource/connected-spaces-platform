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

#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Systems/SystemsResult.h"

namespace csp::json
{
class JsonSerializer;
class JsonDeserializer;
} // namespace csp::json

namespace csp::common
{

class DateTime;

} // namespace csp::common

namespace csp::services
{

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

class UserSystem;

/// @brief Data structure for an individual analytics frame users Analytics Session.
class CSP_API UserAnalyticFrame
{
public:
    /// @brief The position of the user.
    csp::common::Vector3 Position;

    /// @brief The unique identifer for the user.
    csp::common::Vector4 HeadRotation;

    /// @brief The offset in MS since AnalyticsSession StartTime.
    int32_t StartTimeOffsetMS;
};

/// @brief Data structure for an individual users Analytics Session.
class CSP_API UserAnalyticsSession
{
public:
    /// @brief The unique identifer for the user.
    csp::common::String UserId;

    /// @brief The start time for the users Analytics Session.
    csp::common::String StartTime;

    /// @brief The end time for the users Analytics Session.
    csp::common::String EndTime;

    /// @brief An array of analytic frames for the user.
    csp::common::Array<UserAnalyticFrame> AnalyticFrames;
};

/// @brief Data structure for an Analytics Session.
class CSP_API AnalyticsSession
{
public:
    /// @brief The unique identifer for the Space.
    csp::common::String SpaceId;

    /// @brief An array of analytics data for each user in the space.
    csp::common::Array<UserAnalyticsSession> UserAnalyticsData;
};

/// @brief Result structure for a space analytics request.
class CSP_API SpaceAnalyticsResult : public ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    friend class UserSystem;
    CSP_END_IGNORE
    /** @endcond */

public:
    [[nodiscard]] const AnalyticsSession& GetAnalyticsSession() const;

private:
    SpaceAnalyticsResult();
    SpaceAnalyticsResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    AnalyticsSession* Session;
};

typedef std::function<void(const SpaceAnalyticsResult& Result)> SpaceAnalyticsResultCallback;

} // namespace csp::systems

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::AnalyticsSession& Obj);
void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::AnalyticsSession& Obj);

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::UserAnalyticsSession& Obj);
void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::UserAnalyticsSession& Obj);

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::UserAnalyticFrame& Obj);
void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::UserAnalyticFrame& Obj);
