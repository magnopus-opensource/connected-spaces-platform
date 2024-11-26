# Debugging CSP for Unity

_Note: This tutorial is for debugging the native CSP DLL. If you are trying to debug the C# wrapper code, you can do this directly from your Unity project's generated Visual Studio project._

This guide assumes the developer is working on a Windows PC using Visual Studio.

1. If you haven't already done so already, please follow the [C# Build instructions](../building/csharp) to generate a locally debuggable version of the library.
2. Navigate to your local CSP repository.
3. Fetch the latest tags.
    - `git fetch --all`
4. Checkout the tag that matches your current CSP package version.
    - eg. `git checkout tags/v3.1.245 -b v3.1.245-branch`
    - This is required as the source you use to debug must be the same as the source that was used to build the Unity package. Visual Studio's debugger will load the required debug symbols from the PDB provided with the CSP package, so you do not need to build locally.
5. Run `generate_solution.bat` to generate the CSP project and solution files.
    - You _must_ re-run `generate_solution.bat` every time you pull latest, check out a different branch/tag, or make changes to the public headers.
    - This will ensure that your project file is up-to-date in case any new files were added or old ones removed.
6. Open `ConnectedSpacesPlatform.sln` in Visual Studio, switch to the `DebugDLL-CSharp` configuration, and make sure the target platform is x64 and not Android.
7. Open your Unity project.
8. Hit Play in Unity to start the session.
9. In Visual Studio, press `Ctrl + Alt + P` to show the `Attach to Process` window and attach to `Unity.exe`.
10. Set a breakpoint in some code you'd like to inspect in Visual Studio, then run your tests/press play in Unity.
11. Your breakpoint within the library will now be hit when the corresponding line of code is executed.

