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

#include "CSP/Systems/Users/SpaceAnalytics.h"

#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Common/DateTime.h"
#include "Debug/Logging.h"
#include "Events/EventSystem.h"
#include "Services/AggregationService/Api.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/UserService/Dto.h"

#include "Json/JsonSerializer.h"

#include "Common/Convert.h"

using namespace csp;
using namespace csp::common;

namespace chs_users = csp::services::generated::userservice;

void ConvertDtoToAnalyticsSession(const chs_users::AnalyticsSessionDto& Dto, csp::systems::AnalyticsSession& Session)
{
    Session.SpaceId = Dto.GetSpaceId();

    auto& AnalyticsData = Dto.GetUserAnalyticsData();
    Session.UserAnalyticsData = csp::common::Array<systems::UserAnalyticsSession>(AnalyticsData.size());

    for (size_t i = 0; i < AnalyticsData.size(); ++i)
    {
        Session.UserAnalyticsData[i].UserId = AnalyticsData[i]->GetUserId();
        Session.UserAnalyticsData[i].StartTime = AnalyticsData[i]->GetStartTime();
        Session.UserAnalyticsData[i].EndTime = AnalyticsData[i]->GetEndTime();

        auto& AnalyticsFrames = AnalyticsData[i]->GetAnalyticsFrames();
        Session.UserAnalyticsData[i].AnalyticFrames = csp::common::Array<systems::UserAnalyticFrame>(AnalyticsFrames.size());

        for (size_t j = 0; j < AnalyticsFrames.size(); ++j)
        {
            Session.UserAnalyticsData[i].AnalyticFrames[i].Position.X = AnalyticsFrames[i]->GetPosition()->GetX();
            Session.UserAnalyticsData[i].AnalyticFrames[i].Position.Y = AnalyticsFrames[i]->GetPosition()->GetY();
            Session.UserAnalyticsData[i].AnalyticFrames[i].Position.Z = AnalyticsFrames[i]->GetPosition()->GetZ();
            Session.UserAnalyticsData[i].AnalyticFrames[i].HeadRotation.X = AnalyticsFrames[i]->GetRotation()->GetX();
            Session.UserAnalyticsData[i].AnalyticFrames[i].HeadRotation.Y = AnalyticsFrames[i]->GetRotation()->GetY();
            Session.UserAnalyticsData[i].AnalyticFrames[i].HeadRotation.Z = AnalyticsFrames[i]->GetRotation()->GetZ();
            Session.UserAnalyticsData[i].AnalyticFrames[i].StartTimeOffsetMS = AnalyticsFrames[i]->GetStartTimeOffsetMS();
        }
    }
}

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::AnalyticsSession& Obj)
{
    // Space Id
    Serializer.SerializeMember("SpaceId", Obj.SpaceId);

    // Array of UserAnalyticsSession
    Serializer.SerializeMember("UserAnalyticsData", Obj.UserAnalyticsData);
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::AnalyticsSession& Obj)
{
    // Space Id
    Deserializer.DeserializeMember("SpaceId", Obj.SpaceId);

    // User Analytics Data
    Deserializer.DeserializeMember("UserAnalyticsData", Obj.UserAnalyticsData);
}

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::UserAnalyticsSession& Obj)
{
    // User Id
    Serializer.SerializeMember("UserId", Obj.UserId);

    // Start Time
    Serializer.SerializeMember("StartTime", Obj.StartTime);

    // End Time
    Serializer.SerializeMember("EndTime", Obj.EndTime);

    // Analytic Frames
    Serializer.SerializeMember("AnalyticFrames", Obj.AnalyticFrames);
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::UserAnalyticsSession& Obj)
{
    // UserId
    Deserializer.DeserializeMember("UserId", Obj.UserId);

    // Start Time
    Deserializer.DeserializeMember("StartTime", Obj.StartTime);

    // End Time
    Deserializer.DeserializeMember("EndTime", Obj.EndTime);

    // AnalyticFrames
    Deserializer.DeserializeMember("AnalyticFrames", Obj.AnalyticFrames);
}

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::UserAnalyticFrame& Obj)
{
    // Position
    Serializer.SerializeMember("Position", csp::common::Array<float> { Obj.Position.X, Obj.Position.Y, Obj.Position.Z });

    // Head Rotation
    Serializer.SerializeMember(
        "HeadRotation", csp::common::Array<float> { Obj.HeadRotation.X, Obj.HeadRotation.Y, Obj.HeadRotation.Z, Obj.HeadRotation.W });

    // Start Time Offset MS
    Serializer.SerializeMember("StartTimeOffsetMS", static_cast<uint32_t>(Obj.StartTimeOffsetMS));
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::UserAnalyticFrame& Obj)
{
    // Position
    csp::common::Array<float> PositionArray;
    Deserializer.DeserializeMember("Position", PositionArray);

    Obj.Position = csp::common::Vector3(PositionArray[0], PositionArray[1], PositionArray[2]);

    // Head Rotation
    csp::common::Array<float> HeadRotationArray;
    Deserializer.DeserializeMember("HeadRotation", HeadRotationArray);

    Obj.HeadRotation = csp::common::Vector4(HeadRotationArray[0], HeadRotationArray[1], HeadRotationArray[2], HeadRotationArray[3]);

    // Start Time Offset MS
    Deserializer.DeserializeMember("StartTimeOffsetMS", Obj.StartTimeOffsetMS);
}

namespace csp::systems
{

SpaceAnalyticsResult::SpaceAnalyticsResult() { }

const AnalyticsSession& SpaceAnalyticsResult::GetAnalyticsSession() const { return Session; }

void SpaceAnalyticsResult::OnResponse(const services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto AnalyticsResponse = static_cast<chs_users::AnalyticsSessionDto*>(ApiResponse->GetDto());
    const web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        AnalyticsResponse->FromJson(Response->GetPayload().GetContent());

        ConvertDtoToAnalyticsSession(*AnalyticsResponse, Session);
    }
}

} // namespace csp::systems
