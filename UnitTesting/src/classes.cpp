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

#include "classes.h"

namespace csp::Tests
{

// SimpleClass
SimpleClass::SimpleClass() { }
SimpleClass::~SimpleClass() { }
int SimpleClass::GetValue() const { return 42; }

// BaseClass / DerivedClass
BaseClass::BaseClass() { }
BaseClass::~BaseClass() { }

DerivedClass::DerivedClass() { }
DerivedClass::~DerivedClass() { }

// TemplateClass
template <typename T> TemplateClass<T>::TemplateClass() { }

template <typename T> TemplateClass<T>::~TemplateClass() { }

template <typename T> void TemplateClass<T>::SetValue(const T& value) { m_Value = value; }

template <typename T> void TemplateClass<T>::GetValue(T& value) const { value = m_Value; }

} // namespace csp::Tests