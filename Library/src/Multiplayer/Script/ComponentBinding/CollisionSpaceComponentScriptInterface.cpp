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
#include "Multiplayer/Script/ComponentBinding/CollisionSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/CollisionSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "fmt/format.h"

using namespace csp::systems;

namespace csp::multiplayer
{
namespace
{

std::string BuildVec3Json(const ComponentScriptInterface::Vector3& Value)
{
    if (Value.size() < 3)
    {
        return {};
    }

    return fmt::format("[{:.9g},{:.9g},{:.9g}]", Value[0], Value[1], Value[2]);
}

std::string BuildVec4Json(const ComponentScriptInterface::Vector4& Value)
{
    if (Value.size() < 4)
    {
        return {};
    }

    return fmt::format("[{:.9g},{:.9g},{:.9g},{:.9g}]", Value[0], Value[1], Value[2], Value[3]);
}

} // namespace

CollisionSpaceComponentScriptInterface::CollisionSpaceComponentScriptInterface(CollisionSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_VEC3(CollisionSpaceComponent, Position);
DEFINE_SCRIPT_PROPERTY_VEC4(CollisionSpaceComponent, Rotation);
DEFINE_SCRIPT_PROPERTY_VEC3(CollisionSpaceComponent, Scale);

DEFINE_SCRIPT_PROPERTY_TYPE(CollisionSpaceComponent, csp::multiplayer::CollisionMode, int32_t, CollisionMode);
DEFINE_SCRIPT_PROPERTY_TYPE(CollisionSpaceComponent, csp::multiplayer::CollisionShape, int32_t, CollisionShape);

DEFINE_SCRIPT_PROPERTY_STRING(CollisionSpaceComponent, CollisionAssetId);
DEFINE_SCRIPT_PROPERTY_STRING(CollisionSpaceComponent, AssetCollectionId);
DEFINE_SCRIPT_PROPERTY_TYPE(CollisionSpaceComponent, float, float, Friction);
DEFINE_SCRIPT_PROPERTY_TYPE(CollisionSpaceComponent, float, float, Restitution);
DEFINE_SCRIPT_PROPERTY_TYPE(CollisionSpaceComponent, float, float, Mass);

void CollisionSpaceComponentScriptInterface::SetKinematicPose(qjs::Value Pose, float Dt)
{
    if ((Component == nullptr) || !Pose.ctx)
    {
        return;
    }

    try
    {
        InvokeAction("setKinematicPose", fmt::format(R"({{"pose":{},"dt":{:.9g}}})", Pose.toJSON(), Dt));
    }
    catch (...)
    {
    }
}

void CollisionSpaceComponentScriptInterface::SetKinematicPosition(Vector3 Position, float Dt)
{
    const std::string PositionJson = BuildVec3Json(Position);
    if ((Component == nullptr) || PositionJson.empty())
    {
        return;
    }

    InvokeAction("setKinematicPosition", fmt::format(R"({{"position":{},"dt":{:.9g}}})", PositionJson, Dt));
}

void CollisionSpaceComponentScriptInterface::SetKinematicRotation(Vector4 Rotation, float Dt)
{
    const std::string RotationJson = BuildVec4Json(Rotation);
    if ((Component == nullptr) || RotationJson.empty())
    {
        return;
    }

    InvokeAction("setKinematicRotation", fmt::format(R"({{"rotation":{},"dt":{:.9g}}})", RotationJson, Dt));
}

void CollisionSpaceComponentScriptInterface::ResetKinematicPose()
{
    if (Component == nullptr)
    {
        return;
    }

    InvokeAction("resetKinematicPose", "{}");
}

} // namespace csp::multiplayer
