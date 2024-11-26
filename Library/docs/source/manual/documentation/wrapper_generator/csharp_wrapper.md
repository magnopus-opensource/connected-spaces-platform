# C# Wrapper

## General rules
* The root namespace of the C# wrapper should be `Csp`.

## Classes
* All classes without a base type should inherit from `NativeClassWrapper`.
  * The exception to this is classes marked with `CSP_INTERFACE`, which should have no base type.
* All classes marked with `CSP_INTERFACE` should be wrapped as C# interfaces.

## Template classes
* Template classes should be wrapped as C# generic classes.

## Return types
### Primitives

* Primitive C types should be translated to the equivalent C# type.
  * `int8_t` -> `sbyte`
  * `uint8_t` -> `byte`
  * `short`, `signed short`, `short int`, `signed short int`, `int16_t` -> `short`
  * `unsigned short`, `unsigned short int`, `uint16_t` -> `ushort`
  * `int`, `signed int`, `long`, `signed long`, `long int`, `signed long int`, `int32_t` -> `int`
  * `unsigned int`, `unsigned long`, `unsigned long int`, `uint32_t` -> `uint`
  * `long long`, `signed long long`, `long long int`, `signed long long int`, `int64_t` -> `long`
  * `unsigned long long`, `unsigned long long int`, `uint64_t` -> `ulong`
  * `float` -> `float`
  * `double` -> `double`

* The P/Invoke signature for any C wrapper functions returning primitive pointers should use IntPtr to represent the pointer.

* By-value return types should be kept as-is.
  ```c++
  // C++
  namespace csp {
    CSP_API int32_t GetPrimitiveByValue();
  }
  ```
  ``` c#
  // C#
  namespace Csp {
    public static class Global {
      [DllImport("OlympusFoundation.dll")]
      static extern int csp_GetPrimitiveByValue();

      public int GetPrimitiveByValue() {
        return csp_GetPrimitiveByValue();
      }
    }
  }
  ```

* Non-const pointer and reference return types should be returned as `Ref<T>`.
  ```c++
  // C++
  namespace csp {
    CSP_API int32_t& GetPrimitiveByReference();
  }
  ```
  ``` c#
  // C#
  namespace Csp {
    public static class Global {
      [DllImport("OlympusFoundation.dll")]
      static extern IntPtr csp_GetPrimitiveByReference();

      public Ref<int> GetPrimitiveByReference() {
        return new Ref<int>(csp_GetPrimitiveByReference());
      }
    }
  }
  ```

* Const pointer or reference return types should be passed by value.
  ```c++
  // C++
  namespace csp {
    CSP_API const int32_t* GetPrimitiveByConstPointer();
  }
  ```
  ``` c#
  // C#
  namespace Csp {
    public static class Global {
      [DllImport("OlympusFoundation.dll")]
      static extern int csp_GetPrimitiveByConstPointer();

      public int GetPrimitiveByConstPointer() {
        return csp_GetPrimitiveByConstPointer();
      }
    }
  }
  ```

### Classes

* Classes are always returned from the C interface as `NativePointer`.
* The returned pointer should be passed to the class' `internal` constructor.
  ```c++
  // C++
  namespace csp {
    CSP_API MyClass& GetClassByReference();
  }
  ```
  ```c#
  // C#
  namespace Csp {
    public static class Global {
      [DllImport("OlympusFoundation.dll")]
      static extern NativePointer csp_GetClassByReference();

      public Csp.MyClass GetClassByReference() {
        return new Csp.MyClass(csp_GetClassByReference());
      }
    }
  }
  ```

### `csp::common::String`

* String returns should be converted to a C# string using `WrapperHelper.NativeUTF8ToString()`. The pointer returned from the C interface should then freed with `Global.Free()`.
  ```c++
  // C++
  namespace csp {
    CSP_API csp::common::String GetStringByValue();
  }
  ```
  ```c#
  // C#
  namespace Csp {
    public static class Global {
      [DllImport("OlympusFoundation.dll")]
      static extern IntPtr csp_GetStringByValue();

      public string GetStringByValue() {
        var result = csp_GetStringByValue();

        var stringResult = WrapperHelper.NativeUTF8ToString(result);
        Global.Free(result);

        return stringResult;
      }
    }
  }
  ```

## Parameter types
* Parameter names should be converted from `UpperCamelCase` to `lowerCamelCase`.

### Primitives

* By-value parameter types should be kept as-is.
  ```c++
  // C++
  namespace csp {
    CSP_API void SetPrimitiveByValue(int32_t Value);
  }
  ```
  ```c#
  // C#
  namespace Csp {
    public static class Global {
      [DllImport("OlympusFoundation.dll")]
      static extern void csp_SetPrimitiveByValue(int value);

      public void SetPrimitiveByValue(int value) {
        csp_SetPrimitiveByValue(value);
      }
    }
  }
  ```

* Non-const reference parameter types marked with `CSP_OUT` should be passed as out parameters.
  ```c++
  // C++
  namespace csp {
    CSP_API void GetPrimitiveByOut(CSP_OUT int32_t& OutValue);
  }
  ```
  ```c#
  // C#
  namespace Csp {
    public static class Global {
      [DllImport("OlympusFoundation.dll")]
      static extern void csp_GetPrimitiveByOut(out int outValue);

      public void GetPrimitiveByOut(out int outValue) {
        csp_SetPrimitiveByValue(out outValue);
      }
    }
  }
  ```

* Non-const reference parameter types marked with `CSP_IN_OUT` should be passed as ref parameters.
  ```c++
  // C++
  namespace csp {
    CSP_API void SetPrimitiveByInGetByOut(CSP_IN_OUT int32_t& InOutValue);
  }
  ```
  ```c#
  // C#
  namespace Csp {
    public static class Global {
      [DllImport("OlympusFoundation.dll")]
      static extern void csp_SetPrimitiveByInGetByOut(ref int inOutValue);

      public void SetPrimitiveByInGetByOut(ref int inOutValue) {
        csp_SetPrimitiveByInGetByOut(ref inOutValue);
      }
    }
  }
  ```