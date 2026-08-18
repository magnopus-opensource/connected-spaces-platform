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
#include "Multiplayer/ComponentSchema.h"

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::Image),
    "Image",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::Name_DEPRECATED),
            "name_DEPRECATED",
            PlainValue<std::string> { "" },
            /*.IsScriptable =*/false,
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::ImageAssetId),
            "imageAssetId",
            PlainValue<std::string> { "" },
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::AssetCollectionId),
            "assetCollectionId",
            PlainValue<std::string> { "" },
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::Position),
            "position",
            PlainValue<csp::common::Vector3> { csp::common::Vector3::Zero() },
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::Rotation),
            "rotation",
            PlainValue<csp::common::Vector4> { csp::common::Vector4 { 0, 0, 0, 1 } },
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::Scale),
            "scale",
            PlainValue<csp::common::Vector3> { csp::common::Vector3::One() },
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::IsVisible),
            "isVisible",
            PlainValue<bool> { true },
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::BillboardMode),
            "billboardMode",
            PlainValue<int64_t> { static_cast<int64_t>(BillboardMode::Off) },
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::DisplayMode),
            "displayMode",
            PlainValue<int64_t> { static_cast<int64_t>(DisplayMode::DoubleSided) },
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::IsARVisible),
            "isARVisible",
            PlainValue<bool> { true },
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::IsEmissive),
            "isEmissive",
            PlainValue<bool> { false },
        },
        {
            static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            PlainValue<bool> { true },
        },
    },
};

const ComponentSchema& ImageSpaceComponent::GetSchema() { return Schema; }

ImageSpaceComponent::ImageSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ImageSpaceComponent(Schema, LogSystem, Parent)
{
}

std::unique_ptr<ImageSpaceComponent> ImageSpaceComponent::TryMake(
    const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
{
    if (!IsCompatible(ImageSpaceComponent::GetSchema(), InSchema))
    {
        return nullptr;
    }

    return std::unique_ptr<ImageSpaceComponent>(new ImageSpaceComponent(InSchema, LogSystem, Parent));
}

ImageSpaceComponent::ImageSpaceComponent(const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(InSchema, LogSystem, Parent)
{
}

const csp::common::String& ImageSpaceComponent::GetImageAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(ImagePropertyKeys::ImageAssetId));
}

void ImageSpaceComponent::SetImageAssetId(const csp::common::String& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(ImagePropertyKeys::ImageAssetId), Value);
}

const csp::common::String& ImageSpaceComponent::GetAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(ImagePropertyKeys::AssetCollectionId));
}

void ImageSpaceComponent::SetAssetCollectionId(const csp::common::String& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(ImagePropertyKeys::AssetCollectionId), Value);
}

const csp::common::String& ImageSpaceComponent::GetName() const
{
    return GetStringProperty(static_cast<uint32_t>(ImagePropertyKeys::Name_DEPRECATED));
}

void ImageSpaceComponent::SetName(const csp::common::String& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(ImagePropertyKeys::Name_DEPRECATED), Value);
}

/* ITransformComponent */

const csp::common::Vector3& ImageSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ImagePropertyKeys::Position));
}

void ImageSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(ImagePropertyKeys::Position), Value);
}

const csp::common::Vector4& ImageSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(ImagePropertyKeys::Rotation));
}

void ImageSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(ImagePropertyKeys::Rotation), Value);
}

const csp::common::Vector3& ImageSpaceComponent::GetScale() const { return GetVector3Property(static_cast<uint32_t>(ImagePropertyKeys::Scale)); }

void ImageSpaceComponent::SetScale(const csp::common::Vector3& Value) { SetPropertyDirect(static_cast<uint32_t>(ImagePropertyKeys::Scale), Value); }

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

void ImageSpaceComponent::SetIsEmissive(bool Value) { SetPropertyDirect(static_cast<uint32_t>(ImagePropertyKeys::IsEmissive), Value); }

/* IVisibleComponent */

bool ImageSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ImagePropertyKeys::IsVisible)); }

void ImageSpaceComponent::SetIsVisible(bool Value) { SetPropertyDirect(static_cast<uint32_t>(ImagePropertyKeys::IsVisible), Value); }

bool ImageSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ImagePropertyKeys::IsARVisible)); }

void ImageSpaceComponent::SetIsARVisible(bool Value) { SetPropertyDirect(static_cast<uint32_t>(ImagePropertyKeys::IsARVisible), Value); }

bool ImageSpaceComponent::GetIsVirtualVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ImagePropertyKeys::IsVirtualVisible)); }

void ImageSpaceComponent::SetIsVirtualVisible(bool Value) { SetPropertyDirect(static_cast<uint32_t>(ImagePropertyKeys::IsVirtualVisible), Value); }

BillboardMode ImageSpaceComponent::GetBillboardMode() const
{
    return static_cast<BillboardMode>(GetIntegerProperty(static_cast<uint32_t>(ImagePropertyKeys::BillboardMode)));
}

void ImageSpaceComponent::SetBillboardMode(BillboardMode Value)
{
    SetPropertyDirect(static_cast<uint32_t>(ImagePropertyKeys::BillboardMode), static_cast<int64_t>(Value));
}

DisplayMode ImageSpaceComponent::GetDisplayMode() const
{
    return static_cast<DisplayMode>(GetIntegerProperty(static_cast<uint32_t>(ImagePropertyKeys::DisplayMode)));
}

void ImageSpaceComponent::SetDisplayMode(DisplayMode Value)
{
    SetPropertyDirect(static_cast<uint32_t>(ImagePropertyKeys::DisplayMode), static_cast<int64_t>(Value));
}
} // namespace csp::multiplayer
