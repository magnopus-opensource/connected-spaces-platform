# Programming Guidelines
We closely follow coding standards outlined in the [Epic C++ Coding Standard](https://docs.unrealengine.com/5.2/en-US/epic-cplusplus-coding-standard-for-unreal-engine/).

All variable, field, parameter, class, and function names should be UpperCamelCase. We include a .clang-format file to help with code formatting which is run on a pre-commit hook.

## API Markup Standards

The source makes use of Macros to add additional functionality to the public interface when it is processed by the wrapper generator. Because of this, it's important to understand the correct usage of these Macros so that the library works correctly when being used with other languages.

* Use `CSP_API` to mark your class or free function for export. This allows it to be accessed across the DLL boundary and be made visible by the wrapper generator.
  ```c++
  CSP_API void DoSomething();

  class CSP_API MyClass { }
  ```
* Functions or fields that should not be wrapped should be marked with `CSP_NO_EXPORT`.
  ```c++
  // This function will still be exported in the DLL, but not wrapped by the wrapper generator.
  CSP_API CSP_NO_EXPORT void DoSomethingFromUnreal();
  ```

* Entire files can also be ignored by placing `CSP_NO_EXPORT` at the top of the file after the header includes.
* Blocks of code that should be ignored by the wrapper generator (in the case of syntax not being supported yet) should be surrounded by `CSP_START_IGNORE` and `CSP_END_IGNORE`.

  ```c++
  class CSP_API MyClass
  {
    CSP_START_IGNORE
    // Friend template class declarations are not yet parsed correctly by the wrapper generator.
    friend class csp::MyTemplateClass<int>;
    CSP_END_IGNORE
  }
  ```

* Reference-type parameters intended to store an output of a function should be marked with `CSP_OUT`.
  ```c++
  CSP_API void GetSomeIntValue(CSP_OUT int& OutValue)
  {
    OutValue = 5;
  }
  ```
* Reference-type parameters intended to both provide an initial value and store an output of a function should be marked with `CSP_IN_OUT`.
  ```c++
  CSP_API void ReadSomeBytes(CSP_IN_OUT int& Count) 
  {
    // Reads at most Count bytes from the socket and stores the number of read bytes in Count
    Count = socket.read(buffer, Count);
  }
  ```
* Functions that take a callback should always have the callback as the last parameter.

* Functions that take a callback that is intended to only be called once when the requested action has been completed should be marked with `CSP_ASYNC_RESULT`.
  ```c++
    // Log out of the current account asynchronously.
    CSP_API CSP_ASYNC_RESULT void LogOut(LogOutCallback Callback);
  ```
* If the callback should also be called with progress updates for the requested action (as is the case with file downloads), it should instead use `CSP_ASYNC_RESULT_WITH_PROGRESS`.

* Functions that take a callback that is intended to be called whenever an event occurs should be marked with `CSP_EVENT`.

  ```c++
    // Allow the user to set a callback to be notified when the client is disconnected.
    CSP_API CSP_EVENT void SetDisconnectCallback(DisconnectCallback Callback);
  ```

* Classes that are meant to be used as interfaces should be marked with `CSP_INTERFACE`.

  ```c++
  class CSP_API CSP_INTERFACE IMyInterfaceClass 
  {
    public:
      virtual void DoSomething() = 0;
  }
  ```

* _**NOTE: Interface classes must only contain pure virtual functions and must also contain a virtual default destructor.**_
* Classes that should never be disposed by the wrapper should be marked with `CSP_NO_DISPOSE`. This is useful for classes where only a single instance will ever be created and is owned by another class (as is the case for all of the *System classes and SystemsManager). Interfaces should also use the "I" prefix.

```c++
  class CSP_API CSP_NO_DISPOSE ClassWithSingleInstanceOwnedByManager { }
```