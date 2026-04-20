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

/// @file AttachmentSpaceComponent.h
/// @brief Visually parents a SpaceEntity's render node to a named anchor (e.g. an XR controller).

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Multiplayer/ComponentBase.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for an attachment component.
enum class AttachmentPropertyKeys
{
    AnchorPath = 0,
    AttachedPosition,
    AttachedRotation,
    AttachedScale,
    Num
};

/// @ingroup AttachmentSpaceComponent
/// @brief Declares a visual attachment anchor for the owning SpaceEntity.
///
/// The renderer resolves `AnchorPath` (for example `/xr/left-hand`) to a node in its scene graph
/// and re-parents this entity's render root to that node. While attached, the entity's
/// `SpaceTransform` is interpreted as a local offset from the anchor root. The CSP-level
/// hierarchy (SpaceEntity::ParentId and the state patcher) is left untouched; the attachment
/// is a rendering concern only.
///
/// Path resolution is local to each client. If the path does not resolve (XR not active, unknown
/// scheme, anchor missing), the entity falls back to rendering at its `SpaceTransform` in world
/// space, exactly as if this component were absent.
class CSP_API AttachmentSpaceComponent : public ComponentBase
{
public:
    /// @brief Constructs the attachment component and associates it with the specified parent space entity.
    /// @param Parent The space entity that owns this component.
    AttachmentSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent);

    /// @brief Gets the anchor path that the renderer should resolve for this entity.
    /// An empty string means the entity is not attached.
    /// @return The anchor path string.
    const csp::common::String& GetAnchorPath() const;

    /// @brief Sets the anchor path that the renderer should resolve for this entity.
    /// Supported schemes (MVP): `/xr/left-hand`, `/xr/right-hand`. Set to empty string to detach.
    /// @param Value The anchor path string.
    void SetAnchorPath(const csp::common::String& Value);

    /// @brief Gets the local position applied to the entity's render node while the anchor
    /// path resolves. Ignored when not attached (the SpaceEntity's world transform is used instead).
    /// @return The position offset relative to the resolved anchor.
    const csp::common::Vector3& GetAttachedPosition() const;

    /// @brief Sets the local position applied to the entity's render node while the anchor
    /// path resolves.
    /// @param Value The position offset relative to the resolved anchor.
    void SetAttachedPosition(const csp::common::Vector3& Value);

    /// @brief Gets the local rotation (quaternion) applied to the entity's render node while
    /// the anchor path resolves. Ignored when not attached.
    /// @return The rotation relative to the resolved anchor.
    const csp::common::Vector4& GetAttachedRotation() const;

    /// @brief Sets the local rotation (quaternion) applied to the entity's render node while
    /// the anchor path resolves.
    /// @param Value The rotation relative to the resolved anchor.
    void SetAttachedRotation(const csp::common::Vector4& Value);

    /// @brief Gets the local scale applied to the entity's render node while the anchor
    /// path resolves. Ignored when not attached.
    /// @return The scale relative to the resolved anchor.
    const csp::common::Vector3& GetAttachedScale() const;

    /// @brief Sets the local scale applied to the entity's render node while the anchor
    /// path resolves.
    /// @param Value The scale relative to the resolved anchor.
    void SetAttachedScale(const csp::common::Vector3& Value);
};

} // namespace csp::multiplayer
