/*
 * Copyright 2023 Magnopus LLC
 *
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

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "quickjspp.hpp"

// Implementation of js_traits for the CSP types that need to be marshaled between C++ and JavaScript

namespace qjs
{

// js_traits for PropertyType enum
template<> 
struct js_traits<csp::multiplayer::PropertyType, void> 
{
    static JSValue wrap(JSContext* ctx, csp::multiplayer::PropertyType val) 
    {
        return JS_NewInt32(ctx, static_cast<int32_t>(val));
    }
    
    static csp::multiplayer::PropertyType unwrap(JSContext* ctx, JSValueConst v) 
    {
        int32_t val;
        JS_ToInt32(ctx, &val, v);
        return static_cast<csp::multiplayer::PropertyType>(val);
    }
};

// Template specializations for some custom csp types we want to use
template <> struct qjs::js_property_traits<csp::common::String>
{
    static void set_property(JSContext* ctx, JSValue this_obj, csp::common::String str, JSValue value)
    {
        int err = JS_SetPropertyStr(ctx, this_obj, str.c_str(), value);

        if (err < 0)
            throw exception { ctx };
    }

    static JSValue get_property(JSContext* ctx, JSValue this_obj, csp::common::String str) noexcept
    {
        return JS_GetPropertyStr(ctx, this_obj, str.c_str());
    }
};

template <> struct qjs::js_traits<csp::common::String>
{
    static csp::common::String unwrap(JSContext* ctx, JSValueConst v)
    {
        size_t plen;
        const char* ptr = JS_ToCStringLen(ctx, &plen, v);

        if (!ptr)
            throw exception { ctx };

        return csp::common::String(ptr, plen);
    }

    static JSValue wrap(JSContext* ctx, csp::common::String str) noexcept { return JS_NewStringLen(ctx, str.c_str(), str.Length()); }
};

}

