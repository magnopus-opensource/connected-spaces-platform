# C Interface

## General rules
* All functions in the C interface should be marked with `CSP_C_API` instead of `CSP_API`. This is to accommodate Emscripten's requirements for exported functions in the WASM binary.

## Classes
* All public fields and functions should be wrapped.
  * Fields are wrapped with a getter function and a setter function.
* Private constructors/destructors should not be wrapped.

## Template classes
* Every template instance needs to be wrapped.
* Template instances should be treated like regular classes.

## Return types
### Primitives

* By-value return types should be kept as-is.
  ```c++
  // C++
  namespace csp {
      CSP_API int32_t GetPrimitiveByValue();
  }

  // C
  CSP_C_API int32_t csp_GetPrimitiveByValue() {
      return csp::GetPrimitiveByValue();
  }
  ```

* Non-const pointer return types should be kept as-is.
  ```c++
  // C++
  namespace csp {
      CSP_API int32_t* GetPrimitiveByPointer();
  }

  // C
  CSP_C_API int32_t* csp_GetPrimitiveByPointer() {
      return csp::GetPrimitiveByPointer();
  }
  ```

* Const pointer return types should be returned by value.
  ```c++
  // C++
  namespace csp {
      CSP_API const int32_t* GetPrimitiveByConstPointer();
  }

  // C
  CSP_C_API int32_t csp_GetPrimitiveByConstPointer() {
      return *csp::GetPrimitiveByConstPointer();
  }
  ```

* Non-const reference return types should be returned as a pointer.
  ```c++
  // C++
  namespace csp {
      CSP_API int32_t& GetPrimitiveByReference();
  }

  // C
  CSP_C_API int32_t* csp_GetPrimitiveByReference() {
      return &csp::GetPrimitiveByReference();
  }
  ```

* Const reference return types should be returned by value.
  ```c++
  // C++
  namespace csp {
      CSP_API const int32_t& GetPrimitiveByConstReference();
  }

  // C
  CSP_C_API int32_t csp_GetPrimitiveByConstReference() {
      return csp::GetPrimitiveByConstReference();
  }
  ```

### Classes
* Classes are always returned as `NativePointer`.
* A copy should be made on the heap for classes returned by value, and `NativePointer.OwnsOwnData` should be `true`.
  ```c++
  // C++
  namespace csp {
      class ExampleClass;

      CSP_API ExampleClass GetClassByValue();
  }

  // C
  CSP_C_API NativePointer csp_GetClassByValue() {
      return NativePointer {
          CSP_NEW csp::ExampleClass(csp::GetClassByValue()),
          true
      };
  }
  ```

* Non-const pointers should be returned with `NativePointer.OwnsOwnData` set to `false`.
  ```c++
  // C++
  namespace csp {
      class ExampleClass;

      CSP_API ExampleClass* GetClassByPointer();
  }

  // C
  CSP_C_API NativePointer csp_GetClassByPointer() {
      return NativePointer {
          csp::GetClassByPointer(),
          false
      };
  }
  ```

* A copy should be made on the heap for classes returned by const pointer, and `NativePointer.OwnsOwnData` should be `true`.
  ```c++
  // C++
  namespace csp {
      class ExampleClass;

      CSP_API const ExampleClass* GetClassByConstPointer();
  }

  // C
  CSP_C_API NativePointer csp_GetClassByConstPointer() {
      return NativePointer {
          CSP_NEW csp::ExampleClass(*csp::GetClassByConstPointer()),
          true
      };
  }
  ```

* Non-const references should be returned by pointer with `NativePointer.OwnsOwnData` set to `false`.
  ```c++
  // C++
  namespace csp {
      class ExampleClass;

      CSP_API ExampleClass& GetClassByReference();
  }

  // C
  CSP_C_API NativePointer csp_GetClassByReference() {
      return NativePointer {
          &csp::GetClassByReference(),
          false
      };
  }
  ```

* A copy should be made on the heap for classes returned by const reference, and `NativePointer.OwnsOwnData` should be `true`.
  ```c++
  // C++
  namespace csp {
      class CSP_API ExampleClass {};

      CSP_API const ExampleClass& GetClassByConstReference();
  }

  // C
  CSP_C_API NativePointer csp_GetClassByConstReference() {
      return NativePointer {
          CSP_NEW csp::ExampleClass(csp::GetClassByConstReference()),
          true
      };
  }
  ```

### `csp::common::String`
* For Strings returned by value or by const-reference, a copy of the underlying C-string should be made on the heap and returned. The caller is responsible for freeing the memory allocated during the copy.
  ```c++
  // C++
  namespace csp {
      CSP_API csp::common::String GetStringByValue();
      CSP_API const csp::common::String& GetStringByConstReference();
  }

  // C
  CSP_C_API char* csp_GetStringByValue() {
      auto Value = csp::GetStringByValue();
      auto Buf = (char*)csp::memory::DllAlloc(Value.Length() + 1);
      std::memcpy(Buf, Value.c_str(), Value.Length());
      Buf[Value.Length()] = '\0';

      return Buf;
  }

  CSP_C_API NativePointer csp_GetStringByConstReference() {
      const auto& Value = csp::GetStringByConstReference();
      auto Buf = (char*)csp::memory::DllAlloc(Value.Length() + 1);
      std::memcpy(Buf, Value.c_str(), Value.Length());
      Buf[Value.Length()] = '\0';

      return Buf;
  }
  ```

* Returning Strings by pointer or non-const reference is not supported.

## Parameter types
### Primitives

* By-value parameter types should be kept as-is.
  ```c++
  // C++
  namespace csp {
      CSP_API void SetPrimitiveByValue(int32_t Value);
  }

  // C
  CSP_C_API void csp_SetPrimitiveByValue(int32_t Value) {
      csp::SetPrimitiveByValue(Value);
  }
  ```

* Non-const reference parameter types marked with `CSP_OUT` should be passed by pointer.
  ```c++
  // C++
  namespace csp {
      CSP_API void GetPrimitiveByOut(CSP_OUT int32_t& OutValue);
  }

  // C
  CSP_C_API void csp_GetPrimitiveByOut(int32_t* OutValue) {
      csp::GetPrimitiveByOut(&OutValue);
  }
  ```

* Non-const reference parameter types marked with `CSP_IN_OUT` should be passed by pointer.
  ```c++
  // C++
  namespace csp {
      CSP_API void SetPrimitiveByInGetByOut(CSP_IN_OUT int32_t& InOutValue);
  }

  // C
  CSP_C_API void SetPrimitiveByInGetByOut(int32_t* InOutValue) {
      csp::SetPrimitiveByInGetByOut(&InOutValue);
  }
  ```

* Pointer primitive types, const reference primitive types, and non-const primitive reference types not marked with `CSP_OUT` or `CSP_IN_OUT` are not supported as parameters.