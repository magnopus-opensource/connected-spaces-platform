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

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"

namespace csp::Tests
{

class CSP_API SimpleClass
{
public:
    SimpleClass();
    ~SimpleClass();

    int GetValue() const;
};

class CSP_API BaseClass
{
public:
    BaseClass();
    virtual ~BaseClass();
};

class CSP_API DerivedClass : public BaseClass
{
public:
    DerivedClass();
    ~DerivedClass();
};

template <typename T> class CSP_API TemplateClass
{
public:
    TemplateClass();
    ~TemplateClass();

    void VoidFunction();

    void SetValue(const T& value);

    // Return type must be const for correct wrapper generation

    // Doesn't work
    // const T GetValue() const;
    // void GetValue(T& value) const;

private:
    T m_Value;
};

class CSP_API UsesTemplateClass
{
public:
    // Dummy functions to force generation of C wrapper functions for specific template types
    // Note that the type must be fully qualified here!

    // Doesn't work
    // void DummyFunctionInt(const csp::Tests::TemplateClass<int>& obj);

    const csp::Tests::TemplateClass<int>& DummyFunctionInt() const;
    const csp::Tests::TemplateClass<csp::common::String>& DummyFunctionString() const;

    // void DummyFunctionSimpleClass(const csp::Tests::TemplateClass<SimpleClass*>& obj);
};

// Doing this does NOT generate C wrapper functions
// CSP_API void DummyFunction(const csp::Tests::TemplateClass<int>& obj);
// CSP_API void DummyFunction(const csp::Tests::TemplateClass<csp::common::String>& obj);
// CSP_API void DummyFunction(const csp::Tests::TemplateClass<SimpleClass*>& obj);

} // namespace csp::Tests
