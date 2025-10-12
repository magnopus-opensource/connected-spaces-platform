/*
 * Copyright 2025 Magnopus LLC

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

// This should definately be one file per module, just for proto.

#include <iostream>

namespace csp::multiplayer
{

class SpaceEntity;

// This virtual callback pattern seems to be the only real way to get C# callbacks working comfortably through SWIG
// Tbh I don't mind, it's rather clean, and reasonably adaptable.
// Gotta think about copy behaviour, maybe we shouldn't allow these objects to be copied ... gotta consider the proxy class in the client language.
class EntityCreatedCallback
{
public:
    virtual ~EntityCreatedCallback() { std::cout << "EntityCreatedCallback::~EntityCreatedCallback()" << std::endl; }
    virtual void Call(csp::multiplayer::SpaceEntity*) const { std::cout << "EntityCreatedCallback::run()" << std::endl; }
};

class LongRunningOperationToMakeAnIntCallback
{
public:
    virtual ~LongRunningOperationToMakeAnIntCallback() = default;
    virtual void Call(int) const {};
};

} // namespace csp::mulitplayer