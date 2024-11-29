# TypeScript Wrapper

## General rules
* The root C++ namespace `csp` should not appear in the wrapper generator. Child namespaces `systems`, `multiplayer`, etc. should, but should be translated to PascalCase.
* Type names (classes, interfaces, structs, etc.) should be left as-is (ie. PascalCase).
* Functions, class fields, and function parameters should be translated to camelCase to fit JavaScript naming conventions.

## Classes
* All public fields and functions should be wrapped.
* Fields are wrapped with a getter function and a setter function.
* Private constructors/destructors should not be wrapped.

## Return types
### Primitives

* Primitive C types should be translated to the equivalent TypeScript type.
  * Any integer type that is 32-bits or smaller in width -> `number`
  * Any integer type that is 64-bits in width -> `bigint`
  * Any floating-point type -> `number`
  * `char*` -> `string`

* By-value return types should be kept as-is.
  ``` c++
  // C++
  namespace csp {
    CSP_API int32_t GetPrimitiveByValue();
  }
  ```
  ``` typescript
  // TypeScript
  function getPrimitiveByValue(): number {
    // `Module.ccall()` is an Emscripten function that manages calling in to the C API.
    // It takes a function name, the return type as a string, an array of parameter types as strings,
    // and an array of arguments.
    return Module.ccall("csp_GetPrimitiveByValue", "number", [], []);
  }
  ```

* Non-const pointer or reference return types should be returned as `NativeRef`.
  ``` c++
  // C++
  namespace csp {
    CSP_API const int32_t* GetPrimitiveByPointer();
  }
  ```
  ``` typescript
  // TypeScript
  function getPrimitiveByPointer(): NativeRef {
    const _result = Module.ccall("csp_GetPrimitiveByPointer", "number", [], []);
    
    return new NativeRef(_result, NativeType.Int32);
  }
  ```

* Const pointer or reference return types should be passed by value.
  ``` c++
  // C++
  namespace csp {
    CSP_API const int32_t* GetPrimitiveByConstPointer();
  }
  ```
  ``` typescript
  // TypeScript
  function getPrimitiveByConstPointer(): number {
    let _result = Module.ccall("csp_GetPrimitiveByConstPointer", "number", [], []);
    
    return _result;
  }
  ```

**NOTE: Due to the nature of WebAssembly, _unsigned_ integer values need to be corrected to get the original value returned by CSP. For a 32-bit unsigned integer, the following snippet of code corrects the value by erasing the sign. Replace the `32` in `2 ** 32` with however many bits are in the type of the value you wish to correct.**
``` typescript
let _fixedValue = _unfixedValue < 0 ? _unfixedValue + 2 ** 32 : _unfixedValue;
```

### Classes
* Classes are always returned from the C interface as `NativePointer`.
  ``` c++
  // C++
  namespace csp {
    CSP_API MyClass& GetClassByReference();
  }
  ```
  ``` typescript
  // TypeScript
  function: getClassByReference(): MyClass {
    // Emscripten/WebAssembly does not return struct types.
    // Instead, we are required to allocate space to hold the resulting struct and pass a pointer
    // to this new memory as the first argument in the function call.
    var _ret = Module._malloc(8);
    Module.ccall("csp_Global_GetClassByReference_MyClassP", "void", [], [_ret]);

    // `getNativePointer` in our wrapper returns a NativePointer struct instance from a raw pointer
    // by reading values from the address.
    var _nPtr = new MyClass(getNativePointer(_ret));

    // Care should be taken to ensure that the allocated space is always freed!
    Module._free(_ret);

    return _nPtr;
  }
  ```

### `csp::common::String`
* `csp::common::String` is returned as a pointer. You should convert this pointer to a Javascript string using `Module.UTF8ToString` and then free the original pointer using `free(pointer)`,
  ``` c++
  // C++
  namespace csp {
    CSP_API csp::common::String GetStringByValue();
  }
  ```
  ``` typescript
  // TypeScript
  function getStringByValue(): string {
    const _result = Module.ccall("csp_GetStringByValue", "number", [], []);
    const _resultString = Module.UTF8ToString(_result);

    // `free()` is a function we have declared in our C++ library and takes care of freeing memory
    // allocated using our custom allocator.
    free(_result);

    return _resultString;
  }
  ```

## Parameter types
* Parameter names should be converted from `PascalCase` to `camelCase`.
* Out parameters should be removed from the parameter list when generate the TypeScript function, and should instead be included in the return value. If the function has no return value and only a single out parameter, the type of the out parameter becomes the return type of the function. If there is a return value, or there are multiple out parameters, the function should instead return an anonymous object containing the original return value as `result` and any out parameters using their original name as the key.
* In-out parameters should be treated the same way as out parameters, except they should not be removed from the list of parameters.

### Primitives

* By-value parameter types should be kept as-is.
  ``` c++
  // C++
  namespace csp {
    CSP_API void SetPrimitiveByValue(int32_t Value);
  }
  ```
  ``` typescript
  // TypeScript
  function setPrimitiveByValue(value: number): void {
    Module.ccall("csp_SetPrimitiveByValue", "void", ["number"], [value]);
  }
  ```

* Non-const reference parameter types marked with CSP_OUT should be passed as out parameters.
  ``` c++
  // C++
  namespace csp {
    CSP_API void GetPrimitiveByOut(CSP_OUT int32_t& OutValue);
  }
  ```
  ``` typescript
  // TypeScript
  function getPrimitveByOut(): number {
    // Space must be allocated to hold the value of the out parameter.
    const _pOutValue = Module._malloc(4);

    Module.ccall("csp_GetPrimitiveByOut", "void", ["number"], [_pOutValue]);

    // Read the value from the pointer as a 32-bit signed integer
    let _outValue = Module.getValue(_pOutValue, "i32");
    Module._free(_pOutValue);

    return _outValue;
  }
  ```

* Non-const reference parameter types marked with CSP_IN_OUT should be passed as ref parameters.
  ``` c++
  // C++
  namespace csp {
    CSP_API void SetPrimitiveByInGetByOut(CSP_IN_OUT int32_t& InOutValue);
  }
  ```
  ``` typescript
  // TypeScript
  function setPrimitiveByInGetByOut(inOutValue: number): number {
    const _pInOutValue = Module._malloc(4);

    // We should first write the value passed to this function to the space we allocated.
    Module.setValue(_pInOutValue, inOutValue, "i32");

    Module.ccall("csp_SetPrimitiveByInGetByOut", "void", ["number"], [_pInOutValue]);

    // Then, the resulting value needs to be read back from the pointer.
    let _inOutValue = Module.getValue(_pInOutValue, "i32");
    Module._free(_pInOutValue);

    return _inOutValue;
  }
  ```