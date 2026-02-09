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

#include "CSP/CSPCommon.h"
#include "CSP/Multiplayer/ComponentBase.h"

namespace csp::multiplayer
{

/// @ingroup ComponentExtensions
/// @brief A mechanism by which a component can be extended to reason about new properties, following a key/value pattern.
/// The key is a string-based identifier for the property, and the value is a ReplicatedValue which can represent a variety of primitive types.
//  This allows for the extension of components with additional properties without modifying a component's class.
//  This is particularly useful for prototyping new features, or for supporting the replication of custom data that doesn't warrant the creation of a
//  new component type.
class CSP_API ComponentExtensions
{
public:
    ComponentExtensions();
    ComponentExtensions(ComponentBase* InComponentToExtend);
    virtual ~ComponentExtensions();

    const csp::common::ReplicatedValue& GetProperty(const csp::common::String& Key) const;
    void SetProperty(const csp::common::String& Key, const csp::common::ReplicatedValue& Value);
    bool HasProperty(const csp::common::String& Key) const;

private:
    // The component being extended by this extension class. This is not owned by the ComponentExtensions class, and should be valid for the lifetime
    // of the ComponentExtensions instance.
    class ComponentBase* ExtendedComponent;

    // Components have a set of property keys that are used for the core properties of the component, which are defined in the component's
    // class. These keys are reserved and should avoided when adding new properties through this extension mechanism. 
    size_t ReservedPropertyRange;
};

} // namespace csp::multiplayer
