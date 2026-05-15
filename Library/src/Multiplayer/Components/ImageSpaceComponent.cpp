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

#include "CSP/Multiplayer/ComponentSchema.h"

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::Image),
    "Image",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::Name_DEPRECATED),
            "name",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::ImageAssetId),
            "imageAssetId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::AssetCollectionId),
            "assetCollectionId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::Position),
            "position",
            csp::common::Vector3::Zero(),
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::Rotation),
            "rotation",
            csp::common::Vector4 { 0, 0, 0, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::Scale),
            "scale",
            csp::common::Vector3::One(),
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::IsVisible),
            "isVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::BillboardMode),
            "billboardMode",
            static_cast<int64_t>(BillboardMode::Off),
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::DisplayMode),
            "displayMode",
            static_cast<int64_t>(DisplayMode::DoubleSided),
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::IsARVisible),
            "isARVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::IsEmissive),
            "isEmissive",
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            true,
        },
    },
};

const ComponentSchema& ImageSpaceComponent::GetSchema() { return Schema; }

ImageSpaceComponent::ImageSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
}

const csp::common::String& ImageSpaceComponent::GetImageAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(ImagePropertyKeys::ImageAssetId));
}

void ImageSpaceComponent::SetImageAssetId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(ImagePropertyKeys::ImageAssetId), value);
}

const csp::common::String& ImageSpaceComponent::GetAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(ImagePropertyKeys::AssetCollectionId));
}

void ImageSpaceComponent::SetAssetCollectionId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(ImagePropertyKeys::AssetCollectionId), value);
}

const csp::common::String& ImageSpaceComponent::GetName() const
{
    return GetStringProperty(static_cast<uint32_t>(ImagePropertyKeys::Name_DEPRECATED));
}

void ImageSpaceComponent::SetName(const csp::common::String& value) { SetProperty(static_cast<uint32_t>(ImagePropertyKeys::Name_DEPRECATED), value); }

/* ITransformComponent */

const csp::common::Vector3& ImageSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ImagePropertyKeys::Position));
}

void ImageSpaceComponent::SetPosition(const csp::common::Vector3& value) { SetProperty(static_cast<uint32_t>(ImagePropertyKeys::Position), value); }

const csp::common::Vector4& ImageSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(ImagePropertyKeys::Rotation));
}

void ImageSpaceComponent::SetRotation(const csp::common::Vector4& value) { SetProperty(static_cast<uint32_t>(ImagePropertyKeys::Rotation), value); }

const csp::common::Vector3& ImageSpaceComponent::GetScale() const { return GetVector3Property(static_cast<uint32_t>(ImagePropertyKeys::Scale)); }

void ImageSpaceComponent::SetScale(const csp::common::Vector3& value) { SetProperty(static_cast<uint32_t>(ImagePropertyKeys::Scale), value); }

SpaceTransform ImageSpaceComponent::GetTransform() const
{
    SpaceTransform transform;
    transform.Position = GetPosition();
    transform.Rotation = GetRotation();
    transform.Scale = GetScale();

    return transform;
}

void ImageSpaceComponent::SetTransform(const SpaceTransform& inValue)
{
    SetPosition(inValue.Position);
    SetRotation(inValue.Rotation);
    SetScale(inValue.Scale);
}

bool ImageSpaceComponent::GetIsEmissive() const { return GetBooleanProperty(static_cast<uint32_t>(ImagePropertyKeys::IsEmissive)); }

void ImageSpaceComponent::SetIsEmissive(bool value) { SetProperty(static_cast<uint32_t>(ImagePropertyKeys::IsEmissive), value); }

/* IVisibleComponent */

bool ImageSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ImagePropertyKeys::IsVisible)); }

void ImageSpaceComponent::SetIsVisible(bool value) { SetProperty(static_cast<uint32_t>(ImagePropertyKeys::IsVisible), value); }

bool ImageSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ImagePropertyKeys::IsARVisible)); }

void ImageSpaceComponent::SetIsARVisible(bool value) { SetProperty(static_cast<uint32_t>(ImagePropertyKeys::IsARVisible), value); }

bool ImageSpaceComponent::GetIsVirtualVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ImagePropertyKeys::IsVirtualVisible)); }

void ImageSpaceComponent::SetIsVirtualVisible(bool value) { SetProperty(static_cast<uint32_t>(ImagePropertyKeys::IsVirtualVisible), value); }

BillboardMode ImageSpaceComponent::GetBillboardMode() const
{
    return static_cast<BillboardMode>(GetIntegerProperty(static_cast<uint32_t>(ImagePropertyKeys::BillboardMode)));
}

void ImageSpaceComponent::SetBillboardMode(BillboardMode value)
{
    SetProperty(static_cast<uint32_t>(ImagePropertyKeys::BillboardMode), static_cast<int64_t>(value));
}

DisplayMode ImageSpaceComponent::GetDisplayMode() const
{
    return static_cast<DisplayMode>(GetIntegerProperty(static_cast<uint32_t>(ImagePropertyKeys::DisplayMode)));
}

void ImageSpaceComponent::SetDisplayMode(DisplayMode value)
{
    SetProperty(static_cast<uint32_t>(ImagePropertyKeys::DisplayMode), static_cast<int64_t>(value));
}
} // namespace csp::multiplayer
