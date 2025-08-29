# Unity Tests

This directory is the root of a [Unity] project used for running a mixture of generated and hand-written integration tests for the Wrapper Generator.

To run the tests, run [RunUnityTests.py] from the parent directory.

> [!TIP]
> To ensure the tests are running with a fresh Unity project, we recommend running `git clean -x -d -f` on the contents of this directory first to clear out any intermediate, temporary or generated files.

## Layout

### Generated Package

[RunUnityTests.py] copies the output of the wrapper generator to the `Packages` directory as `connected-spaces-platform.dummy`.

The package contains the generated C# bindings, a generated `.asmdef` file and `ConnectedSpacesPlatform_D.dll` containing the built test classes. This is **not** the actual CSP library.

### Generated Tests
The generated tests (see [GenerateTests.py]) are stored in `Assets/Tests/generated`.

### Conventions

This project should only contain C# source files (*.cs) and the minimum set of configuration files to be a valid Unity project.

**Do not check in any generated files**: This includes the *.meta files Unity uses for asset tracking.
Unity will generate these files upon opening the project, as well as configuration files for its various systems.

In the interest of supporting the widest range of Unity versions possible, we want to avoid including any files that might influence compatibility.

[Unity]: https://unity.com/
[GenerateTests.py]: ../../scripts/GenerateTests.py
[RunUnityTests.py]: ../RunUnityTests.py
