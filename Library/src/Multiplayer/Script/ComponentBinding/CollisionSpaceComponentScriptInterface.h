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

#include "Multiplayer/Script/ComponentScriptInterface.h"
#include "quickjspp.hpp"

#include <string>

namespace csp::multiplayer
{

class CollisionSpaceComponent;

class CollisionSpaceComponentScriptInterface : public ComponentScriptInterface
{
public:
    CollisionSpaceComponentScriptInterface(CollisionSpaceComponent* InComponent = nullptr);

    DECLARE_SCRIPT_PROPERTY(Vector3, Position);
    DECLARE_SCRIPT_PROPERTY(Vector3, Scale);
    DECLARE_SCRIPT_PROPERTY(Vector4, Rotation);
    DECLARE_SCRIPT_PROPERTY(int32_t, CollisionShape);
    DECLARE_SCRIPT_PROPERTY(int32_t, CollisionMode);
    DECLARE_SCRIPT_PROPERTY(std::string, CollisionAssetId);
    DECLARE_SCRIPT_PROPERTY(std::string, AssetCollectionId);
    DECLARE_SCRIPT_PROPERTY(float, Friction);
    DECLARE_SCRIPT_PROPERTY(float, Restitution);
    DECLARE_SCRIPT_PROPERTY(float, Mass);

    void SetKinematicPose(qjs::Value Pose, float Dt);
    void SetKinematicPosition(Vector3 Position, float Dt);
    void SetKinematicRotation(Vector4 Rotation, float Dt);
    void ResetKinematicPose();

    // Dynamic-rigidbody helpers. These invoke named actions on the collision
    // component; the renderer is expected to handle them only when the
    // collision mode is CollisionDynamic.
    void ApplyImpulse(Vector3 Impulse);
    void ApplyTorqueImpulse(Vector3 Torque);
    void SetLinearVelocity(Vector3 Velocity);
    void SetAngularVelocity(Vector3 Velocity);
};

} // namespace csp::multiplayer
