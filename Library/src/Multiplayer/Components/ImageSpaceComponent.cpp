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
#include "CSP/Multiplayer/Components/ImageSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/ImageSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

ImageSpaceComponent::ImageSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::Image, Parent)
{
    Properties[static_cast<uint32_t>(ImagePropertyKeys::Name_DEPRECATED)] = "";
    Properties[static_cast<uint32_t>(ImagePropertyKeys::ImageAssetId)] = "";
    Properties[static_cast<uint32_t>(ImagePropertyKeys::AssetCollectionId)] = "";
    Properties[static_cast<uint32_t>(ImagePropertyKeys::Position)] = csp::common::Vector3::Zero();
    Properties[static_cast<uint32_t>(ImagePropertyKeys::Rotation)] = csp::common::Vector4 { 0, 0, 0, 1 };
    Properties[static_cast<uint32_t>(ImagePropertyKeys::Scale)] = csp::common::Vector3::One();
    Properties[static_cast<uint32_t>(ImagePropertyKeys::IsVisible)] = true;
    Properties[static_cast<uint32_t>(ImagePropertyKeys::BillboardMode)] = static_cast<int64_t>(BillboardMode::Off);
    Properties[static_cast<uint32_t>(ImagePropertyKeys::DisplayMode)] = static_cast<int64_t>(DisplayMode::DoubleSided);
    Properties[static_cast<uint32_t>(ImagePropertyKeys::IsARVisible)] = true;
    Properties[static_cast<uint32_t>(ImagePropertyKeys::IsEmissive)] = false;

    SetScriptInterface(CSP_NEW ImageSpaceComponentScriptInterface(this));
}

const csp::common::String& ImageSpaceComponent::GetImageAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(ImagePropertyKeys::ImageAssetId));
}

void ImageSpaceComponent::SetImageAssetId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ImagePropertyKeys::ImageAssetId), Value);
}

const csp::common::String& ImageSpaceComponent::GetAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(ImagePropertyKeys::AssetCollectionId));
}

void ImageSpaceComponent::SetAssetCollectionId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ImagePropertyKeys::AssetCollectionId), Value);
}

const csp::common::String& ImageSpaceComponent::GetName() const
{
    return GetStringProperty(static_cast<uint32_t>(ImagePropertyKeys::Name_DEPRECATED));
}

void ImageSpaceComponent::SetName(const csp::common::String& Value) { SetProperty(static_cast<uint32_t>(ImagePropertyKeys::Name_DEPRECATED), Value); }

/* ITransformComponent */

const csp::common::Vector3& ImageSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ImagePropertyKeys::Position));
}

void ImageSpaceComponent::SetPosition(const csp::common::Vector3& Value) { SetProperty(static_cast<uint32_t>(ImagePropertyKeys::Position), Value); }

const csp::common::Vector4& ImageSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(ImagePropertyKeys::Rotation));
}

void ImageSpaceComponent::SetRotation(const csp::common::Vector4& Value) { SetProperty(static_cast<uint32_t>(ImagePropertyKeys::Rotation), Value); }

const csp::common::Vector3& ImageSpaceComponent::GetScale() const { return GetVector3Property(static_cast<uint32_t>(ImagePropertyKeys::Scale)); }

void ImageSpaceComponent::SetScale(const csp::common::Vector3& Value) { SetProperty(static_cast<uint32_t>(ImagePropertyKeys::Scale), Value); }

SpaceTransform ImageSpaceComponent::GetTransform() const
{
    SpaceTransform Transform;
    Transform.Position = GetPosition();
    Transform.Rotation = GetRotation();
    Transform.Scale = GetScale();

    return Transform;
}

void ImageSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
    SetPosition(InValue.Position);
    SetRotation(InValue.Rotation);
    SetScale(InValue.Scale);
}

bool ImageSpaceComponent::GetIsEmissive() const { return GetBooleanProperty(static_cast<uint32_t>(ImagePropertyKeys::IsEmissive)); }

void ImageSpaceComponent::SetIsEmissive(bool Value) { SetProperty(static_cast<uint32_t>(ImagePropertyKeys::IsEmissive), Value); }

/* IVisibleComponent */

bool ImageSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ImagePropertyKeys::IsVisible)); }

void ImageSpaceComponent::SetIsVisible(bool Value) { SetProperty(static_cast<uint32_t>(ImagePropertyKeys::IsVisible), Value); }

bool ImageSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ImagePropertyKeys::IsARVisible)); }

void ImageSpaceComponent::SetIsARVisible(bool Value) { SetProperty(static_cast<uint32_t>(ImagePropertyKeys::IsARVisible), Value); }

BillboardMode ImageSpaceComponent::GetBillboardMode() const
{
    return static_cast<BillboardMode>(GetIntegerProperty(static_cast<uint32_t>(ImagePropertyKeys::BillboardMode)));
}

void ImageSpaceComponent::SetBillboardMode(BillboardMode Value)
{
    SetProperty(static_cast<uint32_t>(ImagePropertyKeys::BillboardMode), static_cast<int64_t>(Value));
}

DisplayMode ImageSpaceComponent::GetDisplayMode() const
{
    return static_cast<DisplayMode>(GetIntegerProperty(static_cast<uint32_t>(ImagePropertyKeys::DisplayMode)));
}

void ImageSpaceComponent::SetDisplayMode(DisplayMode Value)
{
    SetProperty(static_cast<uint32_t>(ImagePropertyKeys::DisplayMode), static_cast<int64_t>(Value));
}
} // namespace csp::multiplayer
