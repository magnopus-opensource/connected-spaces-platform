# Wrapper Generator Unit Tests

This directory contains a set of unit tests that exercise the Wrapper Generator and test its output.

These tests have been written for a Windows test environment and test two outputs:

* C# tests, running in [Unity] as [Play Mode tests](https://docs.unity3d.com/Packages/com.unity.test-framework@2.0/manual/edit-mode-vs-play-mode-tests.html#play-mode-tests) using the Unity Test Framework
* Typescript tests, running in [node.js]

The tests leverage code generation in a similar fashion to the wrapper generator:

1. Generate a C++ export library
2. Run the wrapper generator on the generated header files
3. Generate test suites for the target platforms
4. Execute the test suites

## Running the tests

### Unity (64-bit Windows)

Prerequisites:

* Unity 2020 or later
* Build environment configured for CSP
* Python configured with [requirements.txt](../teamcity/requirements.txt)

Starting in the root of the project:

```
cd UnitTesting
.\Generate_x64.bat
.\Build_x64.bat
```

To run the tests, generating a test report in [tests/unity_test_results.xml]:

```
cd tests
python RunUnityTests.py
```

Alternatively, open the Unity project in [tests/csharp](tests/csharp/) and run the tests manually.

1. [`scripts/generate.py`](scripts/generate.py) 

### Javascript/WASM

Prerequisites:

* Build environment configured for CSP
* Python configured with [requirements.txt](../teamcity/requirements.txt)
* node.js
* [Docker Desktop]

Starting in the root of the project:

```
cd UnitTesting
.\Generate_WASM.bat
.\Build_x64.bat
```

The WASM tests are currently unavailable but the above will run through the generation process.

[node.js]: https://nodejs.org/
[Unity]: https://unity.com/
[Docker Desktop]: https://docs.docker.com/desktop/
